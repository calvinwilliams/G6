#include "G6.h"

int WorkerProcess( struct ServerEnv *penv )
{
	unsigned long		n ;
	struct epoll_event	event ;
	
	int			nret = 0 ;
	
	/* 设置日志环境 */
	SetLogFile( "%s/log/G6_WorkerProcess.log" , getenv("HOME") );
	SetLogLevel( penv->log_level );
	InfoLog( __FILE__ , __LINE__ , "--- G6.WorkerProcess begin ---" );
	
	/* 创建转发子线程 */
	penv->forward_thread_index = NULL ;
DebugLog( __FILE__ , __LINE__ , "penv->cmd_para.forward_thread_size[%d]" , penv->cmd_para.forward_thread_size );
	for( n = 0 ; n < penv->cmd_para.forward_thread_size ; n++ )
	{
DebugLog( __FILE__ , __LINE__ , "n[%ld]" , n );
		while( penv->forward_thread_index ) sleep(1);
		
		penv->forward_thread_index = (int*)malloc( sizeof(int) ) ;
		if( penv->forward_thread_index == NULL )
		{
			ErrorLog( __FILE__ , __LINE__ , "malloc failed , errno[%d]" , errno );
			return -1;
		}
		*(penv->forward_thread_index) = n ;
		
		nret = pipe( penv->forward_pipe_array[n].fds ) ;
		if( nret == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "pipe failed , errno[%d]" , errno );
			return -1;
		}
		
		memset( & event , 0x00 , sizeof(event) );
		event.data.ptr = NULL ;
		event.events = EPOLLIN | EPOLLET ;
		epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_ADD , penv->forward_pipe_array[n].fds[0] , & event );
		
		nret = pthread_create( penv->forward_thread_tid_array+n , NULL , & _ForwardThread , (void*)penv ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "pthread_create forward thread failed , errno[%d]" , errno );
			return -1;
		}
		else
		{
			InfoLog( __FILE__ , __LINE__ , "parent [%lu] pthread_create [%lu]" , pthread_self() , penv->forward_thread_tid_array[n] );
		}
	}
	
	_AcceptThread( (void*)penv );
	
	InfoLog( __FILE__ , __LINE__ , "--- G6.WorkerProcess finish ---" );
	
	return 0;
}

