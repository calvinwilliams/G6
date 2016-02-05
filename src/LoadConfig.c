#include "G6.h"

static int LoadOneRule( struct ServerEnv *penv , FILE *fp , struct ForwardRule *p_forward_rule )
{
	int		nret = 0 ;
	
	/*
	(rule_id) (load_balance_algorithm) (client_ip1:client_port1,...) - (forward_ip1:forward_port1,...) > (server_ip1:server_port1,...) ;
	*/
	
	nret = fscanf( fp , "%.*[^ \t]" , RULE_ID_MAXLEN , p_forward_rule->rule_id ) ;
	if( nret == EOF )
		return EOF;
	
	nret = fscanf( fp , "%.*[^ \t]" , LOAD_BALANCE_ALGORITHM_MAXLEN , p_forward_rule->load_balance_algorithm ) ;
	if( nret == EOF )
		return -1;
	
	
	
	
	
	
	
	
	return 0;
}

int LoadConfig( struct ServerEnv *penv )
{
	FILE	*fp = NULL ;
	
	fp = fopen( penv->config_pathfilename , "r" ) ;
	if( fp == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "fopen[%s] failed , errno[%d]" , penv->config_pathfilename , errno );
		return -1;
	}
	
	while( ! feof(fp) )
	{
		if( penv->forward_rules == NULL || penv->forward_rules_count >= penv->forward_rules_size )
		{
			unsigned long		new_forward_rules_size ;
			struct ForwardRule	*new_forward_rules = NULL ;
			
			if( penv->forward_rules == NULL )
				new_forward_rules_size = DEFAULT_RULE_INITCOUNT ;
			else
				new_forward_rules_size = penv->forward_rules_size + DEFAULT_RULE_INCREASE ;
			
			new_forward_rules = realloc( penv->forward_rules , new_forward_rules_size ) ;
			if( new_forward_rules == NULL )
			{
				ErrorLog( __FILE__ , __LINE__ , "realloc failed , errno[%d]" , errno );
				return -1;
			}
			penv->forward_rules = new_forward_rules ;
			penv->forward_rules_size = new_forward_rules_size ;
			memset( penv->forward_rules + penv->forward_rules_count , 0x00 , sizeof(struct ForwardRule) * (penv->forward_rules_size-penv->forward_rules_count) );
		}
		
		nret = LoadOneRule( penv , fp , penv->forward_rules + penv->forward_rules_count ) ;
		if( nret == EOF )
			break;
		if( nret )
			return nret;
		
		penv->forward_rules_count++;
	}
	
	fclose( fp );
	
	return 0;
}

