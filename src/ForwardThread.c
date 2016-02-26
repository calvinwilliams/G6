#include "G6.h"

static void IgnoreReverseSessionEvents( struct ForwardSession *p_forward_session , struct epoll_event *p_events , int event_index , int event_count )
{
	struct epoll_event	*p_event = NULL ;
	
	for( ++event_index , p_event = p_events + event_index ; event_index < event_count ; event_index++ , p_event++ )
	{
		if( p_event->data.ptr == p_forward_session )
			continue;
		
		if( p_event->data.ptr == p_forward_session->p_reverse_forward_session )
			p_event->events = 0 ;
	}
	
	return;
}

static int OnForwardInput( struct ServerEnv *penv , struct ForwardSession *p_forward_session , int forward_epoll_fd , struct epoll_event *p_events , int event_index , int event_count )
{
	int			len ;
	struct epoll_event	event ;
	
	/* 接收通讯数据 */
	p_forward_session->io_buffer_offsetpos = 0 ;
	p_forward_session->io_buffer_len = recv( p_forward_session->sock , p_forward_session->io_buffer , IO_BUFFER_SIZE , 0 ) ;
	if( p_forward_session->io_buffer_len == 0 )
	{
		/* 对端断开连接 */
		if( p_forward_session->type == FORWARD_SESSION_TYPE_SERVER )
			p_forward_session->p_reverse_forward_session->p_forward_rule->servers_addr[p_forward_session->p_reverse_forward_session->server_index].server_connection_count--;
		else if( p_forward_session->type == FORWARD_SESSION_TYPE_CLIENT )
			p_forward_session->p_forward_rule->servers_addr[p_forward_session->server_index].server_connection_count--;
		InfoLog( __FILE__ , __LINE__ , "recv #%d# closed" , p_forward_session->sock );
		epoll_ctl( forward_epoll_fd , EPOLL_CTL_DEL , p_forward_session->sock , NULL );
		epoll_ctl( forward_epoll_fd , EPOLL_CTL_DEL , p_forward_session->p_reverse_forward_session->sock , NULL );
		DebugLog( __FILE__ , __LINE__ , "close #%d# #%d#" , p_forward_session->sock , p_forward_session->p_reverse_forward_session->sock );
		_CLOSESOCKET2( p_forward_session->sock , p_forward_session->p_reverse_forward_session->sock );
		SetForwardSessionUnused2( p_forward_session , p_forward_session->p_reverse_forward_session );
		return 0;
	}
	else if( p_forward_session->io_buffer_len == -1 )
	{
		/* 通讯接收出错 */
		if( p_forward_session->type == FORWARD_SESSION_TYPE_SERVER )
			p_forward_session->p_reverse_forward_session->p_forward_rule->servers_addr[p_forward_session->p_reverse_forward_session->server_index].server_connection_count--;
		else if( p_forward_session->type == FORWARD_SESSION_TYPE_CLIENT )
			p_forward_session->p_forward_rule->servers_addr[p_forward_session->server_index].server_connection_count--;
		ErrorLog( __FILE__ , __LINE__ , "recv #%d# failed , errno[%d]" , p_forward_session->sock , errno );
		IgnoreReverseSessionEvents( p_forward_session , p_events , event_index , event_count );
		epoll_ctl( forward_epoll_fd , EPOLL_CTL_DEL , p_forward_session->sock , NULL );
		epoll_ctl( forward_epoll_fd , EPOLL_CTL_DEL , p_forward_session->p_reverse_forward_session->sock , NULL );
		DebugLog( __FILE__ , __LINE__ , "close #%d# #%d#" , p_forward_session->sock , p_forward_session->p_reverse_forward_session->sock );
		_CLOSESOCKET2( p_forward_session->sock , p_forward_session->p_reverse_forward_session->sock );
		SetForwardSessionUnused2( p_forward_session , p_forward_session->p_reverse_forward_session );
		return 0;
	}
	
	/* 发送通讯数据 */
	len = send( p_forward_session->p_reverse_forward_session->sock , p_forward_session->io_buffer , p_forward_session->io_buffer_len , 0 ) ;
	if( len == -1 )
	{
		if( _ERRNO == _EWOULDBLOCK )
		{
			/* 通讯发送缓冲区满 */
			IgnoreReverseSessionEvents( p_forward_session , p_events , event_index , event_count );
			
			memset( & event , 0x00 , sizeof(struct epoll_event) );
			event.data.ptr = p_forward_session ;
			event.events = EPOLLERR ;
			epoll_ctl( forward_epoll_fd , EPOLL_CTL_MOD , p_forward_session->sock , & event );
			
			memset( & event , 0x00 , sizeof(struct epoll_event) );
			event.data.ptr = p_forward_session->p_reverse_forward_session ;
			event.events = EPOLLOUT | EPOLLERR ;
			epoll_ctl( forward_epoll_fd , EPOLL_CTL_MOD , p_forward_session->p_reverse_forward_session->sock , & event );
		}
		else
		{
			/* 通讯发送出错 */
			if( p_forward_session->type == FORWARD_SESSION_TYPE_SERVER )
				p_forward_session->p_reverse_forward_session->p_forward_rule->servers_addr[p_forward_session->p_reverse_forward_session->server_index].server_connection_count--;
			else if( p_forward_session->type == FORWARD_SESSION_TYPE_CLIENT )
				p_forward_session->p_forward_rule->servers_addr[p_forward_session->server_index].server_connection_count--;
			epoll_ctl( forward_epoll_fd , EPOLL_CTL_DEL , p_forward_session->sock , NULL );
			epoll_ctl( forward_epoll_fd , EPOLL_CTL_DEL , p_forward_session->p_reverse_forward_session->sock , NULL );
			DebugLog( __FILE__ , __LINE__ , "close #%d# #%d#" , p_forward_session->sock , p_forward_session->p_reverse_forward_session->sock );
			_CLOSESOCKET2( p_forward_session->sock , p_forward_session->p_reverse_forward_session->sock );
			SetForwardSessionUnused2( p_forward_session , p_forward_session->p_reverse_forward_session );
			return 0;
		}
	}
	else if( len == p_forward_session->io_buffer_len )
	{
		/* 一次全发完 */
		InfoLog( __FILE__ , __LINE__ , "transfer #%d# [%d]bytes to #%d#" , p_forward_session->sock , len , p_forward_session->p_reverse_forward_session->sock );
		p_forward_session->io_buffer_len = 0 ;
	}
	else
	{
		/* 只发送了部分 */
		InfoLog( __FILE__ , __LINE__ , "transfer #%d# [%d/%d]bytes to #%d#" , p_forward_session->sock , len , p_forward_session->io_buffer_len , p_forward_session->p_reverse_forward_session->sock );
		
		IgnoreReverseSessionEvents( p_forward_session , p_events , event_index , event_count );
		
		p_forward_session->io_buffer_len -= len ;
		p_forward_session->io_buffer_offsetpos = len ;
		
		memset( & event , 0x00 , sizeof(struct epoll_event) );
		event.data.ptr = p_forward_session ;
		event.events = EPOLLERR ;
		epoll_ctl( forward_epoll_fd , EPOLL_CTL_MOD , p_forward_session->sock , & event );
		
		memset( & event , 0x00 , sizeof(struct epoll_event) );
		event.data.ptr = p_forward_session->p_reverse_forward_session ;
		event.events = EPOLLOUT | EPOLLERR ;
		epoll_ctl( forward_epoll_fd , EPOLL_CTL_MOD , p_forward_session->p_reverse_forward_session->sock , & event );
	}
	
	return 0;
}

static int OnForwardOutput( struct ServerEnv *penv , struct ForwardSession *p_forward_session , int forward_epoll_fd , struct epoll_event *p_events , int event_index , int event_count )
{
	int			len ;
	struct epoll_event	event ;
	
	/* 继续发送通讯数据 */
	len = send( p_forward_session->p_reverse_forward_session->sock , p_forward_session->io_buffer + p_forward_session->io_buffer_offsetpos , p_forward_session->io_buffer_len , 0 ) ;
	if( len == -1 )
	{
		if( _ERRNO == _EWOULDBLOCK )
		{
			/* 通讯发送缓冲区满 */
		}
		else
		{
			/* 通讯发送出错 */
			if( p_forward_session->type == FORWARD_SESSION_TYPE_SERVER )
				p_forward_session->p_reverse_forward_session->p_forward_rule->servers_addr[p_forward_session->p_reverse_forward_session->server_index].server_connection_count--;
			else if( p_forward_session->type == FORWARD_SESSION_TYPE_CLIENT )
				p_forward_session->p_forward_rule->servers_addr[p_forward_session->server_index].server_connection_count--;
			epoll_ctl( forward_epoll_fd , EPOLL_CTL_DEL , p_forward_session->sock , NULL );
			epoll_ctl( forward_epoll_fd , EPOLL_CTL_DEL , p_forward_session->p_reverse_forward_session->sock , NULL );
			DebugLog( __FILE__ , __LINE__ , "close #%d# #%d#" , p_forward_session->sock , p_forward_session->p_reverse_forward_session->sock );
			_CLOSESOCKET2( p_forward_session->sock , p_forward_session->p_reverse_forward_session->sock );
			SetForwardSessionUnused2( p_forward_session , p_forward_session->p_reverse_forward_session );
			return 0;
		}
	}
	else if( len == p_forward_session->io_buffer_len )
	{
		/* 一次全发完 */
		InfoLog( __FILE__ , __LINE__ , "transfer #%d# [%d]bytes to #%d#" , p_forward_session->sock , len , p_forward_session->p_reverse_forward_session->sock );
		
		p_forward_session->io_buffer_len = 0 ;
		
		memset( & event , 0x00 , sizeof(struct epoll_event) );
		event.data.ptr = p_forward_session ;
		event.events = EPOLLIN | EPOLLERR ;
		epoll_ctl( forward_epoll_fd , EPOLL_CTL_MOD , p_forward_session->sock , & event );
		
		memset( & event , 0x00 , sizeof(struct epoll_event) );
		event.data.ptr = p_forward_session->p_reverse_forward_session ;
		event.events = EPOLLIN | EPOLLERR ;
		epoll_ctl( forward_epoll_fd , EPOLL_CTL_MOD , p_forward_session->p_reverse_forward_session->sock , & event );
	}
	else
	{
		/* 只发送了部分 */
		InfoLog( __FILE__ , __LINE__ , "transfer #%d# [%d/%d]bytes to #%d#" , p_forward_session->sock , len , p_forward_session->io_buffer_len , p_forward_session->p_reverse_forward_session->sock );
		
		p_forward_session->io_buffer_len -= len ;
		p_forward_session->io_buffer_offsetpos = len ;
	}
	
	return 0;
}

void *ForwardThread( struct ServerEnv *penv )
{
	int			forward_thread_index ;
	int			forward_epoll_fd ;
	
	struct epoll_event	events[ WAIT_EVENTS_COUNT ] ;
	int			event_count ;
	int			event_index ;
	struct epoll_event	*p_event = NULL ;
	
	char			command ;
	
	struct ForwardSession	*p_forward_session = NULL ;
	
	int			nret = 0 ;
	
	forward_thread_index = *(penv->forward_thread_index) ;
	forward_epoll_fd = penv->forward_epoll_fd_array[forward_thread_index] ;
	{ int *tmp = penv->forward_thread_index ; penv->forward_thread_index = NULL ; free( tmp ); }
	
	/* 设置日志输出文件 */
	SetLogFile( "%s/log/G6.log" , getenv("HOME") );
	SetLogLevel( penv->cmd_para.log_level );
	InfoLog( __FILE__ , __LINE__ , "--- G6.WorkerProcess.ForwardThread.%d ---" , forward_thread_index+1 );
	
	/* 主工作循环 */
	while(1)
	{
		event_count = epoll_wait( forward_epoll_fd , events , WAIT_EVENTS_COUNT , -1 ) ;
		DebugLog( __FILE__ , __LINE__ , "epoll_wait return [%d]events" , event_count );
		for( event_index = 0 , p_event = events ; event_index < event_count ; event_index++ , p_event++ )
		{
			if( p_event->data.ptr == NULL ) /* 命令管道事件 */
			{
				DebugLog( __FILE__ , __LINE__ , "command pipe event" );
				
				if( p_event->events & EPOLLIN )
				{
					read( penv->forward_pipe_array[forward_thread_index].fds[0] , & command , 1 );
					InfoLog( __FILE__ , __LINE__ , "read command pipe[%c]" , command );
				}
				else if( p_event->events & EPOLLERR )
				{
					ErrorLog( __FILE__ , __LINE__ , "command pipe EPOLLERR" );
					pthread_exit( NULL );
				}
				else if( p_event->events & EPOLLHUP )
				{
					ErrorLog( __FILE__ , __LINE__ , "command pipe EPOLLHUP" );
					pthread_exit( NULL );
				}
				else
				{
					ErrorLog( __FILE__ , __LINE__ , "command pipe [%d]" , p_event->events );
					pthread_exit( NULL );
				}
			}
			else
			{
				DebugLog( __FILE__ , __LINE__ , "forward session event" );
				
				p_forward_session = p_event->data.ptr ;
				
				if( p_event->events & EPOLLIN ) /* 输入事件 */
				{
					nret = OnForwardInput( penv , p_forward_session , forward_epoll_fd , events , event_index , event_count ) ;
					if( nret )
					{
						ErrorLog( __FILE__ , __LINE__ , "OnForwardInput failed[%d]" , nret );
					}
				}
				else if( p_event->events & EPOLLOUT ) /* 输出事件 */
				{
					nret = OnForwardOutput( penv , p_forward_session , forward_epoll_fd , events , event_index , event_count ) ;
					if( nret )
					{
						ErrorLog( __FILE__ , __LINE__ , "OnForwardOutput failed[%d]" , nret );
					}
				}
				else
				{
					if( p_forward_session->type == FORWARD_SESSION_TYPE_SERVER )
						p_forward_session->p_reverse_forward_session->p_forward_rule->servers_addr[p_forward_session->p_reverse_forward_session->server_index].server_connection_count--;
					else if( p_forward_session->type == FORWARD_SESSION_TYPE_CLIENT )
						p_forward_session->p_forward_rule->servers_addr[p_forward_session->server_index].server_connection_count--;
					ErrorLog( __FILE__ , __LINE__ , "forward session EPOLLERR" );
			        	epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_DEL , p_forward_session->sock , NULL );
					DebugLog( __FILE__ , __LINE__ , "close #%d# #%d#" , p_forward_session->sock , p_forward_session->p_reverse_forward_session->sock );
					_CLOSESOCKET2( p_forward_session->sock , p_forward_session->p_reverse_forward_session->sock );
					SetForwardSessionUnused2( p_forward_session , p_forward_session->p_reverse_forward_session );
				}
			}
		}
	}
	
	return NULL;
}

void *_ForwardThread( void *pv )
{
	return ForwardThread( (struct ServerEnv *)pv );
}

