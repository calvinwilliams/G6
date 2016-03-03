#include "G6.h"

int InitEnvirment( struct ServerEnv *penv )
{
	int		n ;
	
	penv->accept_epoll_fd = epoll_create( 1024 );
	if( penv->accept_epoll_fd == -1 )
	{
		printf( "epoll_create failed , errno[%d]" , errno );
		return -1;
	}
	
	penv->forward_thread_tid_array = (pthread_t *)malloc( sizeof(pthread_t) * penv->cmd_para.forward_thread_size ) ;
	if( penv->forward_thread_tid_array == NULL )
	{
		printf( "malloc failed , errno[%d]" , errno );
		return -1;
	}
	memset( penv->forward_thread_tid_array , 0x00 , sizeof(pthread_t) * penv->cmd_para.forward_thread_size );
	
	penv->forward_epoll_fd_array = (int *)malloc( sizeof(int) * penv->cmd_para.forward_thread_size ) ;
	if( penv->forward_epoll_fd_array == NULL )
	{
		printf( "malloc failed , errno[%d]" , errno );
		return -1;
	}
	memset( penv->forward_epoll_fd_array , 0x00 , sizeof(int) * penv->cmd_para.forward_thread_size );
	
	for( n = 0 ; n < penv->cmd_para.forward_thread_size ; n++ )
	{
		penv->forward_epoll_fd_array[n] = epoll_create( 1024 );
		if( penv->forward_epoll_fd_array[n] == -1 )
		{
			printf( "epoll_create failed , errno[%d]" , errno );
			return -1;
		}
	}
	
	penv->forward_session_array = (struct ForwardSession *)malloc( sizeof(struct ForwardSession) * penv->cmd_para.forward_session_size ) ;
	if( penv->forward_session_array == NULL )
	{
		printf( "malloc failed , errno[%d]" , errno );
		return -1;
	}
	memset( penv->forward_session_array , 0x00 , sizeof(struct ForwardSession) * penv->cmd_para.forward_session_size );
	
	pthread_mutex_init( & (penv->mutex) , NULL );
	
	return 0;
}

void CleanEnvirment( struct ServerEnv *penv )
{
	int				forward_session_index ;
	struct ForwardSession		*p_forward_session = NULL ;
	
	int				n ;
	
	for( forward_session_index = 0 , p_forward_session = penv->forward_session_array ; forward_session_index < penv->cmd_para.forward_session_size ; forward_session_index++ , p_forward_session++ )
	{
		if( IsForwardSessionUsed( p_forward_session ) )
		{
			epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_DEL , p_forward_session->sock , NULL );
			DebugLog( __FILE__ , __LINE__ , "close #%d#" , p_forward_session->sock );
			_CLOSESOCKET( p_forward_session->sock );
			SetForwardSessionUnused( penv , p_forward_session );
		}
	}
	
	close( penv->request_pipe.fds[0] );
	close( penv->request_pipe.fds[1] );
	close( penv->response_pipe.fds[0] );
	close( penv->response_pipe.fds[1] );
	
	close( penv->accept_epoll_fd );
	free( penv->forward_thread_tid_array );
	free( penv->forward_session_array );
	
	for( n = 0 ; n < penv->cmd_para.forward_thread_size ; n++ )
	{
		close( penv->forward_epoll_fd_array[n] );
	}
	
	pthread_mutex_destroy( & (penv->mutex) );
	
	return;
}

int AddListeners( struct ServerEnv *penv )
{
	int			m , n ;
	struct ForwardRule	*p_forward_rule = NULL ;
	struct ForwardSession	*p_forward_session = NULL ;
	struct epoll_event	event ;
	
	int			nret = 0 ;
	
	for( m = 0 ; m < penv->forward_rule_count ; m++ )
	{
		p_forward_rule = penv->forward_rule_array + m ;
		
		for( n = 0 ; n < p_forward_rule->forward_addr_count ; n++ )
		{
			p_forward_rule->forward_addr_array[n].sock = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP );
			if( p_forward_rule->forward_addr_array[n].sock == -1 )
			{
				ErrorLog( __FILE__ , __LINE__ , "socket failed , errno[%d]" , errno );
				return -1;
			}
			
			SetNonBlocking( p_forward_rule->forward_addr_array[n].sock );
			SetReuseAddr( p_forward_rule->forward_addr_array[n].sock );
			
			nret = bind( p_forward_rule->forward_addr_array[n].sock , (struct sockaddr *) & (p_forward_rule->forward_addr_array[n].netaddr.sockaddr) , sizeof(struct sockaddr) ) ;
			if( nret )
			{
				ErrorLog( __FILE__ , __LINE__ , "bind[%s:%d] failed , errno[%d]" , p_forward_rule->forward_addr_array[n].netaddr.ip , p_forward_rule->forward_addr_array[n].netaddr.port.port_int , _ERRNO );
				return -1;
			}
			
			nret = listen( p_forward_rule->forward_addr_array[n].sock , 1024 ) ;
			if( nret )
			{
				ErrorLog( __FILE__ , __LINE__ , "listen[%s:%d] failed , errno[%d]" , p_forward_rule->forward_addr_array[n].netaddr.ip , p_forward_rule->forward_addr_array[n].netaddr.port.port_int , _ERRNO );
				return -1;
			}
			
			p_forward_session = GetForwardSessionUnused( penv ) ;
			if( p_forward_session == NULL )
			{
				ErrorLog( __FILE__ , __LINE__ , "GetForwardSessionUnused failed[%d]" , nret );
				return -1;
			}
			
			p_forward_session->sock = p_forward_rule->forward_addr_array[n].sock ;
			p_forward_session->p_forward_rule = p_forward_rule ;
			p_forward_session->status = FORWARD_SESSION_STATUS_LISTEN ;
			p_forward_session->type = FORWARD_SESSION_TYPE_LISTEN ;
			p_forward_session->forward_index = n ;
			
			memset( & event , 0x00 , sizeof(event) );
			event.data.ptr = p_forward_session ;
			event.events = EPOLLIN | EPOLLERR | EPOLLET ;
			epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_ADD , p_forward_rule->forward_addr_array[n].sock , & event );
			
			InfoLog( __FILE__ , __LINE__ , "AddListeners sock[%s:%d][%d]" , p_forward_rule->forward_addr_array[n].netaddr.ip , p_forward_rule->forward_addr_array[n].netaddr.port.port_int , p_forward_rule->forward_addr_array[n].sock );
		}
	}
	
	return 0;
}

struct ForwardSession *GetForwardSessionUnused( struct ServerEnv *penv )
{
	int			n ;
	struct ForwardSession	*p_forward_session = penv->forward_session_array + penv->forward_session_use_offsetpos ;
	
	for( n = 0 ; n < penv->cmd_para.forward_session_size ; n++ )
	{
		if( IsForwardSessionUsed( p_forward_session ) )
		{
			memset( p_forward_session , 0x00 , sizeof(struct ForwardSession) );
			p_forward_session->status = FORWARD_SESSION_STATUS_READY ;
			penv->forward_session_use_offsetpos++;
			if( penv->forward_session_use_offsetpos >= penv->cmd_para.forward_session_size )
			{
				penv->forward_session_use_offsetpos = 0 ;
			}
			penv->forward_session_count++;
			return p_forward_session;
		}
		
		penv->forward_session_use_offsetpos++;
		p_forward_session++;
		if( penv->forward_session_use_offsetpos >= penv->cmd_para.forward_session_size )
		{
			penv->forward_session_use_offsetpos = 0 ;
			p_forward_session = penv->forward_session_array ;
		}
	}
	
	return NULL;
}

void SetForwardSessionUnused( struct ServerEnv *penv , struct ForwardSession *p_forward_session )
{
	if( p_forward_session->status != FORWARD_SESSION_STATUS_UNUSED )
		p_forward_session->status = FORWARD_SESSION_STATUS_UNUSED ;
	
	penv->forward_session_count--;
	
	return;
}

void SetForwardSessionUnused2( struct ServerEnv *penv , struct ForwardSession *p_forward_session , struct ForwardSession *p_forward_session2 )
{
	if( p_forward_session->status != FORWARD_SESSION_STATUS_UNUSED )
		p_forward_session->status = FORWARD_SESSION_STATUS_UNUSED ;
	
	if( p_forward_session2->status != FORWARD_SESSION_STATUS_UNUSED )
		p_forward_session2->status = FORWARD_SESSION_STATUS_UNUSED ;
	
	penv->forward_session_count -= 2 ;
	
	return;
}
