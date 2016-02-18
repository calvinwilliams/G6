#include "G6.h"

static void IgnoreReverseSessionEvents( struct ForwardSession *p_forward_session , struct epoll_event *p_events , int event_count )
{
	int			event_index ;
	struct ForwardSession	*p_reverse_forward_session = p_forward_session->p_reverse_forward_session ;
	
	for( event_index = 0 ; event_index < event_count ; event_index++ )
	{
		if( p_events[event_index].data.ptr == p_forward_session )
			continue;
		
		if( p_events[event_index].data.ptr == p_reverse_forward_session )
			p_events[event_index].events = 0 ;
	}
	
	return;
}

static int OnForwardInput( struct ServerEnv *penv , struct ForwardSession *p_forward_session , int forward_epoll_fd , struct epoll_event *p_events , int event_count )
{
	int			len ;
	struct epoll_event	event ;
	
	p_forward_session->io_buffer_offsetpos = 0 ;
	p_forward_session->io_buffer_len = recv( p_forward_session->sock , p_forward_session->io_buffer , IO_BUFFER_SIZE , 0 ) ;
	if( p_forward_session->io_buffer_len == 0 )
	{
		InfoLog( __FILE__ , __LINE__ , "recv #%d# closed" , p_forward_session->sock , p_forward_session->io_buffer_len );
		epoll_ctl( forward_epoll_fd , EPOLL_CTL_DEL , p_forward_session->sock , NULL );
		epoll_ctl( forward_epoll_fd , EPOLL_CTL_DEL , p_forward_session->p_reverse_forward_session->sock , NULL );
		SetForwardSessionUnused( p_forward_session );
		SetForwardSessionUnused( p_forward_session->p_reverse_forward_session );
		return 0;
	}
	else if( p_forward_session->io_buffer_len == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "recv #%d# failed , errno[%d]" , p_forward_session->sock , p_forward_session->io_buffer_len , errno );
		IgnoreReverseSessionEvents( p_forward_session , p_events , event_count );
		epoll_ctl( forward_epoll_fd , EPOLL_CTL_DEL , p_forward_session->sock , NULL );
		epoll_ctl( forward_epoll_fd , EPOLL_CTL_DEL , p_forward_session->p_reverse_forward_session->sock , NULL );
		SetForwardSessionUnused( p_forward_session );
		SetForwardSessionUnused( p_forward_session->p_reverse_forward_session );
		return 0;
	}
	
	len = send( p_forward_session->p_reverse_forward_session->sock , p_forward_session->io_buffer , p_forward_session->io_buffer_len , 0 ) ;
	if( len == -1 )
	{
		if( _ERRNO == _EWOULDBLOCK )
		{
			IgnoreReverseSessionEvents( p_forward_session , p_events , event_count );
			
			p_forward_session->io_buffer_len -= len ;
			p_forward_session->io_buffer_offsetpos = len ;
			
			memset( & event , 0x00 , sizeof(struct epoll_event) );
			event.data.ptr = p_forward_session ;
			event.events = EPOLLERR | EPOLLET ;
			epoll_ctl( forward_epoll_fd , EPOLL_CTL_MOD , p_forward_session->sock , & event );
			
			memset( & event , 0x00 , sizeof(struct epoll_event) );
			event.data.ptr = p_forward_session->p_reverse_forward_session ;
			event.events = EPOLLOUT | EPOLLERR | EPOLLET ;
			epoll_ctl( forward_epoll_fd , EPOLL_CTL_MOD , p_forward_session->p_reverse_forward_session->sock , & event );
		}
		else
		{
			epoll_ctl( forward_epoll_fd , EPOLL_CTL_DEL , p_forward_session->sock , NULL );
			epoll_ctl( forward_epoll_fd , EPOLL_CTL_DEL , p_forward_session->p_reverse_forward_session->sock , NULL );
			SetForwardSessionUnused( p_forward_session );
			SetForwardSessionUnused( p_forward_session->p_reverse_forward_session );
		}
	}
	else if( len == p_forward_session->io_buffer_len )
	{
		InfoLog( __FILE__ , __LINE__ , "transfer #%d# [%d]bytes to #%d#" , p_forward_session->sock , len , p_forward_session->p_reverse_forward_session->sock );
		p_forward_session->io_buffer_len = 0 ;
	}
	else
	{
		InfoLog( __FILE__ , __LINE__ , "transfer #%d# [%d/%d]bytes to #%d#" , p_forward_session->sock , len , p_forward_session->io_buffer_len , p_forward_session->p_reverse_forward_session->sock );
		
		IgnoreReverseSessionEvents( p_forward_session , p_events , event_count );
		
		p_forward_session->io_buffer_len -= len ;
		p_forward_session->io_buffer_offsetpos = len ;
		
		memset( & event , 0x00 , sizeof(struct epoll_event) );
		event.data.ptr = p_forward_session ;
		event.events = EPOLLERR | EPOLLET ;
		epoll_ctl( forward_epoll_fd , EPOLL_CTL_MOD , p_forward_session->sock , & event );
		
		memset( & event , 0x00 , sizeof(struct epoll_event) );
		event.data.ptr = p_forward_session->p_reverse_forward_session ;
		event.events = EPOLLOUT | EPOLLERR | EPOLLET ;
		epoll_ctl( forward_epoll_fd , EPOLL_CTL_MOD , p_forward_session->p_reverse_forward_session->sock , & event );
	}
	
	return 0;
}

static int OnForwardOutput( struct ServerEnv *penv , struct ForwardSession *p_forward_session , int forward_epoll_fd , struct epoll_event *p_events , int event_count )
{
	int			len ;
	struct epoll_event	event ;
	
	len = send( p_forward_session->p_reverse_forward_session->sock , p_forward_session->io_buffer + p_forward_session->io_buffer_offsetpos , p_forward_session->io_buffer_len , 0 ) ;
	if( len == -1 )
	{
		if( _ERRNO == _EWOULDBLOCK )
		{
			p_forward_session->io_buffer_len -= len ;
			p_forward_session->io_buffer_offsetpos = len ;
		}
		else
		{
			epoll_ctl( forward_epoll_fd , EPOLL_CTL_DEL , p_forward_session->sock , NULL );
			epoll_ctl( forward_epoll_fd , EPOLL_CTL_DEL , p_forward_session->p_reverse_forward_session->sock , NULL );
			SetForwardSessionUnused( p_forward_session );
			SetForwardSessionUnused( p_forward_session->p_reverse_forward_session );
		}
	}
	else if( len == p_forward_session->io_buffer_len )
	{
		InfoLog( __FILE__ , __LINE__ , "transfer #%d# [%d]bytes to #%d#" , p_forward_session->sock , len , p_forward_session->p_reverse_forward_session->sock );
		
		p_forward_session->io_buffer_len = 0 ;
		
		memset( & event , 0x00 , sizeof(struct epoll_event) );
		event.data.ptr = p_forward_session ;
		event.events = EPOLLIN | EPOLLERR | EPOLLET ;
		epoll_ctl( forward_epoll_fd , EPOLL_CTL_MOD , p_forward_session->sock , & event );
		
		memset( & event , 0x00 , sizeof(struct epoll_event) );
		event.data.ptr = p_forward_session->p_reverse_forward_session ;
		event.events = EPOLLIN | EPOLLERR | EPOLLET ;
		epoll_ctl( forward_epoll_fd , EPOLL_CTL_MOD , p_forward_session->p_reverse_forward_session->sock , & event );
	}
	else
	{
		InfoLog( __FILE__ , __LINE__ , "transfer #%d# [%d/%d]bytes to #%d#" , p_forward_session->sock , len , p_forward_session->io_buffer_len , p_forward_session->p_reverse_forward_session->sock );
		
		p_forward_session->io_buffer_len -= len ;
		p_forward_session->io_buffer_offsetpos = len ;
	}
	
	return 0;
}

static void *ForwardThread( struct ServerEnv *penv )
{
	int			forward_thread_index = *(penv->forward_thread_index) ;
	int			forward_epoll_fd = penv->forward_epoll_fd_array[forward_thread_index] ;
	
	struct epoll_event	events[ WAIT_EVENTS_COUNT ] ;
	int			event_count ;
	int			event_index ;
	struct epoll_event	*p_event = NULL ;
	
	char			command ;
	
	struct ForwardSession	*p_forward_session = NULL ;
	
	int			nret = 0 ;
	
	free( penv->forward_thread_index );
	penv->forward_thread_index = NULL ;
	
	event_count = epoll_wait( forward_epoll_fd , events , sizeof(events)/sizeof(events[0]) , -1 ) ;
	for( event_index = 0 , p_event = events ; event_index < event_count ; event_index++ , p_event++ )
	{
		if( p_event->data.ptr == NULL ) /* 命令管道事件 */
		{
			if( p_event->events & EPOLLIN )
			{
				read( penv->forward_pipe_array[forward_thread_index].fds[0] , & command , 1 );
				InfoLog( __FILE__ , __LINE__ , "read command pipe[%c]" , command );
			}
			else if( p_event->events & EPOLLERR )
			{
				ErrorLog( __FILE__ , __LINE__ , "command pipe EPOLLERR" );
			}
			else if( p_event->events & EPOLLHUP )
			{
				ErrorLog( __FILE__ , __LINE__ , "command pipe EPOLLHUP" );
			}
			else
			{
				ErrorLog( __FILE__ , __LINE__ , "command pipe [%d]" , p_event->events );
			}
		}
		else
		{
			p_forward_session = p_event->data.ptr ;
			
			if( p_event->events & EPOLLIN ) /* 输入事件 */
			{
				nret = OnForwardInput( penv , p_forward_session , forward_epoll_fd , events , event_count ) ;
				if( nret )
				{
					ErrorLog( __FILE__ , __LINE__ , "OnForwardInput failed[%d]" , nret );
				}
				else
				{
					DebugLog( __FILE__ , __LINE__ , "OnForwardInput ok" );
				}
			}
			else if( p_event->events & EPOLLOUT ) /* 输出事件 */
			{
				nret = OnForwardOutput( penv , p_forward_session , forward_epoll_fd , events , event_count ) ;
				if( nret )
				{
					ErrorLog( __FILE__ , __LINE__ , "OnForwardOutput failed[%d]" , nret );
				}
				else
				{
					DebugLog( __FILE__ , __LINE__ , "OnForwardOutput ok" );
				}
			}
			else if( p_event->events & EPOLLERR )
			{
				ErrorLog( __FILE__ , __LINE__ , "accept pipe EPOLLERR" );
			}
			else if( p_event->events & EPOLLHUP )
			{
				ErrorLog( __FILE__ , __LINE__ , "accept pipe EPOLLHUP" );
			}
			else
			{
				ErrorLog( __FILE__ , __LINE__ , "accept pipe [%d]" , p_event->events );
			}
		}
	}
	
	return NULL;
}

void *_ForwardThread( void *pv )
{
	return ForwardThread( (struct ServerEnv *)pv );
}

