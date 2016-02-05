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

#ifndef ULONG_MAX
#define ULONG_MAX 0xffffffffUL
#endif

#define FOUND				9	/* 找到 */ /* found */
#define NOT_FOUND			4	/* 找不到 */ /* not found */

#define MATCH				1	/* 匹配 */ /* match */
#define NOT_MATCH			-1	/* 不匹配 */ /* not match */

#define RULE_ID_MAXLEN			64	/* 最长转发规则名长度 */ /* The longest length of forwarding rules */
#define RULE_MODE_MAXLEN		2	/* 最长转发规则模式长度 */ /* The longest length of forwarding rules mode */

#define FORWARD_RULE_MODE_G		"G"	/* 管理端口 */ /* manager port */
#define FORWARD_RULE_MODE_MS		"MS"	/* 主备模式 */ /* master-standby mode */
#define FORWARD_RULE_MODE_RR		"RR"	/* 轮询模式 */ /* polling mode */
#define FORWARD_RULE_MODE_LC		"LC"	/* 最少连接模式 */ /* minimum number of connections mode */
#define FORWARD_RULE_MODE_RT		"RT"	/* 最小响应时间模式 */ /* minimum response time mode */
#define FORWARD_RULE_MODE_RD		"RD"	/* 随机模式 */ /* random mode */
#define FORWARD_RULE_MODE_HS		"HS"	/* HASH模式 */ /* HASH mode */

#define RULE_CLIENT_MAXCOUNT		10	/* 单条规则中最大客户端配置数量 */ /* maximum clients in rule */
#define RULE_FORWARD_MAXCOUNT		3	/* 单条规则中最大转发端配置数量 */ /* maximum forwards in rule */
#define RULE_SERVER_MAXCOUNT		100	/* 单条规则中最大服务端配置数量 */ /* maximum servers in rule */

#define DEFAULT_FORWARD_RULE_MAXCOUNT	100	/* 缺省最大转发规则数量 */ /* maximum forward rules for default */
#define DEFAULT_CONNECTION_MAXCOUNT	1024	/* 缺省最大连接数量 */ /* 最大转发会话数量 = 最大连接数量 * 3 */ /* maximum connections for default */
#define DEFAULT_TRANSFER_BUFSIZE	4096	/* 缺省通讯转发缓冲区大小 */ /* communication I/O buffer for default */




struct ServerEnv
{
	
	
	
	
} ;



#endif

