#include "G6.h"

static void *AcceptThread( struct ServerEnv *penv )
{
	int			thread_index = *(penv->thread_index) ;
	struct epoll_event	events[ WAIT_EVENTS_COUNT ] ;
	int			event_count ;
	struct epoll_event	*p_event = NULL ;
	int			event_index ;
	
	char			command ;
	
	struct ForwardRule	*p_forward_rule = NULL ;
	
	free( penv->thread_index );
	
	event_count = epoll_wait( penv->accept_epoll_fd , penv->events , sizeof(events)/sizeof(events[0]) , -1 ) ;
	if( event_index = 0 , p_event = events ; event_index < event_count ; event_index++ , p_event++ )
	{
		if( p_event->events.data.ptr == NULL ) /* 命令管道事件 */
		{
			if( p_event->events & EPOLLIN )
			{
				read( penv->accept_pipe[thread_index].fds[0] , & command , 1 );
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
				p_forward_rule = p_event->events.data.ptr ;
				
				sock = accept( p_forward_rule->listen_addr.sock , (struct sockaddr *) & (client_addr.netaddr.sockaddr) , & addr_len ) ;
				
				
				InfoLog( __FILE__ , __LINE__ , "" );
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

