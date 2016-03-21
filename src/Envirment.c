#include "G6.h"

int InitEnvirment( struct ServerEnv *penv )
{
	unsigned int		forward_thread_index ;
	
	struct epoll_event	event ;
	
	int			nret = 0 ;
	
	/* 创建线程ID数组 */
	penv->forward_thread_tid_array = (pthread_t *)malloc( sizeof(pthread_t) * penv->cmd_para.forward_thread_size ) ;
	if( penv->forward_thread_tid_array == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "malloc failed , errno[%d]" , errno );
		return -1;
	}
	memset( penv->forward_thread_tid_array , 0x00 , sizeof(pthread_t) * penv->cmd_para.forward_thread_size );
	
	/* 创建父子进程命令管道 */
	nret = pipe( penv->accept_command_pipe.fds ) ;
	if( nret == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "pipe failed , errno[%d]" , errno );
		return -1;
	}
	else
	{
		DebugLog( __FILE__ , __LINE__ , "pipe ok[%d][%d]" , penv->accept_command_pipe.fds[0] , penv->accept_command_pipe.fds[1] );
		SetCloseExec2( penv->accept_command_pipe.fds[0] , penv->accept_command_pipe.fds[1] );
	}
	
	/* 创建侦听端口epoll池 */
	penv->accept_epoll_fd = epoll_create( 1024 );
	if( penv->accept_epoll_fd == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "epoll_create failed , errno[%d]" , errno );
		return -1;
	}
	else
	{
		DebugLog( __FILE__ , __LINE__ , "epoll_create ok[%d]" , penv->accept_epoll_fd );
		SetCloseExec( penv->accept_epoll_fd );
	}
	
	/* 加入父子进程命令管道到侦听端口epoll池 */
	memset( & event , 0x00 , sizeof(event) );
	event.data.ptr = NULL ;
	event.events = EPOLLIN | EPOLLERR ;
	epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_ADD , penv->accept_command_pipe.fds[0] , & event );
	
	/* 申请父子线程命令管道数组 */
	penv->forward_command_pipe = (struct PipeFds *)malloc( sizeof(struct PipeFds) * penv->cmd_para.forward_thread_size ) ;
	if( penv->forward_command_pipe == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "malloc failed , errno[%d]" , errno );
		return -1;
	}
	memset( penv->forward_command_pipe , 0x00 , sizeof(struct PipeFds) * penv->cmd_para.forward_thread_size );
	
	penv->forward_epoll_fd_array = (int *)malloc( sizeof(int) * penv->cmd_para.forward_thread_size ) ;
	if( penv->forward_epoll_fd_array == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "malloc failed , errno[%d]" , errno );
		return -1;
	}
	memset( penv->forward_epoll_fd_array , 0x00 , sizeof(int) * penv->cmd_para.forward_thread_size );
	
	for( forward_thread_index = 0 ; forward_thread_index < penv->cmd_para.forward_thread_size ; forward_thread_index++ )
	{
		/* 创建父子线程命令管道 */
		nret = pipe( penv->forward_command_pipe[forward_thread_index].fds ) ;
		if( nret == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "pipe failed , errno[%d]" , errno );
			return -1;
		}
		else
		{
			DebugLog( __FILE__ , __LINE__ , "pipe ok[%d][%d]" , penv->forward_command_pipe[forward_thread_index].fds[0] , penv->forward_command_pipe[forward_thread_index].fds[1] );
			SetCloseExec2( penv->forward_command_pipe[forward_thread_index].fds[0] , penv->forward_command_pipe[forward_thread_index].fds[1] );
		}
		
		/* 创建数据收发epoll池 */
		penv->forward_epoll_fd_array[forward_thread_index] = epoll_create( 1024 );
		if( penv->forward_epoll_fd_array[forward_thread_index] == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "epoll_create failed , errno[%d]" , errno );
			return -1;
		}
		else
		{
			DebugLog( __FILE__ , __LINE__ , "epoll_create ok[%d]" , penv->forward_epoll_fd_array[forward_thread_index] );
			SetCloseExec( penv->forward_epoll_fd_array[forward_thread_index] );
		}
		
		/* 加入父子线程命令管道到数据收发epoll池 */
		memset( & event , 0x00 , sizeof(event) );
		event.data.ptr = NULL ;
		event.events = EPOLLIN | EPOLLERR ;
		epoll_ctl( penv->forward_epoll_fd_array[forward_thread_index] , EPOLL_CTL_ADD , penv->forward_command_pipe[forward_thread_index].fds[0] , & event );
	}
	
	/* 申请通讯收发会话数组 */
	penv->forward_session_array = (struct ForwardSession *)malloc( sizeof(struct ForwardSession) * penv->cmd_para.forward_session_size ) ;
	if( penv->forward_session_array == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "malloc failed , errno[%d]" , errno );
		return -1;
	}
	memset( penv->forward_session_array , 0x00 , sizeof(struct ForwardSession) * penv->cmd_para.forward_session_size );
	
	/* 初始化超时红黑树 */
	penv->timeout_rbtree = RB_ROOT ;
	
	/* 创建线程互斥量 */
	pthread_mutex_init( & (penv->timeout_rbtree_mutex) , NULL );
	
	return 0;
}

void CleanEnvirment( struct ServerEnv *penv )
{
	unsigned int			forward_session_index ;
	struct ForwardSession		*p_forward_session = NULL ;
	
	unsigned int			forward_thread_index ;
	
	if( penv->old_forward_addr_array )
	{
		free( penv->old_forward_addr_array );
		penv->old_forward_addr_array = NULL ;
	}
	
	epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_DEL , penv->accept_command_pipe.fds[0] , NULL );
	close( penv->accept_command_pipe.fds[0] );
	close( penv->accept_command_pipe.fds[1] );
	
	close( penv->accept_epoll_fd );
	free( penv->forward_thread_tid_array );
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
	free( penv->forward_session_array );
	
	for( forward_thread_index = 0 ; forward_thread_index < penv->cmd_para.forward_thread_size ; forward_thread_index++ )
	{
		close( penv->forward_command_pipe[forward_thread_index].fds[0] );
		close( penv->forward_command_pipe[forward_thread_index].fds[1] );
		close( penv->forward_epoll_fd_array[forward_thread_index] );
	}
	free( penv->forward_command_pipe );
	free( penv->forward_epoll_fd_array );
	
	pthread_mutex_destroy( & (penv->timeout_rbtree_mutex) );
	
	return;
}

int SaveListenSockets( struct ServerEnv *penv )
{
	unsigned int		forward_session_index ;
	struct ForwardSession	*p_forward_session = NULL ;
	unsigned int		listen_socket_count ;
	char			*env_value = NULL ;
	unsigned int		env_value_size ;
	
	/* 统计侦听端口数量 */
	listen_socket_count = 0 ;
	for( forward_session_index = 0 , p_forward_session = penv->forward_session_array ; forward_session_index < penv->cmd_para.forward_session_size ; forward_session_index++ , p_forward_session++ )
	{
		if( p_forward_session->type == FORWARD_SESSION_TYPE_LISTEN )
			listen_socket_count++;
	}
	
	/* 申请临时缓冲区用于存放侦听端口描述字列表 */
	env_value_size = (1+listen_socket_count)*(10+1) + 1 ;
	env_value = (char*)malloc( env_value_size ) ;
	if( env_value == NULL )
		return -1;
	memset( env_value , 0x00 , env_value_size );
	
	/* 组装侦听端口描述字列表 */
	_SNPRINTF( env_value , env_value_size-1 , "%d|" , listen_socket_count );
	
	for( forward_session_index = 0 , p_forward_session = penv->forward_session_array ; forward_session_index < penv->cmd_para.forward_session_size ; forward_session_index++ , p_forward_session++ )
	{
		if( p_forward_session->type == FORWARD_SESSION_TYPE_LISTEN )
		{
			_SNPRINTF( env_value+strlen(env_value) , env_value_size-1-strlen(env_value) , "%d|" , p_forward_session->sock );
		}
	}
	
	/* 写环境变量 */
	InfoLog( __FILE__ , __LINE__ , "setenv[%s][%s]" , G6_LISTEN_SOCKFDS , env_value );
	setenv( G6_LISTEN_SOCKFDS , env_value , 1 );
	
	/* 是否临时缓冲区 */
	free( env_value );
	
	return 0;
}

int LoadOldListenSockets( struct ServerEnv *penv )
{
	char				*p_env_value = NULL ;
	char				*p_sockfd_count = NULL ;
	unsigned int			old_forward_addr_index ;
	struct ForwardNetAddress	*p_forward_addr = NULL ;
	char				*p_sockfd = NULL ;
	_SOCKLEN_T			addr_len = sizeof(struct sockaddr) ;
	
	int				nret = 0 ;
	
	/* 读出用于存放老进程侦听端口描述字列表环境变量 */
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
				/* 申请临时存放分解出来的老进程侦听端口描述字列表 */
				penv->old_forward_addr_array = (struct ForwardNetAddress *)malloc( sizeof(struct ForwardNetAddress) * penv->old_forward_addr_count ) ;
				if( penv->old_forward_addr_array == NULL )
				{
					ErrorLog( __FILE__ , __LINE__ , "malloc failed , errno[%d]" , errno );
					free( p_env_value );
					return -1;
				}
				memset( penv->old_forward_addr_array , 0x00 , sizeof(struct ForwardNetAddress) * penv->old_forward_addr_count );
				
				/* 分解老进程侦听端口描述字列表 */
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
	unsigned int			old_forward_addr_index ;
	struct ForwardNetAddress	*p_old_forward_addr = NULL ;
	
	if( penv->old_forward_addr_array == NULL )
		return 0;
	
	/* 关闭新进程未用的老进程侦听端口描述字 */
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
	unsigned int			forward_rule_index ;
	struct ForwardRule		*p_forward_rule = NULL ;
	unsigned int			forward_addr_index ;
	struct ForwardNetAddress	*p_forward_addr = NULL ;
	
	unsigned int			old_forward_addr_index ;
	struct ForwardNetAddress	*p_old_forward_addr = NULL ;
	
	struct ForwardSession		*p_forward_session = NULL ;
	struct epoll_event		event ;
	
	int				nret = 0 ;
	
	/* 装载老进程传递过来的侦听端口描述字，如果有的话 */
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
				/* 挑选老进程侦听端口描述字 */
				for( old_forward_addr_index = 0 , p_old_forward_addr = penv->old_forward_addr_array ; old_forward_addr_index < penv->old_forward_addr_count ; old_forward_addr_index++ , p_old_forward_addr++ )
				{
					if( STRCMP( p_old_forward_addr->netaddr.ip , == , p_forward_rule->forward_addr_array[forward_addr_index].netaddr.ip ) && p_old_forward_addr->netaddr.port.port_int == p_forward_rule->forward_addr_array[forward_addr_index].netaddr.port.port_int )
					{
						memcpy( & (p_forward_addr->netaddr.sockaddr) , & (p_old_forward_addr->netaddr.sockaddr) , sizeof(struct sockaddr_in) );
						p_forward_addr->sock = p_old_forward_addr->sock ;
						memset( p_old_forward_addr , 0x00 , sizeof(struct ForwardNetAddress) );
						break;
					}
				}
				if( old_forward_addr_index >= penv->old_forward_addr_count )
					goto _GOTO_CREATE_LISTENER; /* 如果没有，则新创建 */
			}
			else
			{
_GOTO_CREATE_LISTENER :
				/* 创建侦听端口 */
				p_forward_addr->sock = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP ) ;
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
			
			/* 获取一个空闲会话结构，存放侦听会话信息 */
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
			
			/* 加入侦听会话到侦听端口epoll池 */
			memset( & event , 0x00 , sizeof(event) );
			event.data.ptr = p_forward_session ;
			event.events = EPOLLIN | EPOLLERR | EPOLLET ;
			epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_ADD , p_forward_addr->sock , & event );
			
			if( penv->old_forward_addr_array && old_forward_addr_index < penv->old_forward_addr_count )
			{
				InfoLog( __FILE__ , __LINE__ , "reuse listener sock[%s:%d]#%d#" , p_forward_addr->netaddr.ip , p_forward_addr->netaddr.port.port_int , p_forward_addr->sock );
			}
			else
			{
				InfoLog( __FILE__ , __LINE__ , "add listener sock[%s:%d]#%d#" , p_forward_addr->netaddr.ip , p_forward_addr->netaddr.port.port_int , p_forward_addr->sock );
			}
		}
	}
	
	/* 清理未用到的侦听端口描述字 */
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
	unsigned int		n ;
	struct ForwardSession	*p_forward_session = penv->forward_session_array + penv->forward_session_use_offsetpos ;
	
	for( n = 0 ; n < penv->cmd_para.forward_session_size ; n++ )
	{
		if( p_forward_session->status == FORWARD_SESSION_STATUS_UNUSED )
		{
			memset( p_forward_session , 0x00 , sizeof(struct ForwardSession) );
			p_forward_session->status = FORWARD_SESSION_STATUS_READY ;
			penv->forward_session_use_offsetpos++;
			if( penv->forward_session_use_offsetpos >= penv->cmd_para.forward_session_size )
			{
				penv->forward_session_use_offsetpos = 0 ;
			}
			
			__sync_fetch_and_add(&(penv->forward_session_count),1);
			
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
	{
		p_forward_session->status = FORWARD_SESSION_STATUS_UNUSED ;
	}
	
	__sync_fetch_and_sub(&(penv->forward_session_count),1);
	
	return;
}

void SetForwardSessionUnused2( struct ServerEnv *penv , struct ForwardSession *p_forward_session , struct ForwardSession *p_forward_session2 )
{
	if( p_forward_session->status != FORWARD_SESSION_STATUS_UNUSED )
	{
		p_forward_session->status = FORWARD_SESSION_STATUS_UNUSED ;
	}
	
	if( p_forward_session2->status != FORWARD_SESSION_STATUS_UNUSED )
	{
		p_forward_session2->status = FORWARD_SESSION_STATUS_UNUSED ;
	}
	
	__sync_fetch_and_sub(&(penv->forward_session_count),2);
	
	return;
}

static void _RemoveTimeoutTreeNode( struct ServerEnv *penv , struct ForwardSession *p_forward_session )
{
	rb_erase( & (p_forward_session->timeout_rbnode) , & (penv->timeout_rbtree) );
	return;
}

void RemoveTimeoutTreeNode( struct ServerEnv *penv , struct ForwardSession *p_forward_session )
{
	pthread_mutex_lock( & (penv->timeout_rbtree_mutex) );
	
	_RemoveTimeoutTreeNode( penv , p_forward_session );
	
	pthread_mutex_unlock( & (penv->timeout_rbtree_mutex) );
	
	return;
}

void RemoveTimeoutTreeNode2( struct ServerEnv *penv , struct ForwardSession *p_forward_session , struct ForwardSession *p_forward_session2 )
{
	pthread_mutex_lock( & (penv->timeout_rbtree_mutex) );
	
	_RemoveTimeoutTreeNode( penv , p_forward_session );
	_RemoveTimeoutTreeNode( penv , p_forward_session2 );
	
	pthread_mutex_unlock( & (penv->timeout_rbtree_mutex) );
	
	return;
}

static int _AddTimeoutTreeNode( struct ServerEnv *penv , struct ForwardSession *p_forward_session , unsigned int timeout_timestamp )
{
        struct rb_node		**pp_new_node = NULL ;
        struct rb_node		*p_parent = NULL ;
        struct ForwardSession	*p = NULL ;
	
	p_forward_session->timeout_timestamp = timeout_timestamp ;
	
	pp_new_node = & (penv->timeout_rbtree.rb_node) ;
        while( *pp_new_node )
        {
                p = container_of( *pp_new_node , struct ForwardSession , timeout_rbnode ) ;
		
                p_parent = (*pp_new_node) ;
                if( p_forward_session->timeout_timestamp < p->timeout_timestamp )
                        pp_new_node = &((*pp_new_node)->rb_left) ;
                else if( p_forward_session->timeout_timestamp > p->timeout_timestamp )
                        pp_new_node = &((*pp_new_node)->rb_right) ;
                else 
                        pp_new_node = &((*pp_new_node)->rb_left) ;
        }
	
        rb_link_node( & (p_forward_session->timeout_rbnode) , p_parent , pp_new_node );
        rb_insert_color( & (p_forward_session->timeout_rbnode) , &(penv->timeout_rbtree) );
	
	return 0;
}

int AddTimeoutTreeNode( struct ServerEnv *penv , struct ForwardSession *p_forward_session , unsigned int timeout_timestamp )
{
	int		nret = 0 ;
	
	pthread_mutex_lock( & (penv->timeout_rbtree_mutex) );
	
	nret = _AddTimeoutTreeNode( penv , p_forward_session , timeout_timestamp ) ;
	
	pthread_mutex_unlock( & (penv->timeout_rbtree_mutex) );
	
        return 0;
}

int AddTimeoutTreeNode2( struct ServerEnv *penv , struct ForwardSession *p_forward_session , struct ForwardSession *p_forward_session2 , unsigned int timeout_timestamp )
{
	int		nret = 0 ;
	
	pthread_mutex_lock( & (penv->timeout_rbtree_mutex) );
	
	nret = _AddTimeoutTreeNode( penv , p_forward_session , timeout_timestamp ) ;
	if( nret )
	{
		pthread_mutex_unlock( & (penv->timeout_rbtree_mutex) );
		return nret;
	}
	
	nret = _AddTimeoutTreeNode( penv , p_forward_session2 , timeout_timestamp ) ;
	if( nret )
	{
		_RemoveTimeoutTreeNode( penv , p_forward_session );
		pthread_mutex_unlock( & (penv->timeout_rbtree_mutex) );
		return nret;
	}
	
	pthread_mutex_unlock( & (penv->timeout_rbtree_mutex) );
	
	return 0;
}

static int _UpdateTimeoutNode( struct ServerEnv *penv , struct ForwardSession *p_forward_session , unsigned int timeout_timestamp )
{
	int		nret = 0 ;
	
	_RemoveTimeoutTreeNode( penv , p_forward_session );
	
	nret = _AddTimeoutTreeNode( penv , p_forward_session , timeout_timestamp ) ;
	
	return nret;
}

int UpdateTimeoutNode( struct ServerEnv *penv , struct ForwardSession *p_forward_session , unsigned int timeout_timestamp )
{
	int		nret = 0 ;
	
	pthread_mutex_lock( & (penv->timeout_rbtree_mutex) );
	
	nret = _UpdateTimeoutNode( penv , p_forward_session , timeout_timestamp ) ;
	
	pthread_mutex_unlock( & (penv->timeout_rbtree_mutex) );
	
	return 0;
}

int UpdateTimeoutNode2( struct ServerEnv *penv , struct ForwardSession *p_forward_session , struct ForwardSession *p_forward_session2 , unsigned int timeout_timestamp )
{
	int		nret = 0 ;
	
	pthread_mutex_lock( & (penv->timeout_rbtree_mutex) );
	
	nret = _UpdateTimeoutNode( penv , p_forward_session , timeout_timestamp ) ;
	if( nret )
	{
		pthread_mutex_unlock( & (penv->timeout_rbtree_mutex) );
		return nret;
	}
	
	nret = _UpdateTimeoutNode( penv , p_forward_session2 , timeout_timestamp ) ;
	
	pthread_mutex_unlock( & (penv->timeout_rbtree_mutex) );
	
	return nret;
}

int GetLastestTimeout( struct ServerEnv *penv )
{
	struct rb_node		*p_node = NULL ;
	struct ForwardSession	*p_forward_session = NULL ;
	int			timeout ;
	
	pthread_mutex_lock( & (penv->timeout_rbtree_mutex) );
	
	p_node = rb_first( & (penv->timeout_rbtree) );
	if (p_node == NULL)
	{
		pthread_mutex_unlock( & (penv->timeout_rbtree_mutex) );
		return -1;
	}
	
	p_forward_session = rb_entry( p_node , struct ForwardSession , timeout_rbnode );
	timeout = p_forward_session->timeout_timestamp - GETSECONDSTAMP ;
	if( timeout < 0 )
		timeout = 0 ;
	
	pthread_mutex_unlock( & (penv->timeout_rbtree_mutex) );
	
	return timeout*1000;
}

struct ForwardSession *GetExpireTimeoutNode( struct ServerEnv *penv )
{
	struct rb_node		*p_node = NULL ;
	struct ForwardSession	*p_forward_session = NULL ;
	
	pthread_mutex_lock( & (penv->timeout_rbtree_mutex) );
	
	p_node = rb_first( & (penv->timeout_rbtree) ); 
	if (p_node == NULL)
	{
		pthread_mutex_unlock( & (penv->timeout_rbtree_mutex) );
		return NULL;
	}
	
	p_forward_session = rb_entry( p_node , struct ForwardSession , timeout_rbnode );
	if( p_forward_session->timeout_timestamp - GETSECONDSTAMP <= 0 )
	{
		pthread_mutex_unlock( & (penv->timeout_rbtree_mutex) );
		return p_forward_session;
	}
	
	pthread_mutex_unlock( & (penv->timeout_rbtree_mutex) );
	
	return NULL;
}

int InitIpConnectionStat( struct IpConnectionStat *p_ip_connection_stat )
{
	if( p_ip_connection_stat->max_ip > 0 || p_ip_connection_stat->max_connections > 0 || p_ip_connection_stat->max_connections_per_ip > 0 )
	{
		p_ip_connection_stat->ip_connection_stat_size = DEFAULT_IP_CONNECTION_ARRAY_SIZE ;
		p_ip_connection_stat->ip_connection_array = (struct IpConnection *)malloc( sizeof(struct IpConnection) * p_ip_connection_stat->ip_connection_stat_size ) ;
		if( p_ip_connection_stat->ip_connection_array == NULL )
		{
			ErrorLog( __FILE__ , __LINE__ , "malloc failed , errno[%d]" , errno );
			return -1;
		}
		memset( p_ip_connection_stat->ip_connection_array , 0x00 , sizeof(struct IpConnection) * p_ip_connection_stat->ip_connection_stat_size );
		
		p_ip_connection_stat->ip_count = 0 ;
		p_ip_connection_stat->connection_count = 0 ;
	}
	else
	{
		memset( p_ip_connection_stat , 0x00 , sizeof(struct IpConnectionStat) );
	}
	
	return 0;
}

void CleanIpConnectionStat( struct IpConnectionStat *p_ip_connection_stat )
{
	if( p_ip_connection_stat->max_ip > 0 || p_ip_connection_stat->max_connections > 0 || p_ip_connection_stat->max_connections_per_ip > 0 )
	{
		if( p_ip_connection_stat->ip_connection_array )
		{
			free( p_ip_connection_stat->ip_connection_array );
			p_ip_connection_stat->ip_connection_array = NULL ;
		}
	}
	
	return;
}

static int _AddIpConnectionStat( struct ServerEnv *penv , struct IpConnectionStat *p_ip_connection_stat , uint32_t ip_int , unsigned int connection_count )
{
	unsigned int		n ;
	unsigned int		ip_connection_index ;
	struct IpConnection	*p_ip_connection = NULL ;
	
	ip_connection_index = ip_int % (p_ip_connection_stat->ip_connection_stat_size) ;
	p_ip_connection = p_ip_connection_stat->ip_connection_array+ip_connection_index ;
	for( n = 0 ; n < p_ip_connection_stat->ip_connection_stat_size ; n++ )
	{
		if( p_ip_connection->used_flag == 0 )
		{
			if( p_ip_connection_stat->max_ip > 0 && p_ip_connection_stat->ip_count+1 > p_ip_connection_stat->max_ip )
			{
				ErrorLog( __FILE__ , __LINE__ , "too many ip" );
				return -1;
			}
			
			if( p_ip_connection_stat->max_connections_per_ip > 0 && p_ip_connection->connection_count+1 > p_ip_connection_stat->max_connections_per_ip )
			{
				ErrorLog( __FILE__ , __LINE__ , "too many connections per ip" );
				return -1;
			}
			
			p_ip_connection->used_flag = 1 ;
			p_ip_connection->ip_int = ip_int ;
			__sync_fetch_and_add( &(p_ip_connection->connection_count) , connection_count );
			
			__sync_fetch_and_add( &(p_ip_connection_stat->ip_count) , 1 );
			__sync_fetch_and_add( &(p_ip_connection_stat->connection_count) , connection_count );
			
			return 0;
		}
		else if( p_ip_connection->ip_int == ip_int )
		{
			if( p_ip_connection_stat->max_connections_per_ip > 0 && p_ip_connection->connection_count+1 > p_ip_connection_stat->max_connections_per_ip )
			{
				ErrorLog( __FILE__ , __LINE__ , "too many connections per ip" );
				return -1;
			}
			
			__sync_fetch_and_add( &(p_ip_connection->connection_count) , connection_count );
			
			__sync_fetch_and_add( &(p_ip_connection_stat->connection_count) , connection_count );
			
			return 0;
		}
		
		ip_connection_index++;
		p_ip_connection++;
		if( ip_connection_index >= p_ip_connection_stat->ip_connection_stat_size )
		{
			ip_connection_index = 0 ;
			p_ip_connection = p_ip_connection_stat->ip_connection_array ;
		}
	}
	
	ErrorLog( __FILE__ , __LINE__ , "internal error" );
	return -9;
}

int AddIpConnectionStat( struct ServerEnv *penv , struct IpConnectionStat *p_ip_connection_stat , uint32_t ip_int )
{
	int			nret = 0 ;
	
	if( p_ip_connection_stat->ip_connection_stat_size == 0 )
	{
		return 0;
	}
	
	if( p_ip_connection_stat->max_connections > 0 && p_ip_connection_stat->connection_count+1 > p_ip_connection_stat->max_connections )
	{
		ErrorLog( __FILE__ , __LINE__ , "too many connections" );
		return -1;
	}
	
	if( p_ip_connection_stat->connection_count+1 > p_ip_connection_stat->ip_connection_stat_size/2 )
	{
		struct IpConnectionStat		new_ip_connection_stat ;
		unsigned int			connection_index ;
		struct IpConnection		*p_ip_connection = NULL ;
		
		memset( & new_ip_connection_stat , 0x00 , sizeof(struct IpConnectionStat) );
		
		new_ip_connection_stat.max_ip = p_ip_connection_stat->max_ip ;
		new_ip_connection_stat.max_connections = p_ip_connection_stat->max_connections ;
		new_ip_connection_stat.max_connections_per_ip = p_ip_connection_stat->max_connections_per_ip ;
		
		new_ip_connection_stat.ip_connection_stat_size = p_ip_connection_stat->ip_connection_stat_size * 2 ;
		new_ip_connection_stat.ip_connection_array = (struct IpConnection *)malloc( sizeof(struct IpConnection) * new_ip_connection_stat.ip_connection_stat_size ) ;
		if( new_ip_connection_stat.ip_connection_array == NULL )
		{
			ErrorLog( __FILE__ , __LINE__ , "malloc failed , errno[%d]" , errno );
			return -1;
		}
		memset( new_ip_connection_stat.ip_connection_array , 0x00 , sizeof(struct IpConnection) * new_ip_connection_stat.ip_connection_stat_size );
		
		for( connection_index = 0 , p_ip_connection = p_ip_connection_stat->ip_connection_array ; connection_index < p_ip_connection_stat->ip_connection_stat_size ; connection_index++ , p_ip_connection++ )
		{
			if( p_ip_connection->used_flag == 1 )
			{
				nret = _AddIpConnectionStat( penv , & new_ip_connection_stat , p_ip_connection->ip_int , p_ip_connection_stat->connection_count ) ;
				if( nret )
				{
					ErrorLog( __FILE__ , __LINE__ , "AddIpConnectionStat failed[%d]" , nret );
					return -1;
				}
			}
		}
		
		free( p_ip_connection_stat->ip_connection_array );
		memcpy( p_ip_connection_stat , & new_ip_connection_stat , sizeof(struct IpConnectionStat) );
	}
	
	nret = _AddIpConnectionStat( penv , p_ip_connection_stat , ip_int , 1 ) ;
	
	return nret;
}

int RemoveIpConnectionStat( struct ServerEnv *penv , struct IpConnectionStat *p_ip_connection_stat , uint32_t ip_int )
{
	unsigned int		n ;
	unsigned int		ip_connection_index ;
	struct IpConnection	*p_ip_connection = NULL ;
	
	if( p_ip_connection_stat->ip_connection_stat_size == 0 )
	{
		return 0;
	}
	
	ip_connection_index = ip_int % (p_ip_connection_stat->ip_connection_stat_size) ;
	p_ip_connection = p_ip_connection_stat->ip_connection_array+ip_connection_index ;
	for( n = 0 ; n < p_ip_connection_stat->ip_connection_stat_size ; n++ )
	{
		if( p_ip_connection->used_flag == 1 && p_ip_connection->ip_int == ip_int )
		{
			__sync_fetch_and_sub( &(p_ip_connection->connection_count) , 1 );
			
			__sync_fetch_and_sub( &(p_ip_connection_stat->connection_count) , 1 );
			
			if( p_ip_connection->connection_count == 0 )
			{
				p_ip_connection->used_flag = 0 ;
				
				__sync_fetch_and_sub( &(p_ip_connection_stat->ip_count) , 1 );
			}
			
			return 0;
		}
		
		ip_connection_index++;
		p_ip_connection++;
		if( ip_connection_index >= p_ip_connection_stat->ip_connection_stat_size )
		{
			ip_connection_index = 0 ;
			p_ip_connection = p_ip_connection_stat->ip_connection_array ;
		}
	}
	
	ErrorLog( __FILE__ , __LINE__ , "invalid ip_int[%u]" , ip_int );
	
	return 1;
}
