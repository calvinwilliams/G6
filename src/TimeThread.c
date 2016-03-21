#include "G6.h"

void *TimeThread()
{
	struct ServerEnv	*penv = g_penv ;
	
	/* 设置日志输出文件 */
	SetLogFile( penv->cmd_para.log_pathfilename );
	SetLogLevel( penv->cmd_para.log_level );
	SETPID
	SETTID
	UPDATEDATETIMECACHE
	InfoLog( __FILE__ , __LINE__ , "--- G6.WorkerProcess.TimeThread ---" );
	
	/* 定时刷新全局秒戳 */
	while( g_exit_flag == 0 )
	{
		UPDATEDATETIMECACHE
		
		sleep(1);
	}
	
	return NULL;
}

void *_TimeThread( void *pv )
{
	TimeThread();
	
	UPDATEDATETIMECACHE
	InfoLog( __FILE__ , __LINE__ , "pthread_exit" );
	pthread_exit(NULL);
}

