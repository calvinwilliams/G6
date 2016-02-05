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
#define _VSNPRINTF		vsnprintf
#define _SNPRINTF		snprintf
#define _CLOSESOCKET		close
#define _ERRNO			errno
#define _EWOULDBLOCK		EWOULDBLOCK
#define _ECONNABORTED		ECONNABORTED
#define _EINPROGRESS		EINPROGRESS
#define _ECONNRESET		ECONNRESET
#define _ENOTCONN		ENOTCONN
#define _EISCONN		EISCONN
#define _SOCKLEN_T		socklen_t
#define _GETTIMEOFDAY(_tv_)	gettimeofday(&(_tv_),NULL)
#define _LOCALTIME(_tt_,_stime_) \
	localtime_r(&(_tt_),&(_stime_));
#elif ( defined _WIN32 )
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <io.h>
#include <windows.h>
#define _VSNPRINTF		_vsnprintf
#define _SNPRINTF		_snprintf
#define _CLOSESOCKET		closesocket
#define _ERRNO			GetLastError()
#define _EWOULDBLOCK		WSAEWOULDBLOCK
#define _ECONNABORTED		WSAECONNABORTED
#define _EINPROGRESS		WSAEINPROGRESS
#define _ECONNRESET		WSAECONNRESET
#define _ENOTCONN		WSAENOTCONN
#define _EISCONN		WSAEISCONN
#define _SOCKLEN_T		int
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

#define RULE_ID_MAXLEN			64	/* ×î³¤×ª·¢¹æÔòÃû³¤¶È */
#define RULE_MODE_MAXLEN		2	/* ×î³¤×ª·¢¹æÔòÄ£Ê½³¤¶È */

#define LOAD_BALANCE_ALGORITHM_G	"G"	/* ¹ÜÀí¶Ë¿Ú */
#define LOAD_BALANCE_ALGORITHM_MS	"MS"	/* Ö÷±¸Ä£Ê½ */
#define LOAD_BALANCE_ALGORITHM_RR	"RR"	/* ÂÖÑ¯Ä£Ê½ */
#define LOAD_BALANCE_ALGORITHM_LC	"LC"	/* ×îÉÙÁ¬½ÓÄ£Ê½ */
#define LOAD_BALANCE_ALGORITHM_RT	"RT"	/* ×îĞ¡ÏìÓ¦Ê±¼äÄ£Ê½ */
#define LOAD_BALANCE_ALGORITHM_RD	"RD"	/* Ëæ»úÄ£Ê½ */
#define LOAD_BALANCE_ALGORITHM_HS	"HS"	/* HASHÄ£Ê½ */

#define DEFAULT_RULE_INITCOUNT			1
#define DEFAULT_RULE_INCREASE			5

#define DEFAULT_CLIENT_INITCOUNT_IN_ONE_RULE	1
#define DEFAULT_CLIENT_INCREASE_IN_ONE_RULE	5
#define DEFAULT_FORWARD_INITCOUNT_IN_ONE_RULE	1
#define DEFAULT_FORWARD_INCREASE_IN_ONE_RULE	5
#define DEFAULT_SERVER_INITCOUNT_IN_ONE_RULE	1
#define DEFAULT_SERVER_INCREASE_IN_ONE_RULE	5

#define DEFAULT_FORWARD_SESSIONS_MAXCOUNT	1024	/* È±Ê¡×î´óÁ¬½ÓÊıÁ¿ */
#define DEFAULT_FORWARD_TRANSFER_BUFSIZE	4096	/* È±Ê¡Í¨Ñ¶×ª·¢»º³åÇø´óĞ¡ */

/* ÍøÂçµØÖ·ĞÅÏ¢½á¹¹ */
struct NetAddress
{
	char			ip[ 15 + 1 ] ; /* ipµØÖ· */
	char			port[ 10 + 1 ] ; /* ¶Ë¿Ú */
	struct sockaddr_in	sockaddr ; /* sockµØÖ·½á¹¹ */
} ;

/* ¿Í»§¶ËĞÅÏ¢½á¹¹ */
struct ClientNetAddress
{
	struct NetAddress	netaddr ; /* ÍøÂçµØÖ·½á¹¹ */
	int			sock ; /* sockÃèÊö×Ö */
	
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
	int			sock ; /* sockÃèÊö×Ö */
	
	unsigned long		server_connection_count ; /* ·şÎñ¶ËÁ¬½ÓÊıÁ¿ */
} ;

/* ×ª·¢¹æÔò½á¹¹ */
struct ForwardRule
{
	char				rule_id[ RULE_ID_MAXLEN + 1 ] ; /* ¹æÔòID © */
	char				load_balance_algorithm[ LOAD_BALANCE_ALGORITHM_MAXLEN + 1 ] ; /* ¸ºÔØ¾ùºâËã·¨ */
	
	struct ClientNetAddress		*clients_addr ; /* ¿Í»§¶ËµØÖ·½á¹¹ */
	unsigned long			clients_maxcount ; /* ¿Í»§¶Ë¹æÔòÅäÖÃ×î´óÊıÁ¿ */
	unsigned long			clients_count ; /* ¿Í»§¶Ë¹æÔòÅäÖÃÊıÁ¿ */
	
	struct ForwardNetAddress	*forwards_addr[ RULE_FORWARD_MAXCOUNT ] ; /* ×ª·¢¶ËµØÖ·½á¹¹ */
	unsigned long			forwards_maxcount ; /* ×ª·¢¶Ë¹æÔòÅäÖÃ×î´óÊıÁ¿ */
	unsigned long			forwards_count ; /* ×ª·¢¶Ë¹æÔòÅäÖÃÊıÁ¿ */
	
	struct ServerNetAddress		servers_addr[ RULE_SERVER_MAXCOUNT ] ; /* ·şÎñ¶ËµØÖ·½á¹¹ */
	unsigned long			servers_maxcount ; /* ·şÎñ¶Ë¹æÔòÅäÖÃ×î´óÊıÁ¿ */
	unsigned long			servers_count ; /* ·şÎñ¶Ë¹æÔòÅäÖÃÊıÁ¿ */
	unsigned long			selects_index ; /* µ±Ç°·şÎñ¶ËË÷Òı */
	
	union
	{
		struct
		{
			unsigned long	server_unable ; /* ·şÎñ²»¿ÉÓÃÔİ½û´ÎÊı */
		} RR[ RULE_SERVER_MAXCOUNT ] ;
		struct
		{
			unsigned long	server_unable ; /* ·şÎñ²»¿ÉÓÃÔİ½û´ÎÊı */
		} LC[ RULE_SERVER_MAXCOUNT ] ;
		struct
		{
			unsigned long	server_unable ; /* ·şÎñ²»¿ÉÓÃÔİ½û´ÎÊı */
			struct timeval	tv1 ; /* ×î½ü¶ÁÊ±¼ä´Á */
			struct timeval	tv2 ; /* ×î½üĞ´Ê±¼ä´Á */
			struct timeval	dtv ; /* ×î½ü¶ÁĞ´Ê±¼ä´Á²î */
		} RT[ RULE_SERVER_MAXCOUNT ] ;
	} status ;
	
} ;

#define FORWARD_SESSION_STATUS_CONNECTING	1 /* ·Ç¶ÂÈûÁ¬½ÓÖĞ */
#define FORWARD_SESSION_STATUS_CONNECTED	2 /* Á¬½ÓÍê³É */

#define IO_BUFFER_SIZE				4096 /* ÊäÈëÊä³ö»º³åÇø´óĞ¡ */

struct IoBuffer
{
	char			buffer[ IO_BUFFER_SIZE + 1 ] ;
	unsinged long		buffer_len ;
} ;

/* ×ª·¢»á»°½á¹¹ */
struct ForwardSession
{
	struct ForwardRule	*p_forward_rule ;
	
	unsigned char		status ;
	
	struct IoBuffer		io_buffer ;
	
	struct ForwardSession	*p_reverse_forward_session ;
} ;

/* ÃüÁîĞĞ²ÎÊı */
struct CommandParameter
{
	char			*config_pathfilename ; /* -f ... */
	unsigned long		forward_thread_count ; /* -t ... */
	unsigned long		forward_session_maxcount ; /* -s ... */
} ;

/* ·şÎñÆ÷»·¾³½á¹¹ */
struct ServerEnv
{
	struct CommandParameter	cmd_para ;
	
	struct ForwardRule	*forward_rules ;
	unsigned long		forward_rules_size ;
	unsigned long		forward_rules_count ;
	
	struct ForwardSession	*forward_sessions ;
	unsigned long		forward_session_count ;
	unsigned long		forward_session_use_offsetpos ;
} ;

/********* util *********/

int Rand( int min, int max );
unsigned long CalcHash( char *str );
int SetReuseAddr( int sock );
int SetNonBlocking( int sock );

/********* MonitorProcess *********/

int MonitorProcess( struct ServerEnv *penv );

#endif

