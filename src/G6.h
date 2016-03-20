/*
 * G6 - TCP Bridge && LB Dispenser
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
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/wait.h>
#include <signal.h>
#define __USE_GNU
#include <sched.h>
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
#include "rbtree.h"

#ifndef ULONG_MAX
#define ULONG_MAX 0xffffffffUL
#endif

#define FOUND				9	/* 找到 */
#define NOT_FOUND			4	/* 找不到 */

#define MATCH				1	/* 匹配 */
#define NOT_MATCH			-1	/* 不匹配 */

#define LOAD_BALANCE_ALGORITHM_G	"G"	/* 管理端口 */
#define LOAD_BALANCE_ALGORITHM_MS	"MS"	/* 主备模式 */
#define LOAD_BALANCE_ALGORITHM_RR	"RR"	/* 轮询模式 */
#define LOAD_BALANCE_ALGORITHM_LC	"LC"	/* 最少连接模式 */
#define LOAD_BALANCE_ALGORITHM_RT	"RT"	/* 最小响应时间模式 */
#define LOAD_BALANCE_ALGORITHM_RD	"RD"	/* 随机模式 */
#define LOAD_BALANCE_ALGORITHM_HS	"HS"	/* HASH模式 */

#define DEFAULT_RULES_INITCOUNT			2
#define DEFAULT_RULES_INCREASE			5

#define DEFAULT_CLIENTS_INITCOUNT_IN_ONE_RULE	2
#define DEFAULT_CLIENTS_INCREASE_IN_ONE_RULE	5
#define DEFAULT_FORWARDS_INITCOUNT_IN_ONE_RULE	2
#define DEFAULT_FORWARDS_INCREASE_IN_ONE_RULE	5
#define DEFAULT_SERVERS_INITCOUNT_IN_ONE_RULE	2
#define DEFAULT_SERVERS_INCREASE_IN_ONE_RULE	5

#define DEFAULT_FORWARD_SESSIONS_MAXCOUNT	10000	/* 缺省最大连接数量 */
#define DEFAULT_FORWARD_TRANSFER_BUFSIZE	4096	/* 缺省通讯转发缓冲区大小 */

#define RULE_ID_MAXLEN				64
#define LOAD_BALANCE_ALGORITHM_MAXLEN		2
#define IP_MAXLEN				30
#define PORT_MAXLEN				10

#define WAIT_EVENTS_COUNT			1024 /* 一次获取epoll事件数量 */

#define FORWARD_SESSION_TYPE_UNUSED		0 /* 未使用 */
#define FORWARD_SESSION_TYPE_LISTEN		1 /* 侦听会话 */
#define FORWARD_SESSION_TYPE_CLIENT		2 /* 作为客户端连接远端服务端会话 */
#define FORWARD_SESSION_TYPE_SERVER		3 /* 作为服务端接受远端连接会话 */
#define FORWARD_SESSION_TYPE_MANAGER		4 /* 作为管理服务端接受远端连接会话 */

#define FORWARD_SESSION_STATUS_UNUSED		0 /* 未使用 */
#define FORWARD_SESSION_STATUS_READY		1 /* 准备使用 */
#define FORWARD_SESSION_STATUS_LISTEN		2 /* 侦听中 */
#define FORWARD_SESSION_STATUS_CONNECTING	3 /* 非堵塞连接中 */
#define FORWARD_SESSION_STATUS_CONNECTED	4 /* 连接完成 */
#define FORWARD_SESSION_STATUS_COMMAND		5 /* 命令 */

#define IO_BUFFER_SIZE				4096 /* 输入输出缓冲区大小 */

#define DEFAULT_MORATORIUM_SECONDS		60 /* 缺省暂禁时间，单位：秒 */
#define DEFAULT_TIMEOUT_SECONDS			60 /* 缺省超时时间，单位：秒 */

#define G6_LISTEN_SOCKFDS			"G6_LISTEN_SOCKFDS"

/* IP-CONNECTION统计信息结构 */
struct IpConnection
{
	char		used_flag ; /* 使用标志 */
	uint32_t	ip_int ; /* IP地址 */
	unsigned int	connection_count ; /* 连接数量 */
} ;

#define DEFAULT_IP_CONNECTION_ARRAY_SIZE	2

struct IpConnectionStat
{
	unsigned int			max_ip ; /* 最大IP数量 */
	unsigned int			max_connections ; /* 最大CONNECTION数量 */
	unsigned int			max_connections_per_ip ; /* 每个IP最大连接数量 */
	struct IpConnection		*ip_connection_array ; /* IP-CONNECTION统计数组 */
	unsigned int			ip_count ; /* 当前IP数量 */
	unsigned int			connection_count ; /* 当前CONNECTION数量 */
	unsigned int			ip_connection_stat_size ; /* 最大IP-CONNECTION数量 */
} ;

/* 网络地址信息结构 */
struct NetAddress
{
	char			ip[ IP_MAXLEN + 1 ] ; /* ip地址 */
	union
	{
		char		port_str[ PORT_MAXLEN + 1 ] ;
		unsigned int	port_int ;
	} port ; /* 端口 */
	struct sockaddr_in	sockaddr ; /* sock地址结构 */
} ;

/* 客户端信息结构 */
struct ClientNetAddress
{
	struct NetAddress	netaddr ; /* 网络地址结构 */
	
	struct IpConnectionStat	ip_connection_stat ; /* IP-CONNECTION统计信息，也用于连接限制 */
} ;

/* 转发端信息结构 */
struct ForwardNetAddress
{
	struct NetAddress	netaddr ; /* 网络地址结构 */
	int			sock ; /* sock描述字 */
	
	unsigned int		timeout ; /* 超时周期 */
} ;

/* 服务端信息结构 */
struct ServerNetAddress
{
	struct NetAddress	netaddr ; /* 网络地址结构 */
	
	unsigned int		heartbeat ; /* 心跳周期 */
	unsigned char		server_unable ; /* 服务端可用标志 */
	time_t			timestamp_to ; /* 恢复正常秒戳 */
	
	time_t			rtt ; /* 最近读时间戳 */
	time_t			wtt ; /* 最近写时间戳 */
	
	struct IpConnectionStat	ip_connection_stat ; /* IP-CONNECTION统计信息，也用于连接限制 */
} ;

/* 转发规则结构 */
struct ForwardRule
{
	char				rule_id[ RULE_ID_MAXLEN + 1 ] ; /* 规则ID */
	char				load_balance_algorithm[ LOAD_BALANCE_ALGORITHM_MAXLEN + 1 ] ; /* 负载均衡算法 */
	
	struct IpConnectionStat		client_ip_connection_stat ; /* IP-CONNECTION统计信息，也用于连接限制 */
	
	struct ClientNetAddress		*client_addr_array ; /* 客户端地址结构 */
	unsigned int			client_addr_size ; /* 客户端规则配置最大数量 */
	unsigned int			client_addr_count ; /* 客户端规则配置数量 */
	
	unsigned int			forward_timeout ; /* 超时周期 */
	
	struct ForwardNetAddress	*forward_addr_array ; /* 转发端地址结构 */
	unsigned int			forward_addr_size ; /* 转发端规则配置最大数量 */
	unsigned int			forward_addr_count ; /* 转发端规则配置数量 */
	
	unsigned int			server_heartbeat ; /* 心跳周期 */
	struct IpConnectionStat		server_ip_connection_stat ; /* IP-CONNECTION统计信息，也用于连接限制 */
	
	struct ServerNetAddress		*server_addr_array ; /* 服务端地址结构 */
	unsigned int			server_addr_size ; /* 服务端规则配置最大数量 */
	unsigned int			server_addr_count ; /* 服务端规则配置数量 */
	unsigned int			selected_addr_index ; /* 当前服务端索引 */
} ;

/* 转发会话结构 */
struct ForwardSession
{
	unsigned char			status ; /* 会话状态 */
	unsigned char			type ; /* 侦听端、客户端、服务端、管理服务端 */
	
	int				sock ; /* sock描述字 */
	struct NetAddress		netaddr ; /* 网络地址结构 */
	struct ForwardRule		*p_forward_rule ; /* 转发规则 */
	unsigned int			client_index ; /* 客户端索引 */
	unsigned int			forward_index ; /* 转发端索引 */
	unsigned int			server_index ; /* 服务端索引 */
	
	char				io_buffer[ IO_BUFFER_SIZE + 1 ] ; /* 输入输出缓冲区 */
	int				io_buffer_len ; /* 输入输出缓冲区有效数据长度 */
	int				io_buffer_offsetpos ; /* 输入输出缓冲区有效数据开始偏移量 */
	
	struct ForwardSession		*p_reverse_forward_session ; /* 反向会话 */
	
	struct rb_node			timeout_rbnode ; /* 超时 红黑树节点 */
	int				timeout_timestamp ; /* 超时时间戳 */
} ;

/* 命令行参数 */
struct CommandParameter
{
	char				*config_pathfilename ; /* -f ... */
	unsigned int			forward_thread_size ; /* -t ... */
	unsigned int			forward_session_size ; /* -s ... */
	unsigned int			log_level ; /* --log-level (DEBUG|INFO|WARN|ERROR|FATAL)*/
	char				log_pathfilename[ MAXLEN_FILENAME + 1 ] ; /* --log-filename (logfilename) */
	unsigned int			no_daemon_flag ; /* --no-daemon */
	unsigned int			close_log_flag ; /* --close-log */
} ;

/* 服务器环境结构 */
extern struct ServerEnv			*g_penv ;
extern signed char			g_exit_flag ;

struct PipeFds
{
	int				fds[ 2 ] ;
} ;

#define UPDATE_TIME \
	{ \
		UpdateTimeNow( & g_time_tv , g_date_and_time ); \
	}

#define UPDATE_TIME_FROM_CACHE \
	{ \
		pthread_mutex_lock( & (penv->time_cache_mutex) ); \
		g_time_tv.tv_sec = penv->time_tv.tv_sec ; \
		memcpy( g_date_and_time , penv->date_and_time , sizeof(penv->date_and_time) ); \
		pthread_mutex_unlock( & (penv->time_cache_mutex) ); \
	}

struct ServerEnv
{
	struct timeval			time_tv ;
	char				date_and_time[ sizeof(g_date_and_time) ] ;
	
	struct CommandParameter		cmd_para ; /* 命令行参数结构 */
	char				**argv ;
	
	struct ForwardNetAddress	*old_forward_addr_array ; /* 用于平滑升级的老进程侦听端口信息 */
	unsigned int			old_forward_addr_count ;
	
	unsigned int			moratorium ; /* 暂禁时间 */
	unsigned int			timeout ; /* 超时周期 */
	struct rb_root			timeout_rbtree ; /* 超时 红黑树 */
	unsigned int			heartbeat ; /* 心跳周期 */
	struct IpConnectionStat		ip_connection_stat ; /* IP-CONNECTION统计信息，也用于连接限制 */
	
	struct ForwardRule		*forward_rule_array ; /* 转发规则数组 */
	unsigned int			forward_rule_size ; /* 转发规则数组大小 */
	unsigned int			forward_rule_count ; /* 转发规则已装载数量 */
	
	struct ForwardSession		*forward_session_array ; /* 通讯会话数组 */
	unsigned int			forward_session_count ; /* 通讯会话数组大小 */
	unsigned int			forward_session_use_offsetpos ; /* 转发会话最近使用单元偏移量 */
	
	pid_t				pid ; /* 工作进程PID */
	struct PipeFds			accept_request_pipe ; /* 工作进程请求命令管道 */
	struct PipeFds			accept_response_pipe ; /* 工作进程响应命令管道 */
	int				accept_epoll_fd ; /* 侦听端口epoll池 */
	
	pthread_t			time_thread_tid ; /* 时间管理线程 */
	pthread_t			*forward_thread_tid_array ; /* 分发线程TID */
	struct PipeFds			*forward_request_pipe ; /* 分发线程请求命令管道 */
	struct PipeFds			*forward_response_pipe ; /* 分发线程响应命令管道 */
	int				*forward_epoll_fd_array ; /* 数据收发epoll池 */
	
	pthread_mutex_t			ip_connection_stat_mutex ; /* IP-CONNECTION统计 临界区互斥 */
	pthread_mutex_t			forward_session_count_mutex ; /* 通讯会话数量 临界区互斥 */
	pthread_mutex_t			server_connection_count_mutex ; /* 服务端连接数量 临界区互斥 */
	pthread_mutex_t			timeout_rbtree_mutex ; /* 超时红黑树 临界区互斥 */
	pthread_mutex_t			time_cache_mutex ; /* 超时红黑树 临界区互斥 */
} ;

/********* util *********/

int Rand( int min, int max );
unsigned long CalcHash( char *str );
void SetNetAddress( struct NetAddress *p_netaddr );
void GetNetAddress( struct NetAddress *p_netaddr );
int BindDaemonServer( char *pcServerName , int (* ServerMain)( void *pv ) , void *pv , int (* ControlMain)(long lControlStatus) );
int IsMatchString(char *pcMatchString, char *pcObjectString, char cMatchMuchCharacters, char cMatchOneCharacters);
int BindCpuAffinity( int processor_no );
void UpdateTimeNow( struct timeval *p_time_tv , char *p_date_and_time );

#define SetReuseAddr(_sock_) \
	{ \
		int	on = 1 ; \
		setsockopt( _sock_ , SOL_SOCKET , SO_REUSEADDR , (void *) & on, sizeof(on) ); \
	}

#if ( defined __linux ) || ( defined __unix )
#define SetNonBlocking(_sock_) \
	{ \
		int	opts; \
		opts = fcntl( _sock_ , F_GETFL ) ; \
		opts = opts | O_NONBLOCK; \
		fcntl( _sock_ , F_SETFL , opts ); \
	}
#elif ( defined _WIN32 )
#define SetNonBlocking(_sock_) \
	{ \
		u_long	mode = 1 ; \
		ioctlsocket( _sock_ , FIONBIO , & mode ); \
	}
#endif

#define SetNagleClosed(_sock_) \
	{ \
		int	on = 1 ; \
		setsockopt( _sock_ , IPPROTO_TCP , TCP_NODELAY , (void*) & on , sizeof(int) ); \
	}

#define SetCloseExec(_sock_) \
	{ \
		int	val ; \
		val = fcntl( _sock_ , F_GETFD ) ; \
		val |= FD_CLOEXEC ; \
		fcntl( _sock_ , F_SETFD , val ); \
	}

#define SetCloseExec2(_sock_,_sock2_) \
	{ \
		SetCloseExec( _sock_ ); \
		SetCloseExec( _sock2_ ); \
	}

#define SetCloseExec3(_sock_,_sock2_,_sock3_) \
	{ \
		SetCloseExec( _sock_ ); \
		SetCloseExec( _sock2_ ); \
		SetCloseExec( _sock3_ ); \
	}

#define SetCloseExec4(_sock_,_sock2_,_sock3_,_sock4_) \
	{ \
		SetCloseExec( _sock_ ); \
		SetCloseExec( _sock2_ ); \
		SetCloseExec( _sock3_ ); \
		SetCloseExec( _sock4_ ); \
	}

#define SetNetAddress(_p_netaddr_) \
	memset( & ((_p_netaddr_)->sockaddr) , 0x00 , sizeof(struct sockaddr_in) ); \
	(_p_netaddr_)->sockaddr.sin_family = AF_INET ; \
	(_p_netaddr_)->sockaddr.sin_addr.s_addr = inet_addr((_p_netaddr_)->ip) ; \
	(_p_netaddr_)->sockaddr.sin_port = htons( (unsigned short)((_p_netaddr_)->port.port_int) );

#define GetNetAddress(_p_netaddr_) \
	strcpy( (_p_netaddr_)->ip , inet_ntoa((_p_netaddr_)->sockaddr.sin_addr) ); \
	(_p_netaddr_)->port.port_int = (int)ntohs( (_p_netaddr_)->sockaddr.sin_port ) ;

/********* Envirment *********/

int InitEnvirment( struct ServerEnv *penv );
int InitEnvirment2( struct ServerEnv *penv );
void CleanEnvirment( struct ServerEnv *penv );

int SaveListenSockets( struct ServerEnv *penv );
int LoadOldListenSockets( struct ServerEnv *penv );
int CleanOldListenSockets( struct ServerEnv *penv );

int AddListeners( struct ServerEnv *penv );

struct ForwardSession *GetForwardSessionUnused( struct ServerEnv *penv );
#define IsForwardSessionUsed(_p_forward_session_)	((_p_forward_session_)->status!=FORWARD_SESSION_STATUS_UNUSED?1:0)
void SetForwardSessionUnused( struct ServerEnv *penv , struct ForwardSession *p_forward_session );
void SetForwardSessionUnused2( struct ServerEnv *penv , struct ForwardSession *p_forward_session , struct ForwardSession *p_forward_session2 );

void RemoveTimeoutTreeNode( struct ServerEnv *penv , struct ForwardSession *p_forward_session );
void RemoveTimeoutTreeNode2( struct ServerEnv *penv , struct ForwardSession *p_forward_session , struct ForwardSession *p_forward_session2 );
int AddTimeoutTreeNode( struct ServerEnv *penv , struct ForwardSession *p_forward_session , unsigned int timeout_timestamp );
int AddTimeoutTreeNode2( struct ServerEnv *penv , struct ForwardSession *p_forward_session , struct ForwardSession *p_forward_session2 , unsigned int timeout_timestamp );
int UpdateTimeoutNode( struct ServerEnv *penv , struct ForwardSession *p_forward_session , unsigned int timeout_timestamp );
int UpdateTimeoutNode2( struct ServerEnv *penv , struct ForwardSession *p_forward_session , struct ForwardSession *p_forward_session2 , unsigned int timeout_timestamp );
int GetLastestTimeout( struct ServerEnv *penv );
struct ForwardSession *GetExpireTimeoutNode( struct ServerEnv *penv );

int InitIpConnectionStat( struct IpConnectionStat *p_ip_connection_stat );
void CleanIpConnectionStat( struct IpConnectionStat *p_ip_connection_stat );
int AddIpConnectionStat( struct ServerEnv *penv , struct IpConnectionStat *p_ip_connection_stat , uint32_t ip_int );
int RemoveIpConnectionStat( struct ServerEnv *penv , struct IpConnectionStat *p_ip_connection_stat , uint32_t ip_int );

/********* Config *********/

int LoadConfig( struct ServerEnv *penv );
void UnloadConfig( struct ServerEnv *penv );

/********* MonitorProcess *********/

int MonitorProcess( struct ServerEnv *penv );
int _MonitorProcess( void *pv );

/********* WorkerProcess *********/

int WorkerProcess( struct ServerEnv *penv );

/********* AcceptThread *********/

void *AcceptThread( struct ServerEnv *penv );
void *_AcceptThread( void *pv );

/********* ForwardThread *********/
void IgnoreReverseSessionEvents( struct ForwardSession *p_forward_session , struct epoll_event *p_events , int event_index , int event_count );

#define DISCONNECT_PAIR	\
	epoll_ctl( forward_epoll_fd , EPOLL_CTL_DEL , p_forward_session->sock , NULL ); \
	epoll_ctl( forward_epoll_fd , EPOLL_CTL_DEL , p_reverse_forward_session->sock , NULL ); \
	if( p_forward_session->p_forward_rule->forward_addr_array[p_forward_session->forward_index].timeout > 0 ) \
	{ \
		RemoveTimeoutTreeNode2( penv , p_forward_session , p_reverse_forward_session ); \
	} \
	if( p_forward_session->type == FORWARD_SESSION_TYPE_SERVER ) \
	{ \
		nret = RemoveIpConnectionStat( penv , & (p_forward_session->p_forward_rule->client_addr_array[p_forward_session->client_index].ip_connection_stat) , p_forward_session->netaddr.sockaddr.sin_addr.s_addr ) ; \
		if( nret ) \
			ErrorLog( __FILE__ , __LINE__ , "RemoveIpConnectionStat failed[%d] , SERVER.1" , nret ); \
		nret = RemoveIpConnectionStat( penv , & (p_reverse_forward_session->p_forward_rule->server_addr_array[p_reverse_forward_session->server_index].ip_connection_stat) , p_reverse_forward_session->netaddr.sockaddr.sin_addr.s_addr ) ; \
		if( nret ) \
			ErrorLog( __FILE__ , __LINE__ , "RemoveIpConnectionStat failed[%d] , SERVER.2" , nret ); \
	} \
	else if( p_forward_session->type == FORWARD_SESSION_TYPE_CLIENT ) \
	{ \
		nret = RemoveIpConnectionStat( penv , & (p_forward_session->p_forward_rule->server_addr_array[p_forward_session->server_index].ip_connection_stat) , p_forward_session->netaddr.sockaddr.sin_addr.s_addr ) ; \
		if( nret ) \
			ErrorLog( __FILE__ , __LINE__ , "RemoveIpConnectionStat failed[%d] , CLIENT.1" , nret ); \
		nret = RemoveIpConnectionStat( penv , & (p_reverse_forward_session->p_forward_rule->client_addr_array[p_reverse_forward_session->client_index].ip_connection_stat) , p_reverse_forward_session->netaddr.sockaddr.sin_addr.s_addr ) ; \
		if( nret ) \
			ErrorLog( __FILE__ , __LINE__ , "RemoveIpConnectionStat failed[%d] , CLIENT.2" , nret ); \
	} \
	DebugLog( __FILE__ , __LINE__ , "close #%d# #%d#" , p_forward_session->sock , p_reverse_forward_session->sock ); \
	_CLOSESOCKET2( p_forward_session->sock , p_reverse_forward_session->sock ); \
	IgnoreReverseSessionEvents( p_forward_session , events , event_index , event_count ); \
	SetForwardSessionUnused2( penv , p_forward_session , p_reverse_forward_session );

int OnForwardInput( struct ServerEnv *penv , struct ForwardSession *p_forward_session , int forward_epoll_fd , struct epoll_event *p_events , int event_index , int event_count , unsigned char after_accept_flag );
void *ForwardThread( unsigned long forward_thread_index );
void *_ForwardThread( void *pv );

/********* TimeThread *********/

void *TimeThread();
void *_TimeThread( void *pv );

#endif

