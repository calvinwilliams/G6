#include "G6.h"

static int MonitorProcess( struct ServerEnv *penv )
{
	int		status ;
	
	int		nret = 0 ;
	long		lret = 0 ;
	
	signal( SIGCLD , SIG_DFL );
	signal( SIGCHLD , SIG_DFL );
	signal( SIGPIPE , SIG_IGN );
	
	while(1)
	{
		nret = pipe( penv->monitor_pipe.fds ) ;
		if( nret == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "pipe failed , errno[%d]" , errno );
			return -1;
		}
		
		penv->pid = fork() ;
		if( penv->pid == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "fork failed , errno[%d]" , errno );
			close( penv->monitor_pipe.fds[0] );
			close( penv->monitor_pipe.fds[1] );
			return -1;
		}
		else if( penv->pid == 0 )
		{
			InfoLog( __FILE__ , __LINE__ , "child : [%ld]fork[%ld]" , getppid() , getpid() );
			
			nret = WorkerProcess( penv ) ;
			close( penv->monitor_pipe.fds[0] );
			exit(-nret);
		}
		else
		{
			close( penv->monitor_pipe.fds[1] );
			InfoLog( __FILE__ , __LINE__ , "parent : [%ld]fork[%ld]" , getpid() , penv->pid );
		}
		
		lret = waitpid( penv->pid , & status , 0 );
		if( lret == -1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "waitpid failed , errno[%d]" , errno );
			close( penv->monitor_pipe.fds[0] );
			close( penv->monitor_pipe.fds[1] );
			return -1;
		}
		
		close( penv->monitor_pipe.fds[0] );
		close( penv->monitor_pipe.fds[1] );
		
		InfoLog( __FILE__ , __LINE__
			, "waitpid[%ld] WEXITSTATUS[%d] WIFSIGNALED[%d] WTERMSIG[%d] WCOREDUMP[%d]"
			, penv->pid , WEXITSTATUS(status) , WIFSIGNALED(status) , WTERMSIG(status) , WCOREDUMP(status) );
	}
	
	return 0;
}

int _MonitorProcess( void *pv )
{
	return MonitorProcess( (struct ServerEnv *)pv );
}

