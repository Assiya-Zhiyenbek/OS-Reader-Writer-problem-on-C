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
#include <semaphore.h>

#define	QLEN			5
#define	BUFSIZE			3096
#define THREADS		50

FILE *file;
sem_t resource, rmutex, wmutex, readTry;
int readcount = 0, writecounter = 0;

int passivesock( char *service, char *protocol, int qlen, int *rport );

void *reader_preference( void *s )
{
	char buf[BUFSIZE];
	int cc;
	int ssock = (int) s;
	int i = 0;
	char for_file[BUFSIZE];
	char *filename;

	/* start working for this guy */
	/* ECHO what the client says */

	for (;;)
	{
		
		if ( (cc = read( ssock, buf, 256 )) <= 0 )
		{
			printf( "The client has gone.\n" );
			close(ssock);
			break;
		}
		else
		{
				char *tmp;
				char *tmpst[252];
				int j = 0;
				
				filename = strtok(buf, " ");
				while(filename != NULL)
				{
					tmp = filename;
					tmpst[j] = filename;
					filename = strtok(NULL, " ");
					j++;
				}
				//printf("%s\n", tmpst[0]);

				if(strcmp(tmpst[0], "rclient") == 0)
				{
					sem_wait(&rmutex);
					readcount++;

					if(readcount == 1)
					{
						sem_wait(&resource);
					}

					sem_post(&rmutex);

					file = fopen(tmpst[1], "r");
					fgets(for_file, BUFSIZE, file);
					//sleep(3);
					for(int i = 0; i < 550; i++)
					{

						write(ssock, for_file, BUFSIZE);
						
					}
					printf("New Reader file is created\n");

					sem_wait(&rmutex);
					readcount--;

					if(readcount == 0)
					{
						sem_post(&resource);
					}

					sem_post(&rmutex);
					//fclose(file);
				}
				
				else if(strcmp(tmpst[0], "wclient") == 0)
				{
					sem_wait(&resource);

					write(ssock, "GO", 5);

					file = fopen(tmpst[1], "w");

					for(int i = 0; i < 400; i++)
					{
						
						read(ssock, buf, BUFSIZE);
						fprintf(file, "%s\n", buf);
					}
					//printf("!!!!!!%s\n", buf);

					for(int i = 0; i < 200; i++)
					{
						
						fprintf(file, "%s\n", buf);
						
					}
					
					printf("New text was written to the file\n");
					
					fclose(file);

					sem_post(&resource);
				}
				else{
					close(ssock);
					break;
				}

		}
		
	}
	pthread_exit(0);
}


void *writer_preference( void *s )
{
	char buf[BUFSIZE];
	int cc;
	int ssock = (int) s;
	int i = 0;
	char for_file[BUFSIZE];
	char *filename;

	/* start working for this guy */
	/* ECHO what the client says */

	for (;;)
	{
		
		if ( (cc = read( ssock, buf, 256 )) <= 0 )
		{
			printf( "The client has gone.\n" );
			close(ssock);
			break;
		}
		else
		{
				char *tmp;
				char *tmpst[252];
				int j = 0;
				
				filename = strtok(buf, " ");
				while(filename != NULL)
				{
					tmp = filename;
					tmpst[j] = filename;
					filename = strtok(NULL, " ");
					j++;
				}
				//printf("%s\n", tmpst[0]);

				if(strcmp(tmpst[0], "rclient") == 0)
				{
					sem_wait(&readTry);
					sem_wait(&rmutex);
					readcount++;

					if(readcount == 1)
					{
						sem_wait(&resource);
					}

					sem_post(&rmutex);
					sem_post(&readTry);

					printf("New Reader file is created\n");
					file = fopen(tmpst[1], "r");
					fgets(for_file, BUFSIZE, file);
					//sleep(3);
					for(int i = 0; i < 550; i++)
					{

						write(ssock, for_file, BUFSIZE);
						
					}

					sem_post(&rmutex);
					readcount--;

					if(readcount == 0)
					{
						sem_post(&resource);
					}

					sem_post(&rmutex);
					//fclose(file);
				}
				
				else if(strcmp(tmpst[0], "wclient") == 0)
				{
					sem_wait(&wmutex);
					writecounter++;
					if (writecounter == 1)
					{
						sem_wait(&readTry);
					}
					sem_post(&wmutex);

					sem_wait(&resource);

					write(ssock, "GO", 5);

					file = fopen(tmpst[1], "w");

					for(int i = 0; i < 400; i++)
					{
						
						read(ssock, buf, BUFSIZE);
						fprintf(file, "%s\n", buf);
					}
					//printf("!!!!!!%s\n", buf);

					for(int i = 0; i < 200; i++)
					{
						
						fprintf(file, "%s\n", buf);
						
					}
					
					printf("New text was written to the file\n");
					
					fclose(file);

					sem_post(&resource);
					sem_wait(&wmutex);
					writecounter--;
					if(writecounter == 0)
					{
						sem_post(&readTry);
					}
					sem_post(&wmutex);

				}
				else{
					close(ssock);
					break;
				}

		}
		
	}
	pthread_exit(0);
}

int main( int argc, char *argv[] )
{
	char			*service;
	struct sockaddr_in	fsin;
	int			alen;
	int			msock;
	int			ssock;
	int			rport = 0;
	char		*type;
	
	switch (argc) 
	{
		case	2:
			// No args? let the OS choose a port and tell the user
			rport = 1;
			type = argv[1];
			break;
		case	3:
			// User provides a port? then use it
			service = argv[1];
			type = argv[2];
			break;
		default:
			fprintf( stderr, "usage: server [port]\n" );
			exit(-1);
	}

	msock = passivesock( service, "tcp", QLEN, &rport );
	if (rport)
	{
		//	Tell the user the selected port
		printf( "server: port %d\n", rport );	
		fflush( stdout );
	}

	pthread_t	thr[THREADS];
	sem_init(&resource, 0, 1);
	sem_init(&rmutex, 0, 1);
	sem_init(&wmutex, 0, 1);
	sem_init(&readTry, 0, 1);
	
	for (int i = 0; i < THREADS; i++)
	{
		
		int	ssock;

		alen = sizeof(fsin);
		ssock = accept( msock, (struct sockaddr *)&fsin, &alen );
		if (ssock < 0)
		{
			fprintf( stderr, "accept: %s\n", strerror(errno) );
			break;
		}

		printf( "A client has arrived for echoes.\n" );
		fflush( stdout );

		if(strcmp(type, "rp") == 0)
		{	
			pthread_create( &thr[i++], NULL, reader_preference, (void *) ssock );
		}

		else if(strcmp(type, "wp") == 0)
		{
			pthread_create( &thr[i++], NULL, writer_preference, (void *) ssock );
		}

	}

	for (int j = 0; j < THREADS; j++ )
		pthread_join( thr[j], NULL );

	fclose(file);
	pthread_exit(0);
}
