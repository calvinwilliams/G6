#include "G6.h"

char			__G6_VERSION[] = "1.0.0" ;
char			*__G6_VERSION_1_0_0 = __G6_VERSION ;

struct ServerEnv	*g_penv = NULL ;

static void version()
{
	printf( "G6 v%s build %s %s\n" , __G6_VERSION , __DATE__ , __TIME__ );
	printf( "TCP Bridge && Load-Balance Dispenser\n" );
	printf( "Copyright by calvin 2016\n" );
	return;
}

static void usage()
{
	printf( "USAGE : G6 -f (config_pathfilename) [ -t (forward_thread_size) ] [ -s (forward_session_size) ] [ --log-level (DEBUG|INFO|WARN|ERROR|FATAL) ] [ --log-filename (logfilename) ] [ --no-daemon ]\n" );
	return;
}

int main( int argc , char *argv[] )
{
	struct ServerEnv	env , *penv = & env ;
	int			n ;
	
	int			nret = 0 ;
	
	/* 设置标准输出无缓冲 */
	setbuf( stdout , NULL );
	
	/* 设置随机数种子 */
	srand( (unsigned)time(NULL) );
	
	/* 初始化服务器环境 */
	memset( penv , 0x00 , sizeof(struct ServerEnv) );
	g_penv = penv ;
	
	/* 初始化命令行参数 */
	memset( & (penv->cmd_para) , 0x00 , sizeof(struct CommandParameter) );
	penv->cmd_para.config_pathfilename = NULL ;
	penv->cmd_para.forward_thread_size = sysconf(_SC_NPROCESSORS_ONLN) - 2 ;
	if( penv->cmd_para.forward_thread_size < 1 )
		penv->cmd_para.forward_thread_size = 1 ;
	penv->cmd_para.forward_session_size = DEFAULT_FORWARD_SESSIONS_MAXCOUNT ;
	penv->cmd_para.log_level = LOGLEVEL_INFO ;
	snprintf( penv->cmd_para.log_pathfilename , sizeof(penv->cmd_para.log_pathfilename)-1 , "%s/log/G6.log" , getenv("HOME") );
	penv->cmd_para.no_daemon_flag = 0 ;
	
	/* 设置日志输出文件 */
	SetLogFile( penv->cmd_para.log_pathfilename );
	SetLogLevel( penv->cmd_para.log_level );
	
	/* 分析命令行参数 */
	if( argc == 1 )
	{
		version();
		usage();
		exit(7);
	}
	
	for( n = 1 ; n < argc ; n++ )
	{
		if( strcmp( argv[n] , "-v" ) == 0 && 1 + 1 == argc )
		{
			version();
			exit(0);
		}
		else if( strcmp( argv[n] , "-f" ) == 0 && n + 1 < argc )
		{
			n++;
			penv->cmd_para.config_pathfilename = argv[n] ;
		}
		else if( strcmp( argv[n] , "-t" ) == 0 && n + 1 < argc )
		{
			n++;
			penv->cmd_para.forward_thread_size = atoi(argv[n]) ;
		}
		else if( strcmp( argv[n] , "-s" ) == 0 && n + 1 < argc )
		{
			n++;
			penv->cmd_para.forward_session_size = atoi(argv[n]) ;
		}
		else if( strcmp( argv[n] , "--log-level" ) == 0 && n + 1 < argc )
		{
			n++;
			if( strcmp( argv[n] , "DEBUG" ) == 0 )
				penv->cmd_para.log_level = LOGLEVEL_DEBUG ;
			else if( strcmp( argv[n] , "INFO" ) == 0 )
				penv->cmd_para.log_level = LOGLEVEL_INFO ;
			else if( strcmp( argv[n] , "WARN" ) == 0 )
				penv->cmd_para.log_level = LOGLEVEL_WARN ;
			else if( strcmp( argv[n] , "ERROR" ) == 0 )
				penv->cmd_para.log_level = LOGLEVEL_ERROR ;
			else if( strcmp( argv[n] , "FATAL" ) == 0 )
				penv->cmd_para.log_level = LOGLEVEL_FATAL ;
			else
			{
				fprintf( stderr , "invalid log level[%s]\r\n" , argv[n] );
				usage();
				exit(7);
			}
			
			SetLogLevel( penv->cmd_para.log_level );
		}
		else if( strcmp( argv[n] , "--log-filename" ) == 0 && n + 1 < argc )
		{
			n++;
			snprintf( penv->cmd_para.log_pathfilename , sizeof(penv->cmd_para.log_pathfilename)-1 , argv[n] );
			SetLogFile( penv->cmd_para.log_pathfilename );
		}
		else if( strcmp( argv[n] , "--no-daemon" ) == 0 )
		{
			penv->cmd_para.no_daemon_flag = 1 ;
		}
		else if( strcmp( argv[n] , "--close-log" ) == 0 )
		{
			penv->cmd_para.close_log_flag = 1 ;
		}
		else
		{
			fprintf( stderr , "invalid opt[%s]\r\n" , argv[n] );
			usage();
			exit(7);
		}
	}
	penv->argv = argv ;
	
	if( penv->cmd_para.config_pathfilename == NULL )
	{
		usage();
		exit(7);
	}
	
	UPDATE_TIME
	SETPID
	SETTID
	
	InfoLog( __FILE__ , __LINE__ , "--- G6 BEGIN --- v%s build %s %s" , __G6_VERSION , __DATE__ , __TIME__ );
	
	/* 初始化环境 */
	nret = InitEnvirment( penv ) ;
	if( nret )
	{
		printf( "InitEnvirment failed[%d]\n" , nret );
		return -nret;
	}
	
	/* 设置公共参数 */
	/*
	penv->timeout = DEFAULT_TIMEOUT_SECONDS ;
	*/
	
	/* 装载配置 */
	nret = LoadConfig( penv ) ;
	if( nret )
	{
		printf( "LoadConfig failed[%d]\n" , nret );
		return -nret;
	}
	
	/* 创建所有侦听端口 */
	nret = AddListeners( penv ) ;
	if( nret )
	{
		printf( "AddListeners failed[%d]\n" , nret );
		return -nret;
	}
	
	/* 进入监控父进程 */
	if( penv->cmd_para.no_daemon_flag )
	{
		nret = MonitorProcess( penv ) ;
	}
	else
	{
		nret = BindDaemonServer( NULL , & _MonitorProcess , penv , NULL ) ;
	}
	
	/* 卸载配置 */
	UnloadConfig( penv );
	
	/* 清理环境 */
	CleanEnvirment( penv );
	
	InfoLog( __FILE__ , __LINE__ , "--- G6 FINISH --- v%s build %s %s" , __G6_VERSION , __DATE__ , __TIME__ );
	
	return -nret;
}

