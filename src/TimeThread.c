#include "G6.h"

void *TimeThread()
{
	struct ServerEnv	*penv = g_penv ;
	
	struct timeval		time_tv = { 0 , 0 } ;
	char			date_and_time[ sizeof(g_date_and_time) ] = "" ;
	
	/* 设置日志输出文件 */
	SetLogFile( penv->cmd_para.log_pathfilename );
	SetLogLevel( penv->cmd_para.log_level );
	UPDATE_TIME
	SETPID
	SETTID
	InfoLog( __FILE__ , __LINE__ , "--- G6.WorkerProcess.TimeThread ---" );
	
	/* 定时刷新全局秒戳 */
	while( g_exit_flag == 0 )
	{
		UpdateTimeNow( & time_tv , date_and_time );
		
		pthread_mutex_lock( & (penv->time_cache_mutex) );
		penv->time_tv.tv_sec = time_tv.tv_sec ;
		memcpy( penv->date_and_time , date_and_time , sizeof(date_and_time) );
		pthread_mutex_unlock( & (penv->time_cache_mutex) );
		
		sleep(1);
	}
	
	return NULL;
}

void *_TimeThread( void *pv )
{
	TimeThread();
	
	UPDATE_TIME
	InfoLog( __FILE__ , __LINE__ , "pthread_exit" );
	pthread_exit(NULL);
}

