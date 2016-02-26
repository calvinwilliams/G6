/*
 * G6 - TCP Transfer && LB Dispenser
 * Author      : calvin
 * Email       : calvinwillliams@163.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#ifndef _H_G6_
#define _H_G6_

#if ( defined __linux ) || ( defined __unix )
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>
#include <limits.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#define _VSNPRINTF			vsnprintf
#define _SNPRINTF			snprintf
#define _CLOSESOCKET			close
#define _CLOSESOCKET2(_fd1_,_fd2_)	close(_fd1_),close(_fd2_);
#define _ERRNO				errno
#define _EWOULDBLOCK			EWOULDBLOCK
#define _ECONNABORTED			ECONNABORTED
#define _EINPROGRESS			EINPROGRESS
#define _ECONNRESET			ECONNRESET
#define _ENOTCONN			ENOTCONN
#define _EISCONN			EISCONN
#define _SOCKLEN_T			socklen_t
#define _GETTIMEOFDAY(_tv_)		gettimeofday(&(_tv_),NULL)
#define _LOCALTIME(_tt_,_stime_) \
	localtime_r(&(_tt_),&(_stime_));
#elif ( defined _WIN32 )
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <io.h>
#include <windows.h>
#define _VSNPRINTF			_vsnprintf
#define _SNPRINTF			_snprintf
#define _CLOSESOCKET			closesocket
#define _CLOSESOCKET2(_fd1_,_fd2_)	closesocket(_fd1_),closesocket(_fd2_);
#define _ERRNO				GetLastError()
#define _EWOULDBLOCK			WSAEWOULDBLOCK
#define _ECONNABORTED			WSAECONNABORTED
#define _EINPROGRESS			WSAEINPROGRESS
#define _ECONNRESET			WSAECONNRESET
#define _ENOTCONN			WSAENOTCONN
#define _EISCONN			WSAEISCONN
#define _SOCKLEN_T			int
#define _GETTIMEOFDAY(_tv_) \
	{ \
		SYSTEMTIME stNow ; \
		GetLocalTime( & stNow ); \
		(_tv_).tv_usec = stNow.wMilliseconds * 1000 ; \
		time( & ((_tv_).tv_sec) ); \
	}
#define _SYSTEMTIME2TIMEVAL_USEC(_syst_,_tv_) \
		(_tv_).tv_usec = (_syst_).wMilliseconds * 1000 ;
#define _SYSTEMTIME2TM(_syst_,_stime_) \
		(_stime_).tm_year = (_syst_).wYear - 1900 ; \
		(_stime_).tm_mon = (_syst_).wMonth - 1 ; \
		(_stime_).tm_mday = (_syst_).wDay ; \
		(_stime_).tm_hour = (_syst_).wHour ; \
		(_stime_).tm_min = (_syst_).wMinute ; \
		(_stime_).tm_sec = (_syst_).wSecond ;
#define _LOCALTIME(_tt_,_stime_) \
	{ \
		SYSTEMTIME	stNow ; \
		GetLocalTime( & stNow ); \
		_SYSTEMTIME2TM( stNow , (_stime_) ); \
	}
#endif

#include "LOGC.h"

#ifndef ULONG_MAX
#define ULONG_MAX 0xffffffffUL
#endif

#define FOUND				9	/* ÕÒµ½ */
#define NOT_FOUND			4	/* ÕÒ²»µ½ */

#define MATCH				1	/* Æ¥Åä */
#define NOT_MATCH			-1	/* ²»Æ¥Åä */

#define LOAD_BALANCE_ALGORITHM_G	"G"	/* ¹ÜÀí¶Ë¿Ú */
#define LOAD_BALANCE_ALGORITHM_MS	"MS"	/* Ö÷±¸Ä£Ê½ */
#define LOAD_BALANCE_ALGORITHM_RR	"RR"	/* ÂÖÑ¯Ä£Ê½ */
#define LOAD_BALANCE_ALGORITHM_LC	"LC"	/* ×îÉÙÁ¬½ÓÄ£Ê½ */
#define LOAD_BALANCE_ALGORITHM_RT	"RT"	/* ×îĞ¡ÏìÓ¦Ê±¼äÄ£Ê½ */
#define LOAD_BALANCE_ALGORITHM_RD	"RD"	/* Ëæ»úÄ£Ê½ */
#define LOAD_BALANCE_ALGORITHM_HS	"HS"	/* HASHÄ£Ê½ */

#define DEFAULT_RULES_INITCOUNT			2
#define DEFAULT_RULES_INCREASE			5

#define DEFAULT_CLIENTS_INITCOUNT_IN_ONE_RULE	2
#define DEFAULT_CLIENTS_INCREASE_IN_ONE_RULE	5
#define DEFAULT_FORWARDS_INITCOUNT_IN_ONE_RULE	2
#define DEFAULT_FORWARDS_INCREASE_IN_ONE_RULE	5
#define DEFAULT_SERVERS_INITCOUNT_IN_ONE_RULE	2
#define DEFAULT_SERVERS_INCREASE_IN_ONE_RULE	5

#define DEFAULT_FORWARD_SESSIONS_MAXCOUNT	10000	/* È±Ê¡×î´óÁ¬½ÓÊıÁ¿ */
#define DEFAULT_FORWARD_TRANSFER_BUFSIZE	4096	/* È±Ê¡Í¨Ñ¶×ª·¢»º³åÇø´óĞ¡ */

#define RULE_ID_MAXLEN				64
#define LOAD_BALANCE_ALGORITHM_MAXLEN		2
#define IP_MAXLEN				30
#define PORT_MAXLEN				10

#define WAIT_EVENTS_COUNT			1024 /* Ò»´Î»ñÈ¡epollÊÂ¼şÊıÁ¿ */

#define FORWARD_SESSION_TYPE_CLIENT		1
#define FORWARD_SESSION_TYPE_LISTEN		2
#define FORWARD_SESSION_TYPE_SERVER		3

#define FORWARD_SESSION_STATUS_UNUSED		0 /* Î´Ê¹ÓÃ */
#define FORWARD_SESSION_STATUS_READY		1 /* ×¼±¸Ê¹ÓÃ */
#define FORWARD_SESSION_STATUS_LISTEN		9 /* ÕìÌıÖĞ */
#define FORWARD_SESSION_STATUS_CONNECTING	11 /* ·Ç¶ÂÈûÁ¬½ÓÖĞ */
#define FORWARD_SESSION_STATUS_CONNECTED	12 /* Á¬½ÓÍê³É */

#define IO_BUFFER_SIZE				4096 /* ÊäÈëÊä³ö»º³åÇø´óĞ¡ */

#define DEFAULT_DISABLE_TIMEOUT			60 /* È±Ê¡Ôİ½ûÊ±¼ä£¬µ¥Î»£ºÃë */

/* ÍøÂçµØÖ·ĞÅÏ¢½á¹¹ */
struct NetAddress
{
	char			ip[ IP_MAXLEN + 1 ] ; /* ipµØÖ· */
	union
	{
		char		port_str[ PORT_MAXLEN + 1 ] ;
		int		port_int ;
	} port ; /* ¶Ë¿Ú */
	struct sockaddr_in	sockaddr ; /* sockµØÖ·½á¹¹ */
} ;

/* ¿Í»§¶ËĞÅÏ¢½á¹¹ */
struct ClientNetAddress
{
	struct NetAddress	netaddr ; /* ÍøÂçµØÖ·½á¹¹ */
	
	unsigned long		client_connection_count ; /* ¿Í»§¶ËÁ¬½ÓÊıÁ¿ */
	unsigned long		maxclients ; /* ×î´ó¿Í»§¶ËÊıÁ¿ */
} ;

/* ×ª·¢¶ËĞÅÏ¢½á¹¹ */
struct ForwardNetAddress
{
	struct NetAddress	netaddr ; /* ÍøÂçµØÖ·½á¹¹ */
	int			sock ; /* sockÃèÊö×Ö */
} ;

/* ·şÎñ¶ËĞÅÏ¢½á¹¹ */
struct ServerNetAddress
{
	struct NetAddress	netaddr ; /* ÍøÂçµØÖ·½á¹¹ */
	
	unsigned char		server_unable ; /* ·şÎñ¶Ë¿ÉÓÃ±êÖ¾ */
	time_t			timestamp_to_enable ; /* »Ö¸´Õı³£Ãë´Á */
	
	unsigned long		server_connection_count ; /* ·şÎñ¶ËÁ¬½ÓÊıÁ¿ */
	
	struct timeval		tv1 ; /* ×î½ü¶ÁÊ±¼ä´Á */
	struct timeval		tv2 ; /* ×î½üĞ´Ê±¼ä´Á */
	struct timeval		dtv ; /* ×î½ü¶ÁĞ´Ê±¼ä´Á²î */
} ;

/* ×ª·¢¹æÔò½á¹¹ */
struct ForwardRule
{
	char				rule_id[ RULE_ID_MAXLEN + 1 ] ; /* ¹æÔòID © */
	char				load_balance_algorithm[ LOAD_BALANCE_ALGORITHM_MAXLEN + 1 ] ; /* ¸ºÔØ¾ùºâËã·¨ */
	
	struct ClientNetAddress		*clients_addr ; /* ¿Í»§¶ËµØÖ·½á¹¹ */
	unsigned long			clients_addr_size ; /* ¿Í»§¶Ë¹æÔòÅäÖÃ×î´óÊıÁ¿ */
	unsigned long			clients_addr_count ; /* ¿Í»§¶Ë¹æÔòÅäÖÃÊıÁ¿ */
	
	struct ForwardNetAddress	*forwards_addr ; /* ×ª·¢¶ËµØÖ·½á¹¹ */
	unsigned long			forwards_addr_size ; /* ×ª·¢¶Ë¹æÔòÅäÖÃ×î´óÊıÁ¿ */
	unsigned long			forwards_addr_count ; /* ×ª·¢¶Ë¹æÔòÅäÖÃÊıÁ¿ */
	
	struct ServerNetAddress		*servers_addr ; /* ·şÎñ¶ËµØÖ·½á¹¹ */
	unsigned long			servers_addr_size ; /* ·şÎñ¶Ë¹æÔòÅäÖÃ×î´óÊıÁ¿ */
	unsigned long			servers_addr_count ; /* ·şÎñ¶Ë¹æÔòÅäÖÃÊıÁ¿ */
	unsigned long			selected_addr_index ; /* µ±Ç°·şÎñ¶ËË÷Òı */
} ;

/* ×ª·¢»á»°½á¹¹ */
struct ForwardSession
{
	unsigned char		status ; /* »á»°×´Ì¬ */
	unsigned char		type ; /* ¿Í»§¶Ë»¹ÊÇ·şÎñ¶Ë */
	
	struct ForwardRule	*p_forward_rule ; /* ×ª·¢¹æÔòÅÉÉú */
	unsigned long		client_index ; /* ¿Í»§¶ËË÷Òı */
	unsigned long		server_index ; /* ·şÎñ¶ËË÷Òı */
	
	/*
	union
	{
		struct
		{
			
		} MS ;
	} balance_algorithm ;
	*/
	
	struct NetAddress	netaddr ; /* ÍøÂçµØÖ·½á¹¹ */
	int			sock ; /* sockÃèÊö×Ö */
	
	char			io_buffer[ IO_BUFFER_SIZE + 1 ] ; /* ÊäÈëÊä³ö»º³åÇø */
	int			io_buffer_len ; /* ÊäÈëÊä³ö»º³åÇøÓĞĞ§Êı¾İ³¤¶È */
	int			io_buffer_offsetpos ; /* ÊäÈëÊä³ö»º³åÇøÓĞĞ§Êı¾İ¿ªÊ¼Æ«ÒÆÁ¿ */
	
	struct ForwardSession	*p_reverse_forward_session ; /* ·´Ïò»á»° */
} ;

/* ÃüÁîĞĞ²ÎÊı */
struct CommandParameter
{
	char			*config_pathfilename ; /* -f ... */
	unsigned long		forward_thread_size ; /* -t ... */
	unsigned long		forward_session_size ; /* -s ... */
	int			log_level ; /* --log-level (DEBUG|INFO|WARN|ERROR|FATAL)*/
	int			no_daemon_flag ; /* --no-daemon (1|0) */
} ;

/* ·şÎñÆ÷»·¾³½á¹¹ */
struct PipeFds
{
	int			fds[ 2 ] ;
} ;

struct ServerEnv
{
	struct CommandParameter	cmd_para ; /* ÃüÁîĞĞ²ÎÊı½á¹¹ */
	
	struct ForwardRule	*forward_rules_array ; /* ×ª·¢¹æÔòÊı×é */
	unsigned long		forward_rules_size ; /* ×ª·¢¹æÔòÊı×é´óĞ¡ */
	unsigned long		forward_rules_count ; /* ×ª·¢¹æÔòÒÑ×°ÔØÊıÁ¿ */
	
	pid_t			pid ; /* ×Ó½ø³ÌPID */
	struct PipeFds		accept_pipe ; /* ¸¸×Ó½ø³ÌÃüÁî¹ÜµÀ */
	int			accept_epoll_fd ; /* ÕìÌı¶Ë¿Úepoll³Ø */
	
	int			*forward_thread_index ; /* ×ÓÏß³ÌĞòºÅ */
	pthread_t		*forward_thread_tid_array ; /* ×ÓÏß³ÌTID */
	struct PipeFds		*forward_pipe_array ; /* ¸¸×ÓÏß³ÌÃüÁî¹ÜµÀ */
	int			*forward_epoll_fd_array ; /* ×ÓÏß³Ì×ª·¢EPOLL³Ø */
	
	struct ForwardSession	*forward_session_array ; /* ×ª·¢»á»°Êı×é */
	unsigned long		forward_session_count ; /* ×ª·¢»á»°Êı×é´óĞ¡ */
	unsigned long		forward_session_use_offsetpos ; /* ×ª·¢»á»°×î½üÊ¹ÓÃµ¥ÔªÆ«ÒÆÁ¿ */
} ;

/********* util *********/

int Rand( int min, int max );
unsigned long CalcHash( char *str );
int SetReuseAddr( int sock );
int SetNonBlocking( int sock );
void SetNetAddress( struct NetAddress *p_netaddr );
void GetNetAddress( struct NetAddress *p_netaddr );
int BindDaemonServer( char *pcServerName , int (* ServerMain)( void *pv ) , void *pv , int (* ControlMain)(long lControlStatus) );
int IsMatchString(char *pcMatchString, char *pcObjectString, char cMatchMuchCharacters, char cMatchOneCharacters);

/********* envirment *********/

int InitEnvirment( struct ServerEnv *penv );
void CleanEnvirment( struct ServerEnv *penv );
int AddListeners( struct ServerEnv *penv );
struct ForwardSession *GetForwardSessionUnused( struct ServerEnv *penv );
void SetForwardSessionUnused( struct ForwardSession *p_forward_session );
void SetForwardSessionUnused2( struct ForwardSession *p_forward_session , struct ForwardSession *p_forward_session2 );

/********* Config *********/

int LoadConfig( struct ServerEnv *penv );
void UnloadConfig( struct ServerEnv *penv );

/********* MonitorProcess *********/

int MonitorProcess( struct ServerEnv *penv );
int _MonitorProcess( void *pv );

/********* WorkerProcess *********/

int WorkerProcess( struct ServerEnv *penv );
int WorkerProcess( struct ServerEnv *penv );

/********* AcceptThread *********/

void *AcceptThread( struct ServerEnv *penv );
void *_AcceptThread( void *pv );

/********* ForwardThread *********/

void *ForwardThread( struct ServerEnv *penv );
void *_ForwardThread( void *pv );

#endif

