#include "segel.h"
#include "request.h"


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
struct Node {
    int data;
    struct Node* next;
    int size;
};
/*
struct Node* makeDummy()
{
	struct Node* new_node = (struct Node*)malloc(sizeof(struct Node));
	new_node->data = 0;
	new_node->size = 0;
	new_node->next = NULL;
	return new_node;
}
* */


/* Given a reference (pointer to pointer) to the head of a
   list and an int, inserts a new node on the front of the
   list. */
void pushNode(struct Node** head_ref, int new_data)
{
    struct Node* new_node = (struct Node*)malloc(sizeof(struct Node));
    new_node->data = new_data;
    struct Node *temp = *head_ref;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = new_node;
    new_node->next = NULL;
    (*head_ref)->size = (*head_ref)->size +1;
}

void RemoveNode(struct Node** head_ref, int key)
{
    // Store head node
    struct Node *temp = *head_ref, *prev;
 
    // If head node itself holds the key to be deleted
    if (temp != NULL && temp->data == key) {
        *head_ref = temp->next; // Changed head
        free(temp); // free old head
        return;
    }
 
    // Search for the key to be deleted, keep track of the
    // previous node as we need to change 'prev->next'
    while (temp != NULL && temp->data != key) {
        prev = temp;
        temp = temp->next;
    }
 
    // If key was not present in linked list
    if (temp == NULL)
        return;
 
    // Unlink the node from linked list
    prev->next = temp->next;
 
    free(temp); // Free memory
    (*head_ref)->size = (*head_ref)->size -1;
}

void printList(struct Node* node)
{
    while (node != NULL) {
        fprintf(stdout, " %d \n", node->data);
        node = node->next;
    }
}
 
void destroyList(struct Node** ptr) {
	while(*ptr) {
		struct Node *toDelete = *ptr;
		*ptr = (*ptr)->next;
		free(toDelete);
	}
}

int isEmpty(struct Node** head)
{
	return (*head)->size == 0;
}

int RemoveOldestNode(struct Node** head) //return data of node was deleted
{
	if (!isEmpty(head))
	{
		struct Node* real_head = (*head)->next;
		int data = real_head->data;
		struct Node* new_head = real_head->next;
		free(real_head);
		(*head)->next = new_head;
	    (*head)->size = (*head)->size -1;
		return data;
	}
	return 0;
}



int queueSize(struct Node** head)
{
	return (*head)->size;
}


// We need to check if it's should be here:
// ALSO NEED TO CHECK HOW TO INITIALIZE THEM
pthread_cond_t c = PTHREAD_COND_INITIALIZER;
//pthread_cond_init((&c), NULL);
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_init((&m), NULL);

struct Node s_waiting_requests = {0, NULL, 0};
struct Node s_working_requests = {0, NULL, 0};
struct Node* waiting_requests = &(s_waiting_requests);
struct Node* working_requests = &(s_working_requests);

/*
struct Node* waiting_requests = (struct Node*)malloc(sizeof(struct Node));
waiting_requests>data = 0;
waiting_requests->size = 0;
waiting_requests->next = NULL;

struct Node* working_requests = (struct Node*)malloc(sizeof(struct Node));
working_requests->data = 0;
working_requests->size = 0;
working_requests->next = NULL;
*/



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
		pthread_mutex_lock(&m);
		while(isEmpty(&waiting_requests)) // stay in the loop if there is no waiting requests
		{
			pthread_cond_wait(&c, &m);
		}
		//get connfd of the oldest in waiting queue (and delete it)
		int connfd = RemoveOldestNode(&waiting_requests);
		pushNode(&working_requests, connfd);
		// 
		// HW3: In general, don't handle the request in the main thread.
		// Save the relevant info in a buffer and have one of the worker threads 
		// do the work. 
		// 
		
		pthread_mutex_unlock(&m);
		requestHandle(connfd);
		Close(connfd);
		pthread_mutex_lock(&m);
		
		RemoveNode(&working_requests, connfd);
		pthread_mutex_unlock(&m);
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
		fprintf(stdout, "STARTED!\n");
		connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
		if (queueSize(&waiting_requests) + queueSize(&working_requests) <= queue_size) {
			fprintf(stdout, "before pushing!\n");
			pushNode(&waiting_requests, connfd);
			printList(waiting_requests->next);
			pthread_cond_signal(&c);
		}
		//else 
		//{
			//overload!!
			//here is part 2 overload handlings
		//}
	}
	destroyList(&waiting_requests);
	destroyList(&working_requests);
}
