#include "G6.h"

static int LoadOneRule( struct ServerEnv *penv , FILE *fp , struct ForwardRule *p_forward_rule , char *rule_id )
{
	char				ip_and_port[ 100 + 1 ] ;
	struct ClientNetAddress		*p_client_addr = NULL ;
	struct ForwardNetAddress	*p_forward_addr = NULL ;
	struct ServerNetAddress		*p_server_addr = NULL ;
	
	int				nret = 0 ;
	
	/* 复制转发规则ID */
	strcpy( p_forward_rule->rule_id , rule_id );
	
	/* 读转发算法 */
	nret = fscanf( fp , "%2s" , p_forward_rule->load_balance_algorithm ) ;
	if( nret == EOF )
	{
		ErrorLog( __FILE__ , __LINE__ , "unexpect end of config" );
		return -11;
	}
	
	if( STRCMP( p_forward_rule->load_balance_algorithm , != , LOAD_BALANCE_ALGORITHM_G )
		&& STRCMP( p_forward_rule->load_balance_algorithm , != , LOAD_BALANCE_ALGORITHM_MS )
		&& STRCMP( p_forward_rule->load_balance_algorithm , != , LOAD_BALANCE_ALGORITHM_RR )
		&& STRCMP( p_forward_rule->load_balance_algorithm , != , LOAD_BALANCE_ALGORITHM_LC )
		&& STRCMP( p_forward_rule->load_balance_algorithm , != , LOAD_BALANCE_ALGORITHM_RT )
		&& STRCMP( p_forward_rule->load_balance_algorithm , != , LOAD_BALANCE_ALGORITHM_RD )
		&& STRCMP( p_forward_rule->load_balance_algorithm , != , LOAD_BALANCE_ALGORITHM_HS ) )
	{
		ErrorLog( __FILE__ , __LINE__ , "rule rule_mode [%s] invalid\r\n" , p_forward_rule->load_balance_algorithm );
		return -11;
	}
	
	/* 读客户端信息 */
	while(1)
	{
		memset( ip_and_port , 0x00 , sizeof(ip_and_port) );
		nret = fscanf( fp , "%s" , ip_and_port ) ;
		if( nret == EOF )
		{
			ErrorLog( __FILE__ , __LINE__ , "unexpect end of config in client_addr_array in rule[%s]" , p_forward_rule->rule_id );
			return -11;
		}
		
		if( STRCMP( ip_and_port , == , ";" ) )
		{
			ErrorLog( __FILE__ , __LINE__ , "unexpect end of config in client_addr_array in rule[%s]" , p_forward_rule->rule_id );
			return -11;
		}
		if( STRCMP( ip_and_port , == , "-" ) )
			break;
		
		if( p_forward_rule->client_addr_array == NULL || p_forward_rule->client_addr_count+1 > p_forward_rule->client_addr_size )
		{
			unsigned long		new_clients_maxcount ;
			struct ClientNetAddress	*new_clients_addr = NULL ;
			
			if( p_forward_rule->client_addr_array == NULL )
				new_clients_maxcount = DEFAULT_CLIENTS_INITCOUNT_IN_ONE_RULE ;
			else
				new_clients_maxcount = p_forward_rule->client_addr_size + DEFAULT_CLIENTS_INCREASE_IN_ONE_RULE ;
			
			new_clients_addr = realloc( p_forward_rule->client_addr_array , sizeof(struct ForwardRule) * new_clients_maxcount ) ;
			if( new_clients_addr == NULL )
			{
				ErrorLog( __FILE__ , __LINE__ , "realloc failed , errno[%d]" , errno );
				return -11;
			}
			p_forward_rule->client_addr_array = new_clients_addr ;
			p_forward_rule->client_addr_size = new_clients_maxcount ;
			memset( p_forward_rule->client_addr_array+p_forward_rule->client_addr_count , 0x00 , sizeof(struct ForwardRule) * (p_forward_rule->client_addr_size-p_forward_rule->client_addr_count) );
		}
		
		p_client_addr = p_forward_rule->client_addr_array + p_forward_rule->client_addr_count ;
		sscanf( ip_and_port , "%30[^:]:%10s" , p_client_addr->netaddr.ip , p_client_addr->netaddr.port.port_str );
		if( STRCMP( p_client_addr->netaddr.ip , == , "" ) || STRCMP( p_client_addr->netaddr.port.port_str , == , "" ) )
		{
			ErrorLog( __FILE__ , __LINE__ , "unexpect config in client_addr_array in rule[%s]" , p_forward_rule->rule_id );
			return -11;
		}
		
		p_forward_rule->client_addr_count++;
	}
	
	/* 读转发端信息 */
	while(1)
	{
		memset( ip_and_port , 0x00 , sizeof(ip_and_port) );
		nret = fscanf( fp , "%s" , ip_and_port ) ;
		if( nret == EOF )
		{
			ErrorLog( __FILE__ , __LINE__ , "unexpect end of config in forward_addr_array in rule[%s]" , p_forward_rule->rule_id );
			return -11;
		}
		
		if( STRCMP( ip_and_port , == , ";" ) )
		{
			if( STRCMP( p_forward_rule->load_balance_algorithm , == , LOAD_BALANCE_ALGORITHM_G ) )
				return 0;
			
			ErrorLog( __FILE__ , __LINE__ , "unexpect end of config in forward_addr_array in rule[%s]" , p_forward_rule->rule_id );
			return -11;
		}
		if( STRCMP( ip_and_port , == , ">" ) )
			break;
		
		if( p_forward_rule->forward_addr_array == NULL || p_forward_rule->forward_addr_count+1 > p_forward_rule->forward_addr_size )
		{
			unsigned long			new_forwards_addr_maxcount ;
			struct ForwardNetAddress	*new_forwards_addr = NULL ;
			
			if( p_forward_rule->forward_addr_array == NULL )
				new_forwards_addr_maxcount = DEFAULT_CLIENTS_INITCOUNT_IN_ONE_RULE ;
			else
				new_forwards_addr_maxcount = p_forward_rule->forward_addr_size + DEFAULT_CLIENTS_INCREASE_IN_ONE_RULE ;
			
			new_forwards_addr = realloc( p_forward_rule->forward_addr_array , sizeof(struct ForwardRule) * new_forwards_addr_maxcount ) ;
			if( new_forwards_addr == NULL )
			{
				ErrorLog( __FILE__ , __LINE__ , "realloc failed , errno[%d]" , errno );
				return -11;
			}
			p_forward_rule->forward_addr_array = new_forwards_addr ;
			p_forward_rule->forward_addr_size = new_forwards_addr_maxcount ;
			memset( p_forward_rule->forward_addr_array+p_forward_rule->forward_addr_count , 0x00 , sizeof(struct ForwardRule) * (p_forward_rule->forward_addr_size-p_forward_rule->forward_addr_count) );
		}
		
		p_forward_addr = p_forward_rule->forward_addr_array + p_forward_rule->forward_addr_count ;
		sscanf( ip_and_port , "%30[^:]:%d" , p_forward_addr->netaddr.ip , & (p_forward_addr->netaddr.port.port_int) );
		if( STRCMP( p_forward_addr->netaddr.ip , == , "" ) || p_forward_addr->netaddr.port.port_int == 0 )
		{
			ErrorLog( __FILE__ , __LINE__ , "unexpect config in forward_addr_array in rule[%s]" , p_forward_rule->rule_id );
			return -11;
		}
		
		SetNetAddress( & (p_forward_addr->netaddr) );
		
		p_forward_rule->forward_addr_count++;
	}
	
	/* 读服务端信息 */
	while(1)
	{
		memset( ip_and_port , 0x00 , sizeof(ip_and_port) );
		nret = fscanf( fp , "%s" , ip_and_port ) ;
		if( nret == EOF )
		{
			ErrorLog( __FILE__ , __LINE__ , "unexpect end of config in server_addr_array in rule[%s]" , p_forward_rule->rule_id );
			return -11;
		}
		
		if( STRCMP( ip_and_port , == , ";" ) )
			break;
		
		if( p_forward_rule->server_addr_array == NULL || p_forward_rule->server_addr_count+1 > p_forward_rule->server_addr_size )
		{
			unsigned long		new_servers_addr_size ;
			struct ServerNetAddress	*new_servers_addr = NULL ;
			
			if( p_forward_rule->server_addr_array == NULL )
				new_servers_addr_size = DEFAULT_CLIENTS_INITCOUNT_IN_ONE_RULE ;
			else
				new_servers_addr_size = p_forward_rule->server_addr_size + DEFAULT_CLIENTS_INCREASE_IN_ONE_RULE ;
			
			new_servers_addr = realloc( p_forward_rule->server_addr_array , sizeof(struct ForwardRule) * new_servers_addr_size ) ;
			if( new_servers_addr == NULL )
			{
				ErrorLog( __FILE__ , __LINE__ , "realloc failed , errno[%d]" , errno );
				return -11;
			}
			p_forward_rule->server_addr_array = new_servers_addr ;
			p_forward_rule->server_addr_size = new_servers_addr_size ;
			memset( p_forward_rule->server_addr_array+p_forward_rule->server_addr_count , 0x00 , sizeof(struct ForwardRule) * (p_forward_rule->server_addr_size-p_forward_rule->server_addr_count) );
		}
		
		p_server_addr = p_forward_rule->server_addr_array + p_forward_rule->server_addr_count ;
		sscanf( ip_and_port , "%30[^:]:%d" , p_server_addr->netaddr.ip , & (p_server_addr->netaddr.port.port_int) );
		if( STRCMP( p_server_addr->netaddr.ip , == , "" ) || p_server_addr->netaddr.port.port_int == 0 )
		{
			ErrorLog( __FILE__ , __LINE__ , "unexpect config in server_addr_array in rule[%s]" , p_forward_rule->rule_id );
			return -11;
		}
		
		SetNetAddress( & (p_server_addr->netaddr) );
		
		p_forward_rule->server_addr_count++;
	}
	
	return 0;
}

static void LogOneRule( struct ServerEnv *penv , struct ForwardRule *p_forward_rule )
{
	unsigned int			client_addr_index ;
	struct ClientNetAddress		*p_client_addr = NULL ;
	unsigned int			forward_addr_index ;
	struct ForwardNetAddress	*p_forward_addr = NULL ;
	unsigned int			server_addr_index ;
	struct ServerNetAddress		*p_server_addr = NULL ;
	
	/* 客户端配置信息 */
	for( client_addr_index = 0 , p_client_addr = p_forward_rule->client_addr_array ; client_addr_index < p_forward_rule->client_addr_count ; client_addr_index++ , p_client_addr++ )
	{
		InfoLog( __FILE__ , __LINE__ , "    [%s]:[%s]" , p_client_addr->netaddr.ip , p_client_addr->netaddr.port.port_str );
	}
	
	InfoLog( __FILE__ , __LINE__ , "    -" );
	
	/* 转发端配置信息 */
	for( forward_addr_index = 0 , p_forward_addr = p_forward_rule->forward_addr_array ; forward_addr_index < p_forward_rule->forward_addr_count ; forward_addr_index++ , p_forward_addr++ )
	{
		InfoLog( __FILE__ , __LINE__ , "    [%s]:[%d]" , p_forward_addr->netaddr.ip , p_forward_addr->netaddr.port.port_int );
	}
	
	InfoLog( __FILE__ , __LINE__ , "    >" );
	
	/* 服务端配置信息 */
	for( server_addr_index = 0 , p_server_addr = p_forward_rule->server_addr_array ; server_addr_index < p_forward_rule->server_addr_count ; server_addr_index++ , p_server_addr++ )
	{
		InfoLog( __FILE__ , __LINE__ , "    [%s]:[%d]" , p_server_addr->netaddr.ip , p_server_addr->netaddr.port.port_int );
	}
	
	InfoLog( __FILE__ , __LINE__ , "    ;" );
	
	return;
}

int LoadConfig( struct ServerEnv *penv )
{
	FILE			*fp = NULL ;
	char			rule_id[ RULE_ID_MAXLEN + 1 ] ;
	unsigned int		forward_rule_index ;
	struct ForwardRule	*p_forward_rule = NULL ;
	
	int			nret = 0 ;
	
	/* 打开配置文件 */
	fp = fopen( penv->cmd_para.config_pathfilename , "r" ) ;
	if( fp == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "fopen[%s] failed , errno[%d]" , penv->cmd_para.config_pathfilename , errno );
		return -1;
	}
	
	while( ! feof(fp) )
	{
		/* 读转发规则ID */
		nret = fscanf( fp , "%64s" , rule_id ) ;
		if( nret == EOF )
			break;
		
		if( STRCMP( rule_id , == , "(" ) )
		{
			char	property_name[ 64 + 1 ] ;
			char	sepchar[ 1 + 1 ] ;
			
			while(1)
			{
				nret = fscanf( fp , "%64s" , property_name ) ;
				if( nret == EOF )
				{
					ErrorLog( __FILE__ , __LINE__ , "unexpect end of config" );
					return -1;
				}
				
				nret = fscanf( fp , "%1s" , sepchar ) ;
				if( nret == EOF )
				{
					ErrorLog( __FILE__ , __LINE__ , "unexpect end of config" );
					return -1;
				}
				if( STRCMP( sepchar , != , "=" ) )
				{
					ErrorLog( __FILE__ , __LINE__ , "invalid config after property[%s]" , property_name );
					return -1;
				}
				
				if( STRCMP( property_name , == , "max_ip" ) )
				{
					nret = fscanf( fp , "%u" , &(penv->ip_connection_stat.max_ip) ) ;
					if( nret == EOF )
					{
						ErrorLog( __FILE__ , __LINE__ , "unexpect end of config" );
						return -1;
					}
					
					InfoLog( __FILE__ , __LINE__ , "set max_ip[%u]" , penv->ip_connection_stat.max_ip );
				}
				else if( STRCMP( property_name , == , "max_connections" ) )
				{
					nret = fscanf( fp , "%u" , &(penv->ip_connection_stat.max_connections) ) ;
					if( nret == EOF )
					{
						ErrorLog( __FILE__ , __LINE__ , "unexpect end of config" );
						return -1;
					}
					
					InfoLog( __FILE__ , __LINE__ , "set max_connections[%u]" , penv->ip_connection_stat.max_connections );
				}
				else if( STRCMP( property_name , == , "max_connections_per_ip" ) )
				{
					nret = fscanf( fp , "%u" , &(penv->ip_connection_stat.max_connections_per_ip) ) ;
					if( nret == EOF )
					{
						ErrorLog( __FILE__ , __LINE__ , "unexpect end of config" );
						return -1;
					}
					
					InfoLog( __FILE__ , __LINE__ , "set max_connections_per_ip[%u]" , penv->ip_connection_stat.max_connections_per_ip );
				}
				
				nret = fscanf( fp , "%1s" , sepchar ) ;
				if( nret == EOF )
				{
					ErrorLog( __FILE__ , __LINE__ , "unexpect end of config" );
					return -1;
				}
				if( STRCMP( sepchar , == , ")" ) )
				{
					break;
				}
				else if( STRCMP( sepchar , != , "," ) )
				{
					ErrorLog( __FILE__ , __LINE__ , "invalid config after property[%s]" , property_name );
					return -1;
				}
			}
			
			nret = fscanf( fp , "%1s" , sepchar ) ;
			if( nret == EOF )
			{
				ErrorLog( __FILE__ , __LINE__ , "unexpect end of config" );
				return -11;
			}
			if( STRCMP( sepchar , != , ";" ) )
			{
				ErrorLog( __FILE__ , __LINE__ , "invalid config after property[%s]" , property_name );
				return -11;
			}
			
			continue;
		}
		
		/* 调整转发规则数组 */
		if( penv->forward_rule_array == NULL || penv->forward_rule_count+1 > penv->forward_rule_size )
		{
			unsigned int		new_forward_rules_size ;
			struct ForwardRule	*new_forward_rules = NULL ;
			
			if( penv->forward_rule_array == NULL )
				new_forward_rules_size = DEFAULT_RULES_INITCOUNT ;
			else
				new_forward_rules_size = penv->forward_rule_size + DEFAULT_RULES_INCREASE ;
			
			new_forward_rules = realloc( penv->forward_rule_array , sizeof(struct ForwardRule) * new_forward_rules_size ) ;
			if( new_forward_rules == NULL )
			{
				ErrorLog( __FILE__ , __LINE__ , "realloc failed , errno[%d]" , errno );
				nret = -11 ;
				break;
			}
			penv->forward_rule_array = new_forward_rules ;
			penv->forward_rule_size = new_forward_rules_size ;
			memset( penv->forward_rule_array + penv->forward_rule_count , 0x00 , sizeof(struct ForwardRule) * (penv->forward_rule_size-penv->forward_rule_count) );
		}
		
		/* 装载一条转发规则 */
		nret = LoadOneRule( penv , fp , penv->forward_rule_array+penv->forward_rule_count , rule_id ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "LoadOneRule failed[%d]" , nret );
			return nret;
		}
		
		penv->forward_rule_count++;
	}
	
	/* 关闭配置文件 */
	fclose( fp );
	
	if( nret != EOF )
		return nret;
	
	InfoLog( __FILE__ , __LINE__ , "LoadOneRule ok" );
	
	/* 输出已装载转发规则到日志 */
	InfoLog( __FILE__ , __LINE__ , "forward_rule_count[%ld/%ld] >>>" , penv->forward_rule_count , penv->forward_rule_size );
	for( forward_rule_index = 0 , p_forward_rule = penv->forward_rule_array ; forward_rule_index < penv->forward_rule_count ; forward_rule_index++ , p_forward_rule++ )
	{
		InfoLog( __FILE__ , __LINE__ , "  rule_id[%s] load_balance_algorithm[%s] >>>"
			, p_forward_rule->rule_id , p_forward_rule->load_balance_algorithm );
		InfoLog( __FILE__ , __LINE__ , "  client_addr_count[%ld/%ld] forward_addr_count[%ld/%ld] server_addr_count[%ld/%ld]"
			, p_forward_rule->client_addr_count , p_forward_rule->client_addr_size
			, p_forward_rule->forward_addr_count , p_forward_rule->forward_addr_size
			, p_forward_rule->server_addr_count , p_forward_rule->server_addr_size );
		LogOneRule( penv , p_forward_rule );
	}
	
	return 0;
}

void UnloadConfig( struct ServerEnv *penv )
{
	unsigned int		forward_rule_index ;
	struct ForwardRule	*p_forward_rule = NULL ;
	
	if( penv->forward_rule_array )
	{
		for( forward_rule_index = 0 , p_forward_rule = penv->forward_rule_array ; forward_rule_index < penv->forward_rule_count ; forward_rule_index++ , p_forward_rule++ )
		{
			if( p_forward_rule->client_addr_array )
				free( p_forward_rule->client_addr_array );
			if( p_forward_rule->forward_addr_array )
				free( p_forward_rule->forward_addr_array );
			if( p_forward_rule->server_addr_array )
				free( p_forward_rule->server_addr_array );
		}
		
		free( penv->forward_rule_array );
	}
	
	return;
}
