#include "G6.h"

static int SelectServerAddress( struct ServerEnv *penv , struct ForwardSession *p_forward_session )
{
	struct ForwardRule	*p_forward_rule = p_forward_session->p_forward_rule ;
	
	if( STRCMP( p_forward_rule->load_balance_algorithm , == , LOAD_BALANCE_ALGORITHM_MS ) )
	{
		int		n ;
		
		for( n = 0 ; n < p_forward_rule->servers_addr_size ; n++ )
		{
			if(	p_forward_rule->servers_addr[p_forward_rule->selected_addr_index].server_unable == 0
				||
				(
					p_forward_rule->servers_addr[p_forward_rule->selected_addr_index].server_unable == 1
					&&
					p_forward_rule->servers_addr[p_forward_rule->selected_addr_index].timestamp_to_enable >= time(NULL)
				)
			)
			{
				p_forward_session->server_index = p_forward_rule->selected_addr_index ;
				p_forward_rule->selected_addr_index++;
				if( p_forward_rule->selected_addr_index >= p_forward_rule->servers_addr_count )
					p_forward_rule->selected_addr_index = 0 ;
				break;
			}
			
			p_forward_rule->selected_addr_index++;
			if( p_forward_rule->selected_addr_index >= p_forward_rule->servers_addr_count )
				p_forward_rule->selected_addr_index = 0 ;
		}
		if( n >= p_forward_rule->servers_addr_size )
		{
			ErrorLog( __FILE__ , __LINE__ , "all servers unable" );
			return -1;
		}
	}
	else if( STRCMP( p_forward_rule->load_balance_algorithm , == , LOAD_BALANCE_ALGORITHM_RR ) )
	{
		int		n ;
		
		for( n = 0 ; n < p_forward_rule->servers_addr_size ; n++ )
		{
			p_forward_rule->selected_addr_index++;
			if( p_forward_rule->selected_addr_index >= p_forward_rule->servers_addr_count )
				p_forward_rule->selected_addr_index = 0 ;
			
			if(	p_forward_rule->servers_addr[p_forward_rule->selected_addr_index].server_unable == 0
				||
				(
					p_forward_rule->servers_addr[p_forward_rule->selected_addr_index].server_unable == 1
					&&
					p_forward_rule->servers_addr[p_forward_rule->selected_addr_index].timestamp_to_enable >= time(NULL)
				)
			)
			{
				p_forward_session->server_index = p_forward_rule->selected_addr_index ;
				break;
			}
		}
		if( n >= p_forward_rule->servers_addr_size )
		{
			ErrorLog( __FILE__ , __LINE__ , "all servers unable" );
			return -1;
		}
	}
	else if( STRCMP( p_forward_rule->load_balance_algorithm , == , LOAD_BALANCE_ALGORITHM_LC ) )
	{
	}
	else if( STRCMP( p_forward_rule->load_balance_algorithm , == , LOAD_BALANCE_ALGORITHM_RT ) )
	{
	}
	else if( STRCMP( p_forward_rule->load_balance_algorithm , == , LOAD_BALANCE_ALGORITHM_RD ) )
	{
		int		n ;
		
		p_forward_rule->selected_addr_index += Rand( 1 , p_forward_rule->servers_addr_size ) ;
		if( p_forward_rule->selected_addr_index >= p_forward_rule->servers_addr_count )
			p_forward_rule->selected_addr_index = - p_forward_rule->servers_addr_count ;
		
		for( n = 0 ; n < p_forward_rule->servers_addr_size ; n++ )
		{
			p_forward_rule->selected_addr_index++;
			if( p_forward_rule->selected_addr_index >= p_forward_rule->servers_addr_count )
				p_forward_rule->selected_addr_index = 0 ;
			
			if(	p_forward_rule->servers_addr[p_forward_rule->selected_addr_index].server_unable == 0
				||
				(
					p_forward_rule->servers_addr[p_forward_rule->selected_addr_index].server_unable == 1
					&&
					p_forward_rule->servers_addr[p_forward_rule->selected_addr_index].timestamp_to_enable >= time(NULL)
				)
			)
			{
				unsigned long		selected_addr_2_index ;
				
				selected_addr_2_index = Rand( 0 , p_forward_rule->servers_addr_size - 1 ) ;
				if( selected_addr_2_index != p_forward_rule->selected_addr_index )
				{
					struct ServerNetAddress		tmp ;
					
					memcpy( & tmp , p_forward_rule->servers_addr+p_forward_rule->selected_addr_index , sizeof(struct ServerNetAddress) );
					memcpy( p_forward_rule->servers_addr+p_forward_rule->selected_addr_index , p_forward_rule->servers_addr+selected_addr_2_index , sizeof(struct ServerNetAddress) );
					memcpy( p_forward_rule->servers_addr+selected_addr_2_index , & tmp , sizeof(struct ServerNetAddress) );
				}
				p_forward_session->server_index = selected_addr_2_index ;
				break;
			}
		}
		if( n >= p_forward_rule->servers_addr_size )
		{
			ErrorLog( __FILE__ , __LINE__ , "all servers unable" );
			return -1;
		}
	}
	else if( STRCMP( p_forward_rule->load_balance_algorithm , == , LOAD_BALANCE_ALGORITHM_HS ) )
	{
		p_forward_session->server_index = CalcHash(p_forward_rule->clients_addr[p_forward_session->client_index].netaddr.ip) % p_forward_rule->servers_addr_count ;
		if(	p_forward_rule->servers_addr[p_forward_rule->selected_addr_index].server_unable == 0
			||
			(
				p_forward_rule->servers_addr[p_forward_rule->selected_addr_index].server_unable == 1
				&&
				p_forward_rule->servers_addr[p_forward_rule->selected_addr_index].timestamp_to_enable >= time(NULL)
			)
		)
		{
		}
		else
		{
			ErrorLog( __FILE__ , __LINE__ , "server unable" );
			return -1;
		}
	}
	else
	{
		ErrorLog( __FILE__ , __LINE__ , "load_balance_algorithm[%s] invalid" , p_forward_session->p_forward_rule->load_balance_algorithm );
		return -1;
	}
	
	return 0;
}

static void ResolveConnectingError( struct ServerEnv *penv , struct ForwardSession *p_reverse_forward_session )
{
	struct ServerNetAddress		*p_servers_addr = NULL ;
	
	/*
	int				nret = 0 ;
	*/
	
	/* 关闭老套接字 */
	/*
	epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_DEL , p_forward_session->sock , NULL );
	*/
	_CLOSESOCKET( p_reverse_forward_session->sock );
	
	/* 设置服务端不可用 */
	p_servers_addr = p_reverse_forward_session->p_forward_rule->servers_addr + p_reverse_forward_session->server_index ;
	p_servers_addr->server_unable = 1 ;
	p_servers_addr->timestamp_to_enable = time(NULL) + 10 ;
	
	/* 非堵塞连接服务端 */
	/*
	nret = TryToConnectServer( penv , p_forward_session ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "TryToConnectServer failed[%d]" , nret );
		epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_DEL , p_forward_session->sock , NULL );
		epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_DEL , p_forward_session->p_reverse_forward_session->sock , NULL );
		DebugLog( __FILE__ , __LINE__ , "close #%d#" , p_forward_session->p_reverse_forward_session->sock );
		_CLOSESOCKET( p_forward_session->p_reverse_forward_session->sock );
		SetForwardSessionUnused2( p_forward_session , p_forward_session->p_reverse_forward_session );
		return;
	}
	*/
	
	return;
}

static int TryToConnectServer( struct ServerEnv *penv , struct ForwardSession *p_reverse_forward_session )
{
	struct ServerNetAddress	*p_servers_addr = NULL ;
	_SOCKLEN_T		addr_len ;
	struct epoll_event	event ;
	int			epoll_fd_index ;
	
	int			nret = 0 ;
	
	while(1)
	{
		/* 根据负载均衡算法选择服务端 */
		nret = SelectServerAddress( penv , p_reverse_forward_session ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "SelectServerAddress failed[%d]" , nret );
			return 1;
		}
		
		/* 创建连接服务端的客户端 */
		p_reverse_forward_session->sock = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP );
		if( p_reverse_forward_session->sock == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "socket failed , errno[%d]" , errno );
			return -1;
		}
		
		SetNonBlocking( p_reverse_forward_session->sock );
		
		/* 连接服务端 */
		p_servers_addr = p_reverse_forward_session->p_forward_rule->servers_addr + p_reverse_forward_session->server_index ;
		addr_len = sizeof(struct sockaddr) ;
		nret = connect( p_reverse_forward_session->sock , (struct sockaddr *) & (p_servers_addr->netaddr.sockaddr) , addr_len );
		if( nret == -1 )
		{
			if( _ERRNO == _EINPROGRESS ) /* 正在连接 */
			{
				p_reverse_forward_session->status = FORWARD_SESSION_STATUS_CONNECTING ;
				
				InfoLog( __FILE__ , __LINE__ , "#%d#-#%d# connecting [%s:%d] ..." , p_reverse_forward_session->p_reverse_forward_session->sock , p_reverse_forward_session->sock , p_servers_addr->netaddr.ip , p_servers_addr->netaddr.port.port_int );
				
				memset( & event , 0x00 , sizeof(struct epoll_event) );
				event.data.ptr = p_reverse_forward_session ;
				event.events = EPOLLOUT | EPOLLERR ;
				epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_ADD , p_reverse_forward_session->sock , & event );
				
				memset( & event , 0x00 , sizeof(struct epoll_event) );
				event.data.ptr = p_reverse_forward_session->p_reverse_forward_session ;
				event.events = EPOLLERR ;
				epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_ADD , p_reverse_forward_session->p_reverse_forward_session->sock , & event );
				
				break;
			}
			else /* 连接失败 */
			{
				ErrorLog( __FILE__ , __LINE__ , "#%d#-#%d# connect [%s:%d] failed , errno[%d]" , p_reverse_forward_session->p_reverse_forward_session->sock , p_reverse_forward_session->sock , p_servers_addr->netaddr.ip , p_servers_addr->netaddr.port.port_int , errno );
				ResolveConnectingError( penv , p_reverse_forward_session );
				continue;
			}
		}
		else /* 连接成功 */
		{
			p_reverse_forward_session->status = FORWARD_SESSION_STATUS_CONNECTED ;
			
			InfoLog( __FILE__ , __LINE__ , "#%d#-#%d# connect [%s:%d] ok" , p_reverse_forward_session->p_reverse_forward_session->sock , p_reverse_forward_session->sock , p_servers_addr->netaddr.ip , p_servers_addr->netaddr.port.port_int );
			
			if( p_servers_addr->server_unable == 1 )
				p_servers_addr->server_unable = 0 ;
			
			epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_DEL , p_reverse_forward_session->sock , NULL );
			epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_DEL , p_reverse_forward_session->p_reverse_forward_session->sock , NULL );
			
			epoll_fd_index = (p_reverse_forward_session->sock) % (penv->cmd_para.forward_thread_size) ;
			
			memset( & event , 0x00 , sizeof(struct epoll_event) );
			event.data.ptr = p_reverse_forward_session ;
			event.events = EPOLLIN | EPOLLERR ;
			epoll_ctl( penv->forward_epoll_fd_array[epoll_fd_index] , EPOLL_CTL_MOD , p_reverse_forward_session->sock , & event );
			
			memset( & event , 0x00 , sizeof(struct epoll_event) );
			event.data.ptr = p_reverse_forward_session->p_reverse_forward_session ;
			event.events = EPOLLIN | EPOLLERR ;
			epoll_ctl( penv->forward_epoll_fd_array[epoll_fd_index] , EPOLL_CTL_MOD , p_reverse_forward_session->p_reverse_forward_session->sock , & event );
			
			break;
		}
	}
	
	return 0;
}

static int OnListenAccept( struct ServerEnv *penv , struct ForwardSession *p_listen_session )
{
	struct ForwardSession	*p_forward_session = NULL ;
	struct ForwardSession	*p_reverse_forward_session = NULL ;
	_SOCKLEN_T		addr_len = sizeof(struct sockaddr) ;
	
	int			nret = 0 ;
	
	/* 获取两个空闲会话结构 */
	p_forward_session = GetForwardSessionUnused( penv ) ;
	if( p_forward_session == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "GetForwardSessionUnused failed" );
		return -1;
	}
	
	p_reverse_forward_session = GetForwardSessionUnused( penv ) ;
	if( p_reverse_forward_session == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "GetForwardSessionUnused failed" );
		return -1;
	}
	
	p_forward_session->p_forward_rule = p_listen_session->p_forward_rule ;
	p_forward_session->p_reverse_forward_session = p_reverse_forward_session ;
	
	p_reverse_forward_session->p_forward_rule = p_listen_session->p_forward_rule ;
	p_reverse_forward_session->p_reverse_forward_session = p_forward_session ;
	
	/* 接收新客户端连接 */
	p_forward_session->sock = accept( p_listen_session->sock , (struct sockaddr *) & (p_forward_session->netaddr.sockaddr) , & addr_len ) ;
	if( p_forward_session->sock == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "accept failed , errno[%d]" , errno );
		SetForwardSessionUnused2( p_forward_session , p_reverse_forward_session );
		return -1;
	}
	else
	{
		GetNetAddress( & (p_forward_session->netaddr) );
		DebugLog( __FILE__ , __LINE__ , "accept [%s:%d]#%d# ok" , p_forward_session->netaddr.ip , p_forward_session->netaddr.port.port_int , p_forward_session->sock );
	}
	
	SetNonBlocking( p_forward_session->sock );
	
	/* 非堵塞连接服务端 */
	nret = TryToConnectServer( penv , p_reverse_forward_session ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "TryToConnectServer failed[%d]" , nret );
		DebugLog( __FILE__ , __LINE__ , "close #%d#" , p_forward_session->sock );
		_CLOSESOCKET( p_forward_session->sock );
		SetForwardSessionUnused2( p_forward_session , p_reverse_forward_session );
		return -1;
	}
	
	return 0;
}

static int OnConnectingServer( struct ServerEnv *penv , struct ForwardSession *p_forward_session )
{
#if ( defined __linux ) || ( defined __unix )
	int			error , code ;
#endif
	_SOCKLEN_T		addr_len ;
	struct epoll_event	event ;
	
	struct ServerNetAddress	*p_servers_addr = NULL ;
	int			epoll_fd_index ;
	
	/* 检查非堵塞连接结果 */
#if ( defined __linux ) || ( defined __unix )
	addr_len = sizeof(int) ;
	code = getsockopt( p_forward_session->sock , SOL_SOCKET , SO_ERROR , & error , & addr_len ) ;
	if( code < 0 || error )
	{
		ResolveConnectingError( penv , p_forward_session );
		return 0;
	}
#elif ( defined _WIN32 )
	addr_len = sizeof(struct sockaddr_in) ;
	nret = connect( p_forward_session->sock , ( struct sockaddr *) & (p_forward_session->p_forward_rule->servers_addr[p_forward_session->balance_algorithm.MS.server_index]->netaddr) , addr_len ) ;
	if( nret == -1 && _ERRNO == _EISCONN )
	{
		;
	}
	else
        {
		ResolveConnectingError( penv , p_forward_session );
		return 0;
        }
#endif
	
	/* 连接成功 */
	p_forward_session->status = FORWARD_SESSION_STATUS_CONNECTED ;
	
	p_servers_addr = p_forward_session->p_forward_rule->servers_addr + p_forward_session->server_index ;
	InfoLog( __FILE__ , __LINE__ , "#%d#-#%d# connect2 [%s:%d] ok" , p_forward_session->p_reverse_forward_session->sock , p_forward_session->sock , p_servers_addr->netaddr.ip , p_servers_addr->netaddr.port.port_int );
	
	if( p_forward_session->p_forward_rule->servers_addr[p_forward_session->server_index].server_unable == 1 )
		p_forward_session->p_forward_rule->servers_addr[p_forward_session->server_index].server_unable = 0 ;
	
	epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_DEL , p_forward_session->sock , NULL );
	
	epoll_fd_index = (p_forward_session->sock) % (penv->cmd_para.forward_thread_size) ;
	
	memset( & event , 0x00 , sizeof(struct epoll_event) );
	event.data.ptr = p_forward_session ;
	event.events = EPOLLIN | EPOLLERR ;
	epoll_ctl( penv->forward_epoll_fd_array[epoll_fd_index] , EPOLL_CTL_ADD , p_forward_session->sock , & event );
	
	memset( & event , 0x00 , sizeof(struct epoll_event) );
	event.data.ptr = p_forward_session->p_reverse_forward_session ;
	event.events = EPOLLIN | EPOLLERR ;
	epoll_ctl( penv->forward_epoll_fd_array[epoll_fd_index] , EPOLL_CTL_ADD , p_forward_session->p_reverse_forward_session->sock , & event );
	
	return 0;
}

void *AcceptThread( struct ServerEnv *penv )
{
	struct epoll_event	events[ WAIT_EVENTS_COUNT ] ;
	int			event_count ;
	int			event_index ;
	struct epoll_event	*p_event = NULL ;
	
	char			command ;
	
	struct ForwardSession	*p_forward_session = NULL ;
	
	int			nret = 0 ;
	
	/* 设置日志输出文件 */
	SetLogFile( "%s/log/G6.log" , getenv("HOME") );
	SetLogLevel( penv->cmd_para.log_level );
	InfoLog( __FILE__ , __LINE__ , "--- G6.WorkerProcess.AcceptThread ---" );
	
	/* 主工作循环 */
	while(1)
	{
		event_count = epoll_wait( penv->accept_epoll_fd , events , WAIT_EVENTS_COUNT , -1 ) ;
		DebugLog( __FILE__ , __LINE__ , "epoll_wait return [%d]events" , event_count );
		for( event_index = 0 , p_event = events ; event_index < event_count ; event_index++ , p_event++ )
		{
			if( p_event->data.ptr == NULL ) /* 命令管道事件 */
			{
				DebugLog( __FILE__ , __LINE__ , "command pipe event" );
				
				if( p_event->events & EPOLLIN )
				{
					read( penv->accept_pipe.fds[0] , & command , 1 );
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
				
				if( p_forward_session->status == FORWARD_SESSION_STATUS_LISTEN )
				{
					DebugLog( __FILE__ , __LINE__ , "listen session event" );
					
					if( p_event->events & EPOLLIN ) /* 侦听端口事件 */
					{
						nret = OnListenAccept( penv , p_forward_session ) ;
						if( nret )
						{
							ErrorLog( __FILE__ , __LINE__ , "OnListenAccept failed[%d]" , nret );
						}
					}
					else if( p_event->events & EPOLLERR )
					{
						ErrorLog( __FILE__ , __LINE__ , "accept pipe EPOLLERR" );
						exit(0);
					}
					else if( p_event->events & EPOLLHUP )
					{
						ErrorLog( __FILE__ , __LINE__ , "accept pipe EPOLLHUP" );
						exit(0);
					}
					else
					{
						ErrorLog( __FILE__ , __LINE__ , "accept pipe [%d]" , p_event->events );
						exit(0);
					}
				}
				else if( p_forward_session->status == FORWARD_SESSION_STATUS_CONNECTING )
				{
					DebugLog( __FILE__ , __LINE__ , "connecting session event" );
					
					if( p_event->events & EPOLLOUT ) /* 非堵塞连接事件 */
					{
						nret = OnConnectingServer( penv , p_forward_session ) ;
						if( nret )
						{
							ErrorLog( __FILE__ , __LINE__ , "OnConnectingServer failed[%d]" , nret );
						}
					}
					else if( p_event->events & EPOLLERR )
					{
						ErrorLog( __FILE__ , __LINE__ , "connecting session EPOLLERR" );
						ResolveConnectingError( penv , p_forward_session );
					}
					else if( p_event->events & EPOLLHUP )
					{
						ErrorLog( __FILE__ , __LINE__ , "connecting session EPOLLHUP" );
						ResolveConnectingError( penv , p_forward_session );
					}
					else
					{
						ErrorLog( __FILE__ , __LINE__ , "connecting session [%d]" , p_event->events );
						ResolveConnectingError( penv , p_forward_session );
					}
				}
				else
				{
					ErrorLog( __FILE__ , __LINE__ , "unknow event" );
					exit(0);
				}
			}
		}
	}
	
	return 0;
}

void *_AcceptThread( void *pv )
{
	return AcceptThread( (struct ServerEnv *)pv );
}

