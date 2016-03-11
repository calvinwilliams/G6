#include "G6.h"

/* 取随机数工具函数 */
int Rand( int min, int max )
{
	return ( rand() % ( max - min + 1 ) ) + min ;
}

/* 计算字符串HASH工具函数 */
unsigned long CalcHash( char *str )
{
	unsigned long	hashval ;
	unsigned char	*puc = NULL ;
	
	hashval = 19791007 ;
	for( puc = (unsigned char *)str ; *puc ; puc++ )
	{
		hashval = hashval * 19830923 + (*puc) ;
	}
	
	return hashval;
}

/* 设置sock重用选项 */
void SetReuseAddr( int sock )
{
	int	on = 1 ;
	
	setsockopt( sock , SOL_SOCKET , SO_REUSEADDR , (void *) & on, sizeof(on) );
	
	return;
}

/* 设置sock非堵塞选项 */
void SetNonBlocking( int sock )
{
#if ( defined __linux ) || ( defined __unix )
	int	opts;
	
	opts = fcntl( sock , F_GETFL ) ;
	opts = opts | O_NONBLOCK;
	fcntl( sock , F_SETFL , opts );
#elif ( defined _WIN32 )
	u_long	mode = 1 ;
	ioctlsocket( sock , FIONBIO , & mode );
#endif
	
	return;
}

/* 关闭算法 */
void SetNagleClosed( int sock )
{
	int	on = 1 ;
	
	setsockopt( sock , IPPROTO_TCP , TCP_NODELAY , (void*) & on , sizeof(int) );
	
	return;
}

/* 设置exec自动关闭选项 */
void SetCloseExec( int sock )
{
	int	val ;
	
	val = fcntl( sock , F_GETFD ) ;
	val |= FD_CLOEXEC ;
	fcntl( sock , F_SETFD , val );
	
	return;
}

void SetCloseExec2( int sock , int sock2 )
{
	SetCloseExec( sock );
	SetCloseExec( sock2 );
	return;
}

void SetCloseExec3( int sock , int sock2 , int sock3 )
{
	SetCloseExec( sock );
	SetCloseExec( sock2 );
	SetCloseExec( sock3 );
	return;
}

void SetCloseExec4( int sock , int sock2 , int sock3 , int sock4 )
{
	SetCloseExec( sock );
	SetCloseExec( sock2 );
	SetCloseExec( sock3 );
	SetCloseExec( sock4 );
	return;
}

/* 用IP和PORT设置sockaddr结构 */
void SetNetAddress( struct NetAddress *p_netaddr )
{
	memset( & (p_netaddr->sockaddr) , 0x00 , sizeof(struct sockaddr_in) );
	p_netaddr->sockaddr.sin_family = AF_INET ;
	p_netaddr->sockaddr.sin_addr.s_addr = inet_addr(p_netaddr->ip) ;
	p_netaddr->sockaddr.sin_port = htons( (unsigned short)(p_netaddr->port.port_int) );
	return;
}

/* 从sockaddr结构析出IP和PORT */
void GetNetAddress( struct NetAddress *p_netaddr )
{
	strcpy( p_netaddr->ip , inet_ntoa(p_netaddr->sockaddr.sin_addr) );
	p_netaddr->port.port_int = (int)ntohs( p_netaddr->sockaddr.sin_port ) ;
	return;
}

/* 转换为守护进程 */
int BindDaemonServer( char *pcServerName , int (* ServerMain)( void *pv ) , void *pv , int (* ControlMain)(long lControlStatus) )
{
	int pid;
	/*
	int sig;
	int fd;
	*/
	
	pid=fork();
	switch( pid )
	{
		case -1:
			return -1;
		case 0:
			break;
		default		:
			exit( 0 );	
			break;
	}

	setsid() ;
	/*
	signal( SIGHUP,SIG_IGN );
*/

	pid=fork();
	switch( pid )
	{
		case -1:
			return -2;
		case 0:
			break ;
		default:
			exit( 0 );
			break;
	}
	
	setuid( getpid() ) ;
	
	/* chdir("/tmp"); */
	
	umask( 0 ) ;
	
	/*
	for( sig=0 ; sig<30 ; sig++ )
		signal( sig , SIG_DFL );
	*/
	
	/*
	for( fd=0 ; fd<=2 ; fd++ )
		close( fd );
	*/
	
	return ServerMain( pv );
}

/* 判断字符串匹配性 */
int IsMatchString(char *pcMatchString, char *pcObjectString, char cMatchMuchCharacters, char cMatchOneCharacters)
{
	int el=strlen(pcMatchString);
	int sl=strlen(pcObjectString);
	char cs,ce;

	int is,ie;
	int last_xing_pos=-1;

	for(is=0,ie=0;is<sl && ie<el;){
		cs=pcObjectString[is];
		ce=pcMatchString[ie];

		if(cs!=ce){
			if(ce==cMatchMuchCharacters){
				last_xing_pos=ie;
				ie++;
			}else if(ce==cMatchOneCharacters){
				is++;
				ie++;
			}else if(last_xing_pos>=0){
				while(ie>last_xing_pos){
					ce=pcMatchString[ie];
					if(ce==cs)
						break;
					ie--;
				}

				if(ie==last_xing_pos)
					is++;
			}else
				return -1;
		}else{
			is++;
			ie++;
		}
	}

	if(pcObjectString[is]==0 && pcMatchString[ie]==0)
		return 0;

	if(pcMatchString[ie]==0)
		ie--;

	if(ie>=0){
		while(pcMatchString[ie])
			if(pcMatchString[ie++]!=cMatchMuchCharacters)
				return -2;
	} 

	return 0;
}

