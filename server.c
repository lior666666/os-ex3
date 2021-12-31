#include "segel.h"
#include "request.h"
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



// A linked list node
typedef struct node {
    int data;
    struct node* next;
    int size;
}* Node;

Node createNode() {
	Node ptr = malloc(sizeof(*ptr));
	if(!ptr) {
		return NULL;
	}
	ptr->data = 0;
	ptr->next = NULL;
	ptr->size = 0;
	return ptr;
}
 
void destroyList(Node ptr) {
	while(ptr) {
		Node toDelete = ptr;
		ptr = ptr->next;
		free(toDelete);
	}
}

int pushNode(Node head, int new_data)
{
    Node ptr = malloc(sizeof(*ptr));
    if(!ptr) {
		return 0;
	}
	ptr->data = new_data;
	ptr->next = NULL;
	ptr->size = 0;
	head->size = head->size + 1;
	Node last = head;
	while (last->next != NULL)
		last = last->next;
	last->next = ptr;
	return 1;
}

int RemoveOldestNode(Node head) //return data of node was deleted
{
	Node last = head;
	int data;
	while (last->next != NULL)
		last = last->next;
	data = last->data;
	free(last);
	head->size = head->size - 1;
	return data;
}

int RemoveNode(Node head, int to_remove)
{
	Node tmp = head;
	Node tmp2;
	while (tmp->next != NULL){
		if (tmp->next->data == to_remove){
			tmp2 = tmp->next->next;
			free(tmp->next);
			tmp->next = tmp2;
			head->size = head->size - 1;
			return 1;
		}
		tmp = tmp->next;
	}
	return 0;
}

int isEmpty(Node head)
{
	return head->size == 0;
}

int queueSize(Node head)
{
	return head->size;
}


// We need to check if it's should be here:
// ALSO NEED TO CHECK HOW TO INITIALIZE THEM
pthread_cond_t* c;
pthread_cond_init(c, NULL);
pthread_mutex_t* m;
pthread_mutex_init(m, NULL);

Node waiting_requests = createNode();
Node working_requests = createNode();


// HW3: Parse the new arguments to
void getargs(int *port, int argc, int *thread_number, int *queue_size , char *argv[])
{
	//need to change the checking of args number
    if (argc < 2) {
		fprintf(stderr, "Usage: %s <port>\n", argv[0]);
		exit(1);
    }
    *port = atoi(argv[1]);
    *thread_number = atoi(argv[2]);
    *queue_size = atoi(argv[3]);
}

void* handle(void* list)
{
    while (1) {
		pthread_mutex_lock(m);
		while(isEmpty(waiting_requests)) // stay in the loop if there is no waiting requests
		{
			pthread_cond_wait(c, m);
		}
		//get connfd of the oldest in waiting queue (and delete it)
		int connfd = RemoveOldestNode(waiting_requests);
		pushNode(working_requests, connfd);
		// 
		// HW3: In general, don't handle the request in the main thread.
		// Save the relevant info in a buffer and have one of the worker threads 
		// do the work. 
		// 
		
		pthread_mutex_unlock(m);
		requestHandle(waiting_requests->data);
		pthread_mutex_lock(m);
		
		Close(waiting_requests->data);
		RemoveNode(working_requests, connfd);
		pthread_mutex_unlock(m);
    }
}

int main(int argc, char *argv[])
{
    int listenfd, port, threads_number, queue_size, connfd, clientlen;
    getargs(&port, argc, &threads_number, &queue_size, argv);
	struct sockaddr_in clientaddr;
    // 
    // HW3: Create some threads...
    //
    pthread_t threads[threads_number];
    for(unsigned int i =0; i<threads_number; i++)
	{
		pthread_create(&threads[i], NULL, handle, NULL);
	}
	
	listenfd = Open_listenfd(port);
	while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
		if (queueSize(waiting_requests) + queueSize(working_requests) <= queue_size) {
			pushNode(waiting_requests, connfd);
			pthread_cond_signal(c);
		}
		else {
			//overload!!
			//here is part 2 overload handlings
		}
	}
	destroyList(waiting_requests);
	destroyList(working_requests);
}
