#include "G6.h"

int MonitorProcess( struct ServerEnv *penv )
{
	int		nret = 0 ;
	
	SetLogFile( "%s/log/G6_MonitorProcess.log" , getenv("HOME") );
	SetLogLevel( LOGLEVEL_DEBUG );
	
	nret = LoadConfig( penv ) ;
	if( nret )
	{
		printf( "LoadConfig failed[%d]\n" , nret );
		return nret;
	}
	
	
	
	
	
	
	return 0;
}

