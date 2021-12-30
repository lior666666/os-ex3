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
struct Node {
    int data;
    struct Node* next;
    struct Node* last = NULL;
    int size = 0;
};

/* Given a reference (pointer to pointer) to the head of a
   list and an int, inserts a new node on the front of the
   list. */
void push_back(struct Node** head_ref, int new_data)
{
    struct Node* new_node = (struct Node*)malloc(sizeof(struct Node));
    new_node->data = new_data;
    struct Node *temp = *head_ref, *prev;
    while (temp != NULL) {
        prev = temp;
        temp = temp->next;
    }
    prev->next = new_node;
    new_node->next = NULL;
    size++;
}

void deleteNode(struct Node** head_ref, int key)
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
    size--;
}




// We need to check if it's should be here:
// ALSO NEED TO CHECK HOW TO INITIALIZE THEM
cond_t c;
pthread_cond_init(&c, NULL);
mutex_lock m;
pthread_mutex_init(&m, NULL);

struct Node* waiting_requests = NULL;
struct Node* working_requests = NULL;


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

void* handle(void* listenfd)
{
	

    //cond_t c;
    while (1) {
		mutex_lock(&m);
		while((struct Node* waiting_requests == NULL)
		{
			cond_wait(&c, &m);
		}
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
    int listenfd, port, threads_number, queue_size;

    getargs(&port, argc, &threads_number, &queue_size, argv);
	int connfd, clientlen;
	struct sockaddr_in clientaddr;
    // 
    // HW3: Create some threads...
    //
    pthread_t threads[threads_number];
    for(unsigned int i =0; i<threads_number; i++)
	{
		pthread_create(&threads[i], NULL, handle, (void*) waiting_requests);
	}
	listenfd = Open_listenfd(port);
	while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(*(int*)listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
		
	}
	
	pthread_join(threads[0], NULL);

}
