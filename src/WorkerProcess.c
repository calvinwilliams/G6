#include "G6.h"

int WorkerProcess( struct ServerEnv *penv )
{
	int			n ;
	struct epoll_event	event ;
	
	int			nret = 0 ;
	
	/* 设置日志环境 */
	SetLogFile( "%s/log/G6_WorkerProcess.log" , getenv("HOME") );
	SetLogLevel( penv->log_level );
	
	/* 创建转发子线程 */
	for( n = 0 ; n < penv->cmd_para.forward_thread_size ; n++ )
	{
		penv->forward_thread_index = (int*)malloc( sizeof(int) ) ;
		if( penv->forward_thread_index == NULL )
		{
			ErrorLog( __FILE__ , __LINE__ , "malloc failed , errno[%d]" , errno );
			return -1;
		}
		*(penv->forward_thread_index) = n + 1 ;
		
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
			ErrorLog( __FILE__ , __LINE__ , "pthread_create forward threads failed , errno[%d]" , errno );
			return -1;
		}
	}
	
	_AcceptThread( (void*)penv );
	
	return 0;
}

