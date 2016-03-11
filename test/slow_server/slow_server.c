#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int slow_server( int port )
{
	FILE			*fp = NULL ;
	
	int			server_sock ;
	struct sockaddr_in	server_sockaddr ;
	int			client_sock ;
	struct sockaddr_in	client_sockaddr ;
	int			on ;
	socklen_t		addr_len = sizeof(struct sockaddr) ;
	
	char			buffer[ 4096 + 1] ;
	int			file_size ;
	int			total_size ;
	int			read_len ;
	int			write_len ;
	
	int			nret = 0 ;
	
	fp = fopen( "out.txt" , "w" ) ;
	if( fp == NULL )
	{
		printf( "fopen failed , errno[%d]\n" , errno );
		return -1;
	}
	
	server_sock = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP ) ;
	if( server_sock == -1 )
	{
		printf( "socket failed , errno[%d]\n" , errno );
		fclose( fp );
		return -1;
	}
	
	on = 1 ;
	setsockopt( server_sock , SOL_SOCKET , SO_REUSEADDR , (void *) & on, sizeof(on) );
	
	memset( & server_sockaddr , 0x00 , sizeof(struct sockaddr_in) );
	server_sockaddr.sin_family = AF_INET ;
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY) ;
	server_sockaddr.sin_port = htons( (unsigned short)port );
	nret = bind( server_sock , (struct sockaddr *) & server_sockaddr , sizeof(struct sockaddr) ) ;
	if( nret == -1 )
	{
		printf( "bind failed , errno[%d]\n" , errno );
		close( server_sock );
		fclose( fp );
		return -1;
	}
	
	nret = listen( server_sock , 1024 ) ;
	if( nret == -1 )
	{
		printf( "listen failed , errno[%d]\n" , errno );
		close( server_sock );
		fclose( fp );
		return -1;
	}
	
	client_sock = accept( server_sock , (struct sockaddr *) & client_sockaddr , & addr_len ) ;
	if( client_sock == -1 )
	{
		printf( "accept failed , errno[%d]\n" , errno );
		close( server_sock );
		fclose( fp );
		return -1;
	}
	
	file_size = 1024 * 1024 ;
	total_size = 0 ;
	while( total_size < file_size )
	{
		usleep(100*1000);
		
		memset( buffer , 0x00 , sizeof(buffer) );
		read_len = read( client_sock , buffer , sizeof(buffer)-1 ) ;
		if( read_len == 0 )
		{
			printf( "read socket close\n" );
			break;
		}
		else if( read_len == -1 )
		{
			printf( "read failed , errno[%d]\n" , errno );
			close( client_sock );
			close( server_sock );
			fclose( fp );
			return -1;
		}
		else
		{
			printf( "read [%d/%d]bytes from socket , total[%d]\n" , read_len , (int)sizeof(buffer)-1 , total_size+read_len );
		}
		
		write_len = fwrite( buffer , 1 , read_len , fp ) ;
		if( write_len != read_len )
		{
			printf( "fwrite failed , errno[%d]\n" , errno );
			close( client_sock );
			close( server_sock );
			fclose( fp );
			return -1;
		}
		
		total_size += write_len ;
	}
	
	write( client_sock , "Z" , 1 );
	printf( "write [Z] to socket\n" );
	
	close( client_sock );
	close( server_sock );
	fclose( fp );
	
	system( "ls -l out.txt" );
	system( "openssl md5 out.txt" );
	unlink( "out.txt" );
	
	return 0;
}

int main( int argc , char *argv[] )
{
	if( argc != 1 + 1 )
		return 7;
	
	return -slow_server( atoi(argv[1]) );
}

