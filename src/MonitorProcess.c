#include "G6.h"

static int MonitorProcess( struct ServerEnv *penv )
{
	struct epoll_event	event ;
	
	int			status ;
	
	int			nret = 0 ;
	long			lret = 0 ;
	
	/* 设置日志环境 */
	SetLogFile( "%s/log/G6_MonitorProcess.log" , getenv("HOME") );
	SetLogLevel( penv->log_level );
	
	/* 设置信号句柄 */
	signal( SIGCLD , SIG_DFL );
	signal( SIGCHLD , SIG_DFL );
	signal( SIGPIPE , SIG_IGN );
	
	/* 主工作循环 */
	while(1)
	{
		/* 创建管道 */
		nret = pipe( penv->accept_pipe.fds ) ;
		if( nret == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "pipe failed , errno[%d]" , errno );
			return -1;
		}
		
		/* 将管道读端加入到侦听端口epoll池 */
		memset( & event , 0x00 , sizeof(event) );
		event.data.ptr = NULL ;
		event.events = EPOLLIN | EPOLLET ;
		epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_ADD , penv->accept_pipe.fds[0] , & event );
		
		/* 创建子进程 */
		penv->pid = fork() ;
		if( penv->pid == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "fork failed , errno[%d]" , errno );
			close( penv->accept_pipe.fds[0] );
			close( penv->accept_pipe.fds[1] );
			return -1;
		}
		else if( penv->pid == 0 )
		{
			InfoLog( __FILE__ , __LINE__ , "child : [%ld]fork[%ld]" , getppid() , getpid() );
			
			nret = WorkerProcess( penv ) ;
			close( penv->accept_pipe.fds[0] );
			exit(-nret);
		}
		else
		{
			close( penv->accept_pipe.fds[1] );
			InfoLog( __FILE__ , __LINE__ , "parent : [%ld]fork[%ld]" , getpid() , penv->pid );
		}
		
		/* 监控子进程结束 */
		lret = waitpid( penv->pid , & status , 0 );
		if( lret == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "waitpid failed , errno[%d]" , errno );
			close( penv->accept_pipe.fds[0] );
			close( penv->accept_pipe.fds[1] );
			return -1;
		}
		
		close( penv->accept_pipe.fds[0] );
		close( penv->accept_pipe.fds[1] );
		
		InfoLog( __FILE__ , __LINE__
			, "waitpid[%ld] WEXITSTATUS[%d] WIFSIGNALED[%d] WTERMSIG[%d] WCOREDUMP[%d]"
			, penv->pid , WEXITSTATUS(status) , WIFSIGNALED(status) , WTERMSIG(status) , WCOREDUMP(status) );
		
		/* 重启子进程 */
	}
	
	return 0;
}

int _MonitorProcess( void *pv )
{
	return MonitorProcess( (struct ServerEnv *)pv );
}

