#include "G6.h"

static void version()
{
	printf( "G6 - TCP Transfer && Load-Balance Dispenser\n" );
	printf( "Copyright by calvin 2016\n" );
	return;
}

static void usage()
{
	printf( "USAGE : G6 -f config_pathfilename [ -t forward_thread_count ] [ -m session_max_count ]\n" );
	return;
}

int main( int argc , char *argv[] )
{
	struct ServerEnv	env , *penv = & env ;
	
	/* 设置标准输出无缓冲 */
	setbuf( stdout , NULL );
	
	/* 设置随机数种子 */
	srand( (unsigned)time(NULL) );
	
	/* 初始化服务器环境 */
	memset( penv , 0x00 , sizeof(struct ServerEnv) );
	
	/* 初始化命令行参数 */
	penv->cmd_para.forward_thread_count = 2 ;
	penv->cmd_para.forward_session_maxcount = DEFAULT_FORWARD_SESSIONS_MAXCOUNT ;
	
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
			penv->cmd_para.forward_thread_count = atol(argv[n]) ;
		}
		else if( strcmp( argv[n] , "-s" ) == 0 && n + 1 < argc )
		{
			n++;
			penv->cmd_para.forward_session_maxcount = atol(argv[n]) ;
		}
		else
		{
			fprintf( stderr , "invalid opt[%s]\r\n" , argv[n] );
			usage();
			exit(7);
		}
	}
	
	if( penv->cmd_para.config_pathfilename == NULL )
	{
		usage();
		exit(7);
	}
	
	nret = MonitorProcess( penv ) ;
	
	return -nret;
}

