#include "G6.h"

int InitEnvirment( struct ServerEnv *penv )
{
	int			forward_thread_index ;
	
	struct epoll_event	event ;
	
	int			nret = 0 ;
	
	penv->forward_thread_tid_array = (pthread_t *)malloc( sizeof(pthread_t) * penv->cmd_para.forward_thread_size ) ;
	if( penv->forward_thread_tid_array == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "malloc failed , errno[%d]" , errno );
		return -1;
	}
	memset( penv->forward_thread_tid_array , 0x00 , sizeof(pthread_t) * penv->cmd_para.forward_thread_size );
	
	nret = pipe( penv->accept_request_pipe.fds ) ;
	if( nret == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "pipe failed , errno[%d]" , errno );
		return -1;
	}
	SetCloseExec2( penv->accept_request_pipe.fds[0] , penv->accept_request_pipe.fds[1] );
	
	nret = pipe( penv->accept_response_pipe.fds ) ;
	if( nret == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "pipe failed , errno[%d]" , errno );
		return -1;
	}
	SetCloseExec2( penv->accept_response_pipe.fds[0] , penv->accept_response_pipe.fds[1] );
	
	penv->accept_epoll_fd = epoll_create( 1024 );
	if( penv->accept_epoll_fd == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "epoll_create failed , errno[%d]" , errno );
		return -1;
	}
	SetCloseExec( penv->accept_epoll_fd );
	
	memset( & event , 0x00 , sizeof(event) );
	event.data.ptr = NULL ;
	event.events = EPOLLIN | EPOLLERR ;
	epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_ADD , penv->accept_request_pipe.fds[0] , & event );
	
	penv->forward_request_pipe = (struct PipeFds *)malloc( sizeof(struct PipeFds) * penv->cmd_para.forward_thread_size ) ;
	if( penv->forward_request_pipe == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "malloc failed , errno[%d]" , errno );
		return -1;
	}
	memset( penv->forward_request_pipe , 0x00 , sizeof(struct PipeFds) * penv->cmd_para.forward_thread_size );
	
	penv->forward_response_pipe = (struct PipeFds *)malloc( sizeof(struct PipeFds) * penv->cmd_para.forward_thread_size ) ;
	if( penv->forward_response_pipe == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "malloc failed , errno[%d]" , errno );
		return -1;
	}
	memset( penv->forward_response_pipe , 0x00 , sizeof(struct PipeFds) * penv->cmd_para.forward_thread_size );
	
	penv->forward_epoll_fd_array = (int *)malloc( sizeof(int) * penv->cmd_para.forward_thread_size ) ;
	if( penv->forward_epoll_fd_array == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "malloc failed , errno[%d]" , errno );
		return -1;
	}
	memset( penv->forward_epoll_fd_array , 0x00 , sizeof(int) * penv->cmd_para.forward_thread_size );
	
	for( forward_thread_index = 0 ; forward_thread_index < penv->cmd_para.forward_thread_size ; forward_thread_index++ )
	{
		nret = pipe( penv->forward_request_pipe[forward_thread_index].fds ) ;
		if( nret == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "pipe failed , errno[%d]" , errno );
			return -1;
		}
		SetCloseExec2( penv->forward_request_pipe[forward_thread_index].fds[0] , penv->forward_request_pipe[forward_thread_index].fds[1] );
		
		nret = pipe( penv->forward_response_pipe[forward_thread_index].fds ) ;
		if( nret == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "pipe failed , errno[%d]" , errno );
			return -1;
		}
		SetCloseExec2( penv->forward_response_pipe[forward_thread_index].fds[0] , penv->forward_response_pipe[forward_thread_index].fds[1] );
		
		penv->forward_epoll_fd_array[forward_thread_index] = epoll_create( 1024 );
		if( penv->forward_epoll_fd_array[forward_thread_index] == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "epoll_create failed , errno[%d]" , errno );
			return -1;
		}
		SetCloseExec( penv->forward_epoll_fd_array[forward_thread_index] );
		
		memset( & event , 0x00 , sizeof(event) );
		event.data.ptr = NULL ;
		event.events = EPOLLIN | EPOLLERR ;
		epoll_ctl( penv->forward_epoll_fd_array[forward_thread_index] , EPOLL_CTL_ADD , penv->forward_request_pipe[forward_thread_index].fds[0] , & event );
	}
	
	penv->forward_session_array = (struct ForwardSession *)malloc( sizeof(struct ForwardSession) * penv->cmd_para.forward_session_size ) ;
	if( penv->forward_session_array == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "malloc failed , errno[%d]" , errno );
		return -1;
	}
	memset( penv->forward_session_array , 0x00 , sizeof(struct ForwardSession) * penv->cmd_para.forward_session_size );
	
	pthread_mutex_init( & (penv->forward_session_count_mutex) , NULL );
	pthread_mutex_init( & (penv->server_connection_count_mutex) , NULL );
	
	return 0;
}

void CleanEnvirment( struct ServerEnv *penv )
{
	int				forward_session_index ;
	struct ForwardSession		*p_forward_session = NULL ;
	
	int				forward_thread_index ;
	
	if( penv->old_forward_addr_array )
	{
		free( penv->old_forward_addr_array );
		penv->old_forward_addr_array = NULL ;
	}
	
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
	
	close( penv->accept_request_pipe.fds[0] );
	close( penv->accept_request_pipe.fds[1] );
	close( penv->accept_response_pipe.fds[0] );
	close( penv->accept_response_pipe.fds[1] );
	
	close( penv->accept_epoll_fd );
	free( penv->forward_thread_tid_array );
	free( penv->forward_session_array );
	
	for( forward_thread_index = 0 ; forward_thread_index < penv->cmd_para.forward_thread_size ; forward_thread_index++ )
	{
		close( penv->forward_request_pipe[forward_thread_index].fds[0] );
		close( penv->forward_request_pipe[forward_thread_index].fds[1] );
		close( penv->forward_response_pipe[forward_thread_index].fds[0] );
		close( penv->forward_response_pipe[forward_thread_index].fds[1] );
		close( penv->forward_epoll_fd_array[forward_thread_index] );
	}
	free( penv->forward_request_pipe );
	free( penv->forward_response_pipe );
	free( penv->forward_epoll_fd_array );
	
	pthread_mutex_destroy( & (penv->forward_session_count_mutex) );
	pthread_mutex_destroy( & (penv->server_connection_count_mutex) );
	
	return;
}

int SaveListenSockets( struct ServerEnv *penv )
{
	int			forward_session_index ;
	struct ForwardSession	*p_forward_session = NULL ;
	int			listen_socket_count ;
	char			*env_value = NULL ;
	int			env_value_size ;
	
	listen_socket_count = 0 ;
	for( forward_session_index = 0 , p_forward_session = penv->forward_session_array ; forward_session_index < penv->cmd_para.forward_session_size ; forward_session_index++ , p_forward_session++ )
	{
		if( p_forward_session->type == FORWARD_SESSION_TYPE_LISTEN )
			listen_socket_count++;
	}
	
	env_value_size = (1+listen_socket_count)*(10+1) + 1 ;
	env_value = (char*)malloc( env_value_size ) ;
	if( env_value == NULL )
		return -1;
	memset( env_value , 0x00 , env_value_size );
	
	_SNPRINTF( env_value , env_value_size-1 , "%d|" , listen_socket_count );
	
	for( forward_session_index = 0 , p_forward_session = penv->forward_session_array ; forward_session_index < penv->cmd_para.forward_session_size ; forward_session_index++ , p_forward_session++ )
	{
		if( p_forward_session->type == FORWARD_SESSION_TYPE_LISTEN )
		{
			_SNPRINTF( env_value+strlen(env_value) , env_value_size-1-strlen(env_value) , "%d|" , p_forward_session->sock );
		}
	}
	
	InfoLog( __FILE__ , __LINE__ , "setenv[%s][%s]" , G6_LISTEN_SOCKFDS , env_value );
	setenv( G6_LISTEN_SOCKFDS , env_value , 1 );
	
	free( env_value );
	
	return 0;
}

int LoadOldListenSockets( struct ServerEnv *penv )
{
	char				*p_env_value = NULL ;
	char				*p_sockfd_count = NULL ;
	int				old_forward_addr_index ;
	struct ForwardNetAddress	*p_forward_addr = NULL ;
	char				*p_sockfd = NULL ;
	_SOCKLEN_T			addr_len = sizeof(struct sockaddr) ;
	
	int				nret = 0 ;
	
	p_env_value = getenv( G6_LISTEN_SOCKFDS ) ;
	InfoLog( __FILE__ , __LINE__ , "getenv[%s][%s]" , G6_LISTEN_SOCKFDS , p_env_value );
	if( p_env_value )
	{
		p_env_value = strdup( p_env_value ) ;
		if( p_env_value == NULL )
		{
			ErrorLog( __FILE__ , __LINE__ , "strdup failed , errno[%d]" , errno );
			return -1;
		}
		
		p_sockfd_count = strtok( p_env_value , "|" ) ;
		if( p_sockfd_count )
		{
			penv->old_forward_addr_count = atoi(p_sockfd_count) ;
			if( penv->old_forward_addr_count > 0 )
			{
				penv->old_forward_addr_array = (struct ForwardNetAddress *)malloc( sizeof(struct ForwardNetAddress) * penv->old_forward_addr_count ) ;
				if( penv->old_forward_addr_array == NULL )
				{
					ErrorLog( __FILE__ , __LINE__ , "malloc failed , errno[%d]" , errno );
					free( p_env_value );
					return -1;
				}
				memset( penv->old_forward_addr_array , 0x00 , sizeof(struct ForwardNetAddress) * penv->old_forward_addr_count );
				
				for( old_forward_addr_index = 0 , p_forward_addr = penv->old_forward_addr_array ; old_forward_addr_index < penv->old_forward_addr_count ; old_forward_addr_index++ , p_forward_addr++ )
				{
					p_sockfd = strtok( NULL , "|" ) ;
					if( p_sockfd == NULL )
					{
						ErrorLog( __FILE__ , __LINE__ , "env[%s][%s] invalid" , G6_LISTEN_SOCKFDS , p_env_value );
						free( p_env_value );
						return -1;
					}
					
					p_forward_addr->sock = atoi(p_sockfd) ;
					nret = getsockname( p_forward_addr->sock , (struct sockaddr *) & (p_forward_addr->netaddr.sockaddr) , & addr_len ) ;
					if( nret == -1 )
					{
						ErrorLog( __FILE__ , __LINE__ , "getsockname failed , errno[%d]" , errno );
						free( p_env_value );
						return -1;
					}
					
					GetNetAddress( & (p_forward_addr->netaddr) );
					InfoLog( __FILE__ , __LINE__ , "load [%s:%d]#%d#" , p_forward_addr->netaddr.ip , p_forward_addr->netaddr.port.port_int , p_forward_addr->sock );
				}
			}
		}
		
		free( p_env_value );
	}
	
	return 0;
}

int CleanOldListenSockets( struct ServerEnv *penv )
{
	int				old_forward_addr_index ;
	struct ForwardNetAddress	*p_old_forward_addr = NULL ;
	
	if( penv->old_forward_addr_array == NULL )
		return 0;
	
	for( old_forward_addr_index = 0 , p_old_forward_addr = penv->old_forward_addr_array ; old_forward_addr_index < penv->old_forward_addr_count ; old_forward_addr_index++ , p_old_forward_addr++ )
	{
		if( STRCMP( p_old_forward_addr->netaddr.ip , != , "" ) )
		{
			InfoLog( __FILE__ , __LINE__ , "close old #%d#" , p_old_forward_addr->sock );
			close( p_old_forward_addr->sock );
		}
	}
	
	free( penv->old_forward_addr_array ); penv->old_forward_addr_array = NULL ;
	
	return 0;
}

int AddListeners( struct ServerEnv *penv )
{
	int				forward_rule_index ;
	struct ForwardRule		*p_forward_rule = NULL ;
	int				forward_addr_index ;
	struct ForwardNetAddress	*p_forward_addr = NULL ;
	
	int				old_forward_addr_index ;
	struct ForwardNetAddress	*p_old_forward_addr = NULL ;
	
	struct ForwardSession		*p_forward_session = NULL ;
	struct epoll_event		event ;
	
	int				nret = 0 ;
	
	nret = LoadOldListenSockets( penv ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "LoadListenSockets failed[%d]" , nret );
		return -1;
	}
	
	for( forward_rule_index = 0 ; forward_rule_index < penv->forward_rule_count ; forward_rule_index++ )
	{
		p_forward_rule = penv->forward_rule_array + forward_rule_index ;
		
		for( forward_addr_index = 0 , p_forward_addr = p_forward_rule->forward_addr_array ; forward_addr_index < p_forward_rule->forward_addr_count ; forward_addr_index++ , p_forward_addr++ )
		{
			old_forward_addr_index = -1 ;
			
			if( penv->old_forward_addr_array )
			{
				for( old_forward_addr_index = 0 , p_old_forward_addr = penv->old_forward_addr_array ; old_forward_addr_index < penv->old_forward_addr_count ; old_forward_addr_index++ , p_old_forward_addr++ )
				{
					if( STRCMP( p_old_forward_addr->netaddr.ip , == , p_forward_rule->forward_addr_array[forward_addr_index].netaddr.ip ) && p_old_forward_addr->netaddr.port.port_int == p_forward_rule->forward_addr_array[forward_addr_index].netaddr.port.port_int )
					{
						memcpy( p_forward_addr , p_old_forward_addr , sizeof(struct ForwardNetAddress) );
						memset( p_old_forward_addr , 0x00 , sizeof(struct ForwardNetAddress) );
						break;
					}
				}
				if( old_forward_addr_index >= penv->old_forward_addr_count )
					goto _GOTO_CREATE_LISTENER;
			}
			else
			{
_GOTO_CREATE_LISTENER :
				p_forward_addr->sock = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP );
				if( p_forward_addr->sock == -1 )
				{
					ErrorLog( __FILE__ , __LINE__ , "socket failed , errno[%d]" , errno );
					return -1;
				}
				
				SetNonBlocking( p_forward_addr->sock );
				SetReuseAddr( p_forward_addr->sock );
				
				nret = bind( p_forward_addr->sock , (struct sockaddr *) & (p_forward_addr->netaddr.sockaddr) , sizeof(struct sockaddr) ) ;
				if( nret )
				{
					ErrorLog( __FILE__ , __LINE__ , "bind[%s:%d] failed , errno[%d]" , p_forward_addr->netaddr.ip , p_forward_addr->netaddr.port.port_int , _ERRNO );
					return -1;
				}
				
				nret = listen( p_forward_addr->sock , 1024 ) ;
				if( nret )
				{
					ErrorLog( __FILE__ , __LINE__ , "listen[%s:%d] failed , errno[%d]" , p_forward_addr->netaddr.ip , p_forward_addr->netaddr.port.port_int , _ERRNO );
					return -1;
				}
			}
			
			p_forward_session = GetForwardSessionUnused( penv ) ;
			if( p_forward_session == NULL )
			{
				ErrorLog( __FILE__ , __LINE__ , "GetForwardSessionUnused failed" );
				return -1;
			}
			
			p_forward_session->sock = p_forward_addr->sock ;
			p_forward_session->p_forward_rule = p_forward_rule ;
			p_forward_session->status = FORWARD_SESSION_STATUS_LISTEN ;
			p_forward_session->type = FORWARD_SESSION_TYPE_LISTEN ;
			p_forward_session->forward_index = forward_addr_index ;
			
			memset( & event , 0x00 , sizeof(event) );
			event.data.ptr = p_forward_session ;
			event.events = EPOLLIN | EPOLLERR | EPOLLET ;
			epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_ADD , p_forward_addr->sock , & event );
			
			if( penv->old_forward_addr_array && old_forward_addr_index < penv->old_forward_addr_count )
			{
				InfoLog( __FILE__ , __LINE__ , "reuse listener sock[%s:%d][%d]" , p_forward_addr->netaddr.ip , p_forward_addr->netaddr.port.port_int , p_forward_addr->sock );
			}
			else
			{
				InfoLog( __FILE__ , __LINE__ , "add listener sock[%s:%d][%d]" , p_forward_addr->netaddr.ip , p_forward_addr->netaddr.port.port_int , p_forward_addr->sock );
			}
		}
	}
	
	nret = CleanOldListenSockets( penv ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "CleanOldListenSockets failed[%d]" , nret );
		return -1;
	}
	
	return 0;
}

struct ForwardSession *GetForwardSessionUnused( struct ServerEnv *penv )
{
	int			n ;
	struct ForwardSession	*p_forward_session = penv->forward_session_array + penv->forward_session_use_offsetpos ;
	
	for( n = 0 ; n < penv->cmd_para.forward_session_size ; n++ )
	{
		if( IsForwardSessionUsed( p_forward_session ) == 0 )
		{
			memset( p_forward_session , 0x00 , sizeof(struct ForwardSession) );
			p_forward_session->status = FORWARD_SESSION_STATUS_READY ;
			penv->forward_session_use_offsetpos++;
			if( penv->forward_session_use_offsetpos >= penv->cmd_para.forward_session_size )
			{
				penv->forward_session_use_offsetpos = 0 ;
			}
			
			pthread_mutex_lock( & (penv->forward_session_count_mutex) );
			penv->forward_session_count++;
			pthread_mutex_unlock( & (penv->forward_session_count_mutex) );
			
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
	
	pthread_mutex_lock( & (penv->forward_session_count_mutex) );
	penv->forward_session_count--;
	pthread_mutex_unlock( & (penv->forward_session_count_mutex) );
	
	return;
}

void SetForwardSessionUnused2( struct ServerEnv *penv , struct ForwardSession *p_forward_session , struct ForwardSession *p_forward_session2 )
{
	if( p_forward_session->status != FORWARD_SESSION_STATUS_UNUSED )
		p_forward_session->status = FORWARD_SESSION_STATUS_UNUSED ;
	
	if( p_forward_session2->status != FORWARD_SESSION_STATUS_UNUSED )
		p_forward_session2->status = FORWARD_SESSION_STATUS_UNUSED ;
	
	pthread_mutex_lock( & (penv->forward_session_count_mutex) );
	penv->forward_session_count -= 2 ;
	pthread_mutex_unlock( & (penv->forward_session_count_mutex) );
	
	return;
}
