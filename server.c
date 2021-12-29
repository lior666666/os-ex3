#include "segel.h"
#include "request.h"
#include <pthread.h>
//#include <condition_variable>

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//


// We need to check if it's should be here:
// ALSO NEED TO CHECK HOW TO INITIALIZE THEM
cond_t c;
mutex_lock m;
int working_threads_counter = 0;
int waiting_threads_counter = 0;

// HW3: Parse the new arguments too
void getargs(int *port, int argc, int *thread_number, int *queue_size , char *argv[])
{
    if (argc < 2) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    *thread_number = atoi(arv[2]);
    *queue_size = atoi(argv[3]);
}

void* handle(void* port)
{
	
	//struct sockaddr_in clientaddr;
	struct sockaddr_in clientaddr;
	listenfd = Open_listenfd(port);
    cond_t c;
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
	
	// 
	// HW3: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	// 
	
	requestHandle(connfd);

	Close(connfd);
    }
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, threads_number, queue_size;
    
    getargs(&port, argc, &threads_number, &queue_size, argv);
    
    // 
    // HW3: Create some threads...
    //
    pthread_t threads[threads_number];
    
    
    for(unsigned int i =0; i<threads_number; i++)
    {
		pthread_create(&threads[i], NULL, handle, (void*) &port);
		pthread_join(threads[i], NULL);
	}
	
   

}


    


 
