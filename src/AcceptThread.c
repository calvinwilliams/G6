#include "G6.h"

int TryToConnectServer( struct ServerEnv *penv , struct ForwardSession *p_forward_session )
{
	struct ForwardSession	*p_forward_session = NULL ;
	
	SetNonBlocking( sock );
	SetReuseAddr( sock );
	
	
	
	
	
	
	
	return 0;
}

static int OnListenAccept( struct ServerEnv *penv , struct ForwardSession *p_listen_session )
{
	struct ForwardSession	*p_forward_session = NULL ;
	struct ForwardSession	*p_reverse_forward_session = NULL ;
	_SOCKLEN_T		addr_len = sizeof(struct sockaddr_in) ;
	
	p_forward_session = GetForwardSessionUnused( penv ) ;
	if( p_forward_session == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "GetForwardSessionUnused failed[%d]" , nret );
		return -1;
	}
	
	p_reverse_forward_session = GetForwardSessionUnused( penv ) ;
	if( p_reverse_forward_session == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "GetForwardSessionUnused failed[%d]" , nret );
		return -1;
	}
	
	p_forward_session->p_forward_rule = p_listen_session->p_forward_rule ;
	p_forward_session->p_reverse_forward_session = p_reverse_forward_session ;
	
	p_reverse_forward_session->p_forward_rule = p_listen_session->p_forward_rule ;
	p_reverse_forward_session->p_reverse_forward_session = p_forward_session ;
	
	
	
	
	
	
	
	
	
	
	
	p_forward_session->sock = accept( p_listen_session->sock , (struct sockaddr *) & (p_forward_session->netaddr) , & addr_len ) ;
	if( p_forward_session->sock == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "accept failed , errno[%d]" , errno );
		SetForwardSessionUnused( p_forward_session );
		SetForwardSessionUnused( p_reverse_forward_session );
		return -1;
	}
	
	nret = TryToConnectServer( penv , p_reverse_forward_session ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "TryToConnectServer failed[%d]" , nret );
		SetForwardSessionUnused( p_forward_session );
		return -1;
	}
	
	return 0;
}

static void *AcceptThread( struct ServerEnv *penv )
{
	int			forward_thread_index = *(penv->forward_thread_index) ;
	struct epoll_event	events[ WAIT_EVENTS_COUNT ] ;
	int			event_count ;
	struct epoll_event	*p_event = NULL ;
	int			event_index ;
	
	char			command ;
	
	struct ForwardSession	*p_forward_session = NULL ;
	
	free( penv->forward_thread_index );
	
	event_count = epoll_wait( penv->accept_epoll_fd , events , sizeof(events)/sizeof(events[0]) , -1 ) ;
	for( event_index = 0 , p_event = events ; event_index < event_count ; event_index++ , p_event++ )
	{
		if( p_event->data.ptr == NULL ) /* 命令管道事件 */
		{
			if( p_event->events & EPOLLIN )
			{
				read( penv->accept_pipe_array[forward_thread_index].fds[0] , & command , 1 );
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
		else /* 侦听端口事件 */
		{
			if( p_event->events & EPOLLIN )
			{
				p_forward_session = p_event->data.ptr ;
				
				nret = OnListenAccept( penv , p_forward_session ) ;
				if( nret )
				{
					ErrorLog( __FILE__ , __LINE__ , "OnListenAccept failed[%d]" , nret );
				}
				else
				{
					DebugLog( __FILE__ , __LINE__ , "OnListenAccept ok" );
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
	
	return 0;
}

void *_AcceptThread( void *pv )
{
	return AcceptThread( (struct ServerEnv *)pv );
}

