#include "G6.h"

int			g_exit_flag = 0 ;

static void sig_proc( int sig_no )
{
	if( sig_no == SIGUSR1 )
	{
		int			forward_session_index ;
		struct ForwardSession	*p_forward_session = NULL ;
		
		char			command ;
		
		pid_t			pid ;
		
DebugLog( __FILE__ , __LINE__ , "g_exit_flag[%d]" , g_exit_flag );
		for( forward_session_index = 0 , p_forward_session = g_penv->forward_session_array ; forward_session_index < g_penv->cmd_para.forward_session_size ; forward_session_index++ , p_forward_session++ )
		{
DebugLog( __FILE__ , __LINE__ , "g_exit_flag[%d]" , g_exit_flag );
			if( p_forward_session->type != FORWARD_SESSION_TYPE_UNUSED )
			{
DebugLog( __FILE__ , __LINE__ , "g_exit_flag[%d]" , g_exit_flag );
				if( p_forward_session->type == FORWARD_SESSION_TYPE_LISTEN )
				{
DebugLog( __FILE__ , __LINE__ , "g_exit_flag[%d]" , g_exit_flag );
					epoll_ctl( g_penv->accept_epoll_fd , EPOLL_CTL_DEL , p_forward_session->sock , NULL );
					DebugLog( __FILE__ , __LINE__ , "close #%d#" , p_forward_session->sock );
					_CLOSESOCKET( p_forward_session->sock );
					SetForwardSessionUnused( g_penv , p_forward_session );
DebugLog( __FILE__ , __LINE__ , "g_exit_flag[%d]" , g_exit_flag );
				}
DebugLog( __FILE__ , __LINE__ , "g_exit_flag[%d]" , g_exit_flag );
			}
DebugLog( __FILE__ , __LINE__ , "g_exit_flag[%d]" , g_exit_flag );
		}
DebugLog( __FILE__ , __LINE__ , "g_exit_flag[%d]" , g_exit_flag );
		
		write( g_penv->request_pipe.fds[1] , "Q" , 1 );
DebugLog( __FILE__ , __LINE__ , "g_exit_flag[%d]" , g_exit_flag );
		read( g_penv->request_pipe.fds[0] , & command , 1 );
DebugLog( __FILE__ , __LINE__ , "g_exit_flag[%d]" , g_exit_flag );
		
		pid = fork() ;
		if( pid == -1 )
		{
			;
		}
		else if( pid == 0 )
		{
			CleanEnvirment( g_penv );
			
			execvp( "G6" , g_penv->argv );
			
			exit(9);
		}
		
DebugLog( __FILE__ , __LINE__ , "g_exit_flag[%d]" , g_exit_flag );
		g_exit_flag = 1 ;
	}
	else if( sig_no == SIGTERM )
	{
		char			command ;
		
		write( g_penv->request_pipe.fds[1] , "Q" , 1 );
		read( g_penv->request_pipe.fds[0] , & command , 1 );
		
		g_exit_flag = 1 ;
	}
	
	return;
}

int MonitorProcess( struct ServerEnv *penv )
{
	struct epoll_event	event ;
	
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
	signal( SIGTERM , sig_proc );
	signal( SIGUSR1 , sig_proc );
	
	/* 主工作循环 */
	while( g_exit_flag == 0 )
	{
		sleep(1);
		
		/* 创建管道 */
		_PIPE1 :
		nret = pipe( penv->request_pipe.fds ) ;
		if( nret == -1 )
		{
			if( errno == EINTR )
				goto _PIPE1;
			
			ErrorLog( __FILE__ , __LINE__ , "pipe failed , errno[%d]" , errno );
			return -1;
		}
		
		_PIPE2 :
		nret = pipe( penv->response_pipe.fds ) ;
		if( nret == -1 )
		{
			if( errno == EINTR )
				goto _PIPE2;
			
			ErrorLog( __FILE__ , __LINE__ , "pipe failed , errno[%d]" , errno );
			return -1;
		}
		
		/* 将管道读端加入到侦听端口epoll池 */
		memset( & event , 0x00 , sizeof(event) );
		event.data.ptr = NULL ;
		event.events = EPOLLIN | EPOLLERR ;
		epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_ADD , penv->request_pipe.fds[0] , & event );
		
		/* 创建子进程 */
		_FORK :
DebugLog( __FILE__ , __LINE__ , "g_exit_flag[%d]" , g_exit_flag );
		penv->pid = fork() ;
		if( penv->pid == -1 )
		{
DebugLog( __FILE__ , __LINE__ , "g_exit_flag[%d]" , g_exit_flag );
			if( errno == EINTR )
				goto _FORK;
			
			ErrorLog( __FILE__ , __LINE__ , "fork failed , errno[%d]" , errno );
			close( penv->request_pipe.fds[0] );
			close( penv->request_pipe.fds[1] );
			close( penv->response_pipe.fds[0] );
			close( penv->response_pipe.fds[1] );
			return -1;
		}
		else if( penv->pid == 0 )
		{
			InfoLog( __FILE__ , __LINE__ , "child : [%ld]fork[%ld]" , getppid() , getpid() );
			
			close( penv->request_pipe.fds[1] );
			close( penv->response_pipe.fds[0] );
			
			nret = WorkerProcess( penv ) ;
			exit(-nret);
		}
		else
		{
			InfoLog( __FILE__ , __LINE__ , "parent : [%ld]fork[%ld]" , getpid() , penv->pid );
			
			close( penv->request_pipe.fds[0] );
			close( penv->response_pipe.fds[1] );
		}
DebugLog( __FILE__ , __LINE__ , "g_exit_flag[%d]" , g_exit_flag );
		
		/* 监控子进程结束 */
		_WAITPID :
		pid = waitpid( -1 , & status , 0 );
		if( pid == -1 )
		{
			if( errno == EINTR )
				goto _WAITPID;
			
			ErrorLog( __FILE__ , __LINE__ , "waitpid failed , errno[%d]" , errno );
			close( penv->request_pipe.fds[0] );
			close( penv->request_pipe.fds[1] );
			close( penv->response_pipe.fds[0] );
			close( penv->response_pipe.fds[1] );
			return -1;
		}
		if( pid != penv->pid )
			goto _WAITPID;
DebugLog( __FILE__ , __LINE__ , "g_exit_flag[%d]" , g_exit_flag );
		
		close( penv->request_pipe.fds[0] );
		close( penv->request_pipe.fds[1] );
		close( penv->response_pipe.fds[0] );
		close( penv->response_pipe.fds[1] );
		
		InfoLog( __FILE__ , __LINE__
			, "waitpid[%ld] WEXITSTATUS[%d] WIFSIGNALED[%d] WTERMSIG[%d] WCOREDUMP[%d]"
			, penv->pid , WEXITSTATUS(status) , WIFSIGNALED(status) , WTERMSIG(status) , WCOREDUMP(status) );
	}
DebugLog( __FILE__ , __LINE__ , "g_exit_flag[%d]" , g_exit_flag );
	
	return 0;
}

int _MonitorProcess( void *pv )
{
	return MonitorProcess( (struct ServerEnv *)pv );
}

