#include "G6.h"

int			g_exit_flag = 0 ;

static void sig_proc( int sig_no )
{
	int		nret = 0 ;
	
	InfoLog( __FILE__ , __LINE__ , "recv signal[%d]" , sig_no );
	
	if( sig_no == SIGUSR2 )
	{
		pid_t			pid ;
		char			command ;
		
		int			nret = 0 ;
		
		nret = SaveListenSockets( g_penv ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "SaveListenSockets faild[%d]" , nret );
			return;
		}
		
		InfoLog( __FILE__ , __LINE__ , "write accept_request_pipe Q ..." );
		nret = write( g_penv->accept_request_pipe.fds[1] , "Q" , 1 ) ;
		InfoLog( __FILE__ , __LINE__ , "write accept_request_pipe Q done[%d]" , nret );
		
		InfoLog( __FILE__ , __LINE__ , "read accept_response_pipe ..." );
		nret = read( g_penv->accept_response_pipe.fds[0] , & command , 1 ) ;
		InfoLog( __FILE__ , __LINE__ , "read accept_response_pipe done[%d][%c]" , nret , command );
		
		pid = fork() ;
		if( pid == -1 )
		{
			;
		}
		else if( pid == 0 )
		{
			execvp( "G6" , g_penv->argv );
			
			exit(9);
		}
		
		g_exit_flag = 1 ;
		DebugLog( __FILE__ , __LINE__ , "set g_exit_flag[%d]" , g_exit_flag );
	}
	else if( sig_no == SIGTERM )
	{
		char			command ;
		
		InfoLog( __FILE__ , __LINE__ , "write accept_request_pipe Q ..." );
		nret = write( g_penv->accept_request_pipe.fds[1] , "Q" , 1 ) ;
		InfoLog( __FILE__ , __LINE__ , "write accept_request_pipe Q done[%d]" , nret );
		
		InfoLog( __FILE__ , __LINE__ , "read accept_response_pipe ..." );
		nret = read( g_penv->accept_response_pipe.fds[0] , & command , 1 ) ;
		InfoLog( __FILE__ , __LINE__ , "read accept_response_pipe done[%d][%c]" , nret , command );
		
		g_exit_flag = 1 ;
		DebugLog( __FILE__ , __LINE__ , "set g_exit_flag[%d]" , g_exit_flag );
	}
	
	return;
}

int MonitorProcess( struct ServerEnv *penv )
{
	pid_t			pid ;
	int			status ;
	
	int			nret = 0 ;
	
	/* 设置日志输出文件 */
	InfoLog( __FILE__ , __LINE__ , "--- G6.MonitorProcess ---" );
	
	/* 设置信号句柄 */
	signal( SIGCLD , SIG_DFL );
	signal( SIGCHLD , SIG_DFL );
	signal( SIGPIPE , SIG_IGN );
	g_penv = penv ;
	signal( SIGTERM , & sig_proc );
	signal( SIGUSR1 , SIG_IGN );
	signal( SIGUSR2 , & sig_proc );
	
	/* 主工作循环 */
	while( g_exit_flag == 0 )
	{
		sleep(1);
		
		/* 创建子进程 */
		_FORK :
		penv->pid = fork() ;
		if( penv->pid == -1 )
		{
			if( errno == EINTR )
				goto _FORK;
			
			ErrorLog( __FILE__ , __LINE__ , "fork failed , errno[%d]" , errno );
			return -1;
		}
		else if( penv->pid == 0 )
		{
			InfoLog( __FILE__ , __LINE__ , "child : [%ld]fork[%ld]" , getppid() , getpid() );
			
			close( penv->accept_request_pipe.fds[1] );
			close( penv->accept_response_pipe.fds[0] );
			
			nret = WorkerProcess( penv ) ;
			
			CleanEnvirment( penv );
			
			exit(-nret);
		}
		else
		{
			InfoLog( __FILE__ , __LINE__ , "parent : [%ld]fork[%ld]" , getpid() , penv->pid );
			
			close( penv->accept_request_pipe.fds[0] );
			close( penv->accept_response_pipe.fds[1] );
		}
		
		/* 监控子进程结束 */
		_WAITPID :
DebugLog( __FILE__ , __LINE__ , "111" );
		pid = waitpid( -1 , & status , 0 );
DebugLog( __FILE__ , __LINE__ , "222" );
		if( pid == -1 )
		{
			if( errno == EINTR )
				goto _WAITPID;
			
			ErrorLog( __FILE__ , __LINE__ , "waitpid failed , errno[%d]" , errno );
			close( penv->accept_request_pipe.fds[1] );
			close( penv->accept_response_pipe.fds[0] );
			return -1;
		}
		if( pid != penv->pid )
			goto _WAITPID;
		
		if( WCOREDUMP(status) )
		{
			ErrorLog( __FILE__ , __LINE__
				, "waitpid[%ld] WEXITSTATUS[%d] WIFSIGNALED[%d] WTERMSIG[%d] WCOREDUMP[%d]"
				, penv->pid , WEXITSTATUS(status) , WIFSIGNALED(status) , WTERMSIG(status) , WCOREDUMP(status) );
		}
		else
		{
			InfoLog( __FILE__ , __LINE__
				, "waitpid[%ld] WEXITSTATUS[%d] WIFSIGNALED[%d] WTERMSIG[%d] WCOREDUMP[%d]"
				, penv->pid , WEXITSTATUS(status) , WIFSIGNALED(status) , WTERMSIG(status) , WCOREDUMP(status) );
		}
	}
	
	return 0;
}

int _MonitorProcess( void *pv )
{
	return MonitorProcess( (struct ServerEnv *)pv );
}

