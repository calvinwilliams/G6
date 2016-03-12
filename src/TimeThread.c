#include "G6.h"

void UpdateTimeNow( struct timeval *p_time_tv , char *p_date_and_time )
{
	struct tm		stime ;
	
	p_time_tv->tv_sec = time( NULL ) ;
	
#if ( defined __linux__ ) || ( defined __unix ) || ( defined _AIX )
	/*
	gettimeofday( & g_time_tv , NULL );
	*/
	localtime_r( &(p_time_tv->tv_sec) , & stime );
#elif ( defined _WIN32 )
	{
	SYSTEMTIME	stNow ;
	GetLocalTime( & stNow );
	time( & (p_time_tv->tv_sec) );
	/*
	g_time_tv.tv_usec = stNow.wMilliseconds * 1000 ;
	*/
	stime.tm_year = stNow.wYear - 1900 ;
	stime.tm_mon = stNow.wMonth - 1 ;
	stime.tm_mday = stNow.wDay ;
	stime.tm_hour = stNow.wHour ;
	stime.tm_min = stNow.wMinute ;
	stime.tm_sec = stNow.wSecond ;
	}
#endif
	
	strftime( p_date_and_time , sizeof(g_date_and_time) , "%Y-%m-%d %H:%M:%S" , & stime );
	
	return;
}

void *TimeThread()
{
	struct ServerEnv	*penv = g_penv ;
	
	struct timeval		time_tv = { 0 , 0 } ;
	char			date_and_time[ sizeof(g_date_and_time) ] = "" ;
	
	/* 设置日志输出文件 */
	SetLogFile( "%s/log/G6.log" , getenv("HOME") );
	SetLogLevel( penv->cmd_para.log_level );
	INIT_TIME
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
	
	INIT_TIME
	InfoLog( __FILE__ , __LINE__ , "pthread_exit" );
	pthread_exit(NULL);
}

