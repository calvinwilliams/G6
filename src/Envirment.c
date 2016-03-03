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

int LoadListenSockets( struct ServerEnv *penv )
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

int AddListeners( struct ServerEnv *penv )
{
	int				forward_rule_index ;
	struct ForwardRule		*p_forward_rule = NULL ;
	int				forward_addr_index ;
	struct ForwardNetAddress	*p_forward_addr = NULL ;
	
	struct ForwardSession		*p_forward_session = NULL ;
	struct epoll_event		event ;
	
	int				nret = 0 ;
	
	nret = LoadListenSockets( penv ) ;
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
			if( penv->old_forward_addr_array )
			{
				int				old_forward_addr_index ;
				struct ForwardNetAddress	*p_old_forward_addr = NULL ;
				
				for( old_forward_addr_index = 0 , p_old_forward_addr = penv->old_forward_addr_array ; old_forward_addr_index < penv->old_forward_addr_count ; old_forward_addr_index++ , p_old_forward_addr++ )
				{
					if( STRCMP( p_old_forward_addr->netaddr.ip , == , p_forward_rule->forward_addr_array[forward_addr_index].netaddr.ip ) && p_old_forward_addr->netaddr.port.port_int == p_forward_rule->forward_addr_array[forward_addr_index].netaddr.port.port_int )
					{
						memcpy( p_forward_addr , p_old_forward_addr , sizeof(struct ForwardNetAddress) );
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
			
			InfoLog( __FILE__ , __LINE__ , "AddListeners sock[%s:%d][%d]" , p_forward_addr->netaddr.ip , p_forward_addr->netaddr.port.port_int , p_forward_addr->sock );
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
		if( IsForwardSessionUsed( p_forward_session ) == 0 )
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
