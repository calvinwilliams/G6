#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int slow_client( int port )
{
	FILE			*fp = NULL ;
	
	int			client_sock ;
	struct sockaddr_in	client_sockaddr ;
	
	char			buffer[ 30000 + 1 ] ;
	int			file_size ;
	int			total_size ;
	int			read_len ;
	int			write_len ;
	
	int			nret = 0 ;
	
	file_size = 1024*1024 ;
	system( "dd if=/dev/urandom of=in.txt bs=1024 count=1024" );
	system( "ls -l in.txt" );
	system( "openssl md5 in.txt" );
	
	fp = fopen( "in.txt" , "r" ) ;
	if( fp == NULL )
	{
		printf( "fopen failed , errno[%d]\n" , errno );
		return -1;
	}
	
	client_sock = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP ) ;
	if( client_sock == -1 )
	{
		printf( "socket failed , errno[%d]\n" , errno );
		fclose( fp );
		return -1;
	}
	
	memset( & client_sockaddr , 0x00 , sizeof(struct sockaddr_in) );
	client_sockaddr.sin_family = AF_INET ;
	client_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY) ;
	client_sockaddr.sin_port = htons( (unsigned short)port );
	nret = connect( client_sock , (struct sockaddr *) & client_sockaddr , sizeof(struct sockaddr) ) ;
	if( nret == -1 )
	{
		printf( "connect failed , errno[%d]\n" , errno );
		close( client_sock );
		fclose( fp );
		return -1;
	}
	
	total_size = 0 ;
	while( total_size < file_size )
	{
		memset( buffer , 0x00 , sizeof(buffer) );
		read_len = fread( buffer , 1 , sizeof(buffer)-1 , fp ) ;
		if( read_len == 0 )
			break;
		
		write_len = write( client_sock , buffer , read_len ) ;
		if( write_len == -1 )
		{
			printf( "write failed , errno[%d]\n" , errno );
			close( client_sock );
			fclose( fp );
			return -1;
		}
		else
		{
			printf( "write [%d/%d]bytes to socket , total[%d]\n" , write_len , read_len , total_size+write_len );
		}
		
		total_size += write_len ;
	}
	
	read_len = read( client_sock , buffer , 1 ) ;
	printf( "[%d] read [%c] from socket\n" , read_len , buffer[0] );
	
	close( client_sock );
	fclose( fp );
	
	unlink( "in.txt" );
	
	return 0;
}

int main( int argc , char *argv[] )
{
	if( argc != 1 + 1  )
		return 7;
	
	return -slow_client( atoi(argv[1]) );
}

