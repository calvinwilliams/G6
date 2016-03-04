#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

int g_exit_flag = 0 ;
int g_spwan_flag = 0 ;

static void sig_proc( int sig_no )
{
	if( sig_no == SIGUSR2 )
	{
		g_spwan_flag = 1 ;
	}
	
	return;
}

int main()
{
	signal( SIGUSR2 , & sig_proc );
	
	while( g_exit_flag == 0 )
	{
		sleep(1);
		
		if( g_spwan_flag == 1 )
		{
			pid_t	pid ;
			
			pid = fork() ;
			if( pid == -1 )
			{
				exit(9);
			}
			else if( pid == 0 )
			{
				execlp( "t" , "t" , NULL );
				exit(9);
			}
			
			g_exit_flag = 1 ;
		}
	}
	
	return 0;
}

