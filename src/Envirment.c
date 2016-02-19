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
	
	penv->forward_pipe_array = (struct PipeFds *)malloc( sizeof(struct PipeFds) * penv->cmd_para.forward_thread_size ) ;
	if( penv->forward_pipe_array == NULL )
	{
		printf( "malloc failed , errno[%d]" , errno );
		return -1;
	}
	memset( penv->forward_pipe_array , 0x00 , sizeof(struct PipeFds) * penv->cmd_para.forward_thread_size );
	
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
	
	return 0;
}

void CleanEnvirment( struct ServerEnv *penv )
{
	int		n ;
	
	close( penv->accept_epoll_fd );
	free( penv->forward_thread_tid_array );
	free( penv->forward_session_array );
	
	for( n = 0 ; n < penv->cmd_para.forward_thread_size ; n++ )
	{
		close( penv->forward_epoll_fd_array[n] );
	}
	
	return;
}

struct ForwardSession *GetForwardSessionUnused( struct ServerEnv *penv )
{
	struct ForwardSession	*p_forward_session = penv->forward_session_array + penv->forward_session_use_offsetpos ;
	
	while(1)
	{
		if( p_forward_session->status == FORWARD_SESSION_STATUS_UNUSED )
		{
			memset( p_forward_session , 0x00 , sizeof(struct ForwardSession) );
			p_forward_session->status = FORWARD_SESSION_STATUS_READY ;
			penv->forward_session_use_offsetpos++;
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

void SetForwardSessionUnused( struct ForwardSession *p_forward_session )
{
	if( p_forward_session->status != FORWARD_SESSION_STATUS_UNUSED )
	{
		if( p_forward_session->p_reverse_forward_session || p_forward_session->p_reverse_forward_session->status != FORWARD_SESSION_STATUS_UNUSED )
		{
			DebugLog( __FILE__ , __LINE__ , "close #%d# #%d#" , p_forward_session->sock , p_forward_session->p_reverse_forward_session->sock );
			_CLOSESOCKET( p_forward_session->sock );
			_CLOSESOCKET( p_forward_session->p_reverse_forward_session->sock );
			p_forward_session->p_reverse_forward_session->status = FORWARD_SESSION_STATUS_UNUSED ;
		}
		else
		{
			DebugLog( __FILE__ , __LINE__ , "close #%d#" , p_forward_session->sock );
			_CLOSESOCKET( p_forward_session->sock );
		}
		
		p_forward_session->status = FORWARD_SESSION_STATUS_UNUSED ;
	}
	
	return;
}
