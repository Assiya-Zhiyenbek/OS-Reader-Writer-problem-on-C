#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/stat.h>
#include <math.h>
#include <sys/select.h>

#define BUFSIZE		3096
#define THREADS		50

int			csock;
char		*file_name;
int			cc;
char		buf[BUFSIZE];
char 		ch[256];
pthread_t	thr[252];
char		*directory;
pthread_mutex_t mutex;
double		timee;
int count = 0;
FILE * fPtr;

int connectsock( char *host, char *service, char *protocol );

double poissonRandomInterarrivalDelay( double L )

{
    return (log((double) 1.0 - ((double) rand())/((double) RAND_MAX)))/-L;
}

void *rread( void *s )
{

	write(csock, ch, 256);
	
	char filenameq[256];
	sprintf(filenameq, "./ohaio/%ld.txt", pthread_self());
	fPtr = fopen(filenameq, "w");

	int sret;
	struct timeval timeout;
	fd_set readfds;
	//printf("%s\n", ch);

	FD_ZERO(&readfds);
	FD_SET(csock, &readfds);
	timeout.tv_sec = timee;
	timeout.tv_usec = 0;

	sret = select(8, &readfds, NULL, NULL, &timeout);
	for(int i = 0; i < 500; i++)
	{
		
		if( (cc = read(csock, buf, BUFSIZE)) > 0)
		{
			fprintf(fPtr, "%s\n", buf);
		}
		
	}
	if(sret == 0)
		{
			printf("TIMEOUT of thread: %ld\n", pthread_self());
			
			//exit(0);
		}
	printf("New file is created\n");
	//fclose(fPtr);
	pthread_exit(0);
}

void *wwrite( void *s )
{

	pthread_mutex_lock(&mutex);
	write(csock, ch, 256);
	int sret;
	struct timeval timeout;
	fd_set readfds;
	//printf("%s\n", ch);

	FD_ZERO(&readfds);
	FD_SET(csock, &readfds);
	timeout.tv_sec = timee;
	timeout.tv_usec = 0;

	sret = select(8, &readfds, NULL, NULL, &timeout);
	read(csock, buf, 5);
	if(sret == 0)
		{
			printf("TIMEOUT of thread: %ld\n", pthread_self());
			
			//exit(0);
		}

	if(strcmp(buf, "GO") == 0){
		count++;
		printf("New thread\n");
		char textt[BUFSIZE];
		
		for(int j = 0; j < 400; j++)
		{
			
			for(int k = 0; k < BUFSIZE; k++)
			{
				textt[k] = 'A' + count % 26;
			}
			write(csock, textt, BUFSIZE);
			
			memset(textt, 0, BUFSIZE);
		}

		printf("Text is generated\n");
	
	}
	pthread_mutex_unlock(&mutex);

	pthread_exit(0);
}


int main( int argc, char *argv[] )
{
	
	char		*service;		
	char		*host = "localhost";
	char		*client_type;
	double		rate;
	
	int			status;
	
	switch( argc ) 
	{
		case    2:
			service = argv[1];
			break;
		case    7:
			//host = argv[1];
			client_type = argv[1];
			service = argv[2];
			rate = atof(argv[3]);
			file_name = argv[4];
			directory = argv[5];
			timee = atof(argv[6]);
			break;
		default:
			fprintf( stderr, "usage: chat [host] port\n" );
			exit(-1);
	}
	
	/*	Create the socket to the controller  */
	if ( ( csock = connectsock( host, service, "tcp" )) == 0 )
	{
		fprintf( stderr, "Cannot connect to server.\n" );
		exit( -1 );
	}

	printf( "The server is ready, please start sending to the server.\n" );
	//printf( "Type q or Q to quit.\n" );
	fflush( stdout );

		printf("%s\n", buf);
		if (strcmp(client_type, "rclient") == 0)
		{					
			strcat(ch, "rclient ");
			strcat(ch, file_name);

			for(int i = 0; i < THREADS; i++)
			{
			    status = pthread_create( &thr[i++], NULL, rread, NULL );
			    sleep(poissonRandomInterarrivalDelay(rate));
			    if(status != 0)
			    {
			    	printf( "pthread_create error %d.\n", status );
					exit( -1 );
			    }
			}

			for (int j = 0; j < THREADS; j++ )
				pthread_join( thr[j], NULL );
		}

		if(strcmp(client_type, "wclient") == 0)
		{					
			strcat(ch, "wclient ");
			strcat(ch, file_name);
			client_type = NULL;
			file_name = NULL;

			for(int i = 0; i < THREADS; i++)
			{
			    status = pthread_create( &thr[i++], NULL, wwrite, NULL );
			    sleep(poissonRandomInterarrivalDelay(rate));
			    if(status != 0)
			    {
			    	printf( "pthread_create error %d.\n", status );
					exit( -1 );
			    }
			}

			for (int j = 0; j < THREADS; j++ )
				pthread_join( thr[j], NULL );
		}
	
	pthread_exit(0);
	close( csock );
}