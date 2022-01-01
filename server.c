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

pthread_cond_t c = PTHREAD_COND_INITIALIZER;
pthread_cond_t c_overload = PTHREAD_COND_INITIALIZER;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

struct Node s_waiting_requests = {0, NULL, 0};
struct Node s_working_requests = {0, NULL, 0};
struct Node* waiting_requests = &(s_waiting_requests);
struct Node* working_requests = &(s_working_requests);

void getargs(int *port, int argc, int *thread_number, int *queue_size, const char** schedalg, char *argv[])
{
	//need to change the checking of args number
	if (argc != 5) {
		fprintf(stderr, "Usage: %s <portnum> <threads> <queue_size> <schedalg>\n", argv[0]);
		exit(1);
	}
    *port = atoi(argv[1]);
    *thread_number = atoi(argv[2]);
    *queue_size = atoi(argv[3]);
    *schedalg = argv[4];
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
		
		pthread_mutex_unlock(&m);
		requestHandle(connfd);
		Close(connfd);
		pthread_mutex_lock(&m);
		
		RemoveNode(&working_requests, connfd);
		pthread_cond_signal(&c_overload);
		pthread_mutex_unlock(&m);
    }
}

int main(int argc, char *argv[])
{
    int listenfd, port, threads_number, queue_size, connfd, clientlen;
    const char* schedalg;
    int waiting_queue, working_queue;
    getargs(&port, argc, &threads_number, &queue_size, &schedalg, argv);
	struct sockaddr_in clientaddr;

    // HW3: Create some threads...
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
		waiting_queue = queueSize(&waiting_requests);
		working_queue = queueSize(&working_requests);
		fprintf(stdout, "%d, %d, %d\n", waiting_queue, working_queue, queue_size);
		if (waiting_queue + working_queue < queue_size) {
			fprintf(stdout, "before pushing!\n");
			pushNode(&waiting_requests, connfd);
			printList(waiting_requests->next);
			pthread_cond_signal(&c);
		}
		else { //overload!!
			if (strcmp(schedalg, "block") == 0) {
				fprintf(stdout, "before pushing! before while block!!\n");
				pthread_mutex_lock(&m);
				while(waiting_queue + working_queue >= queue_size) { // stay in the loop if there is overload
					pthread_cond_wait(&c_overload, &m);
					waiting_queue = queueSize(&waiting_requests);
					working_queue = queueSize(&working_requests);
				}
				fprintf(stdout, "before pushing! after while block!!\n");
				fprintf(stdout, "%d, %d, %d\n", waiting_queue, working_queue, queue_size);
				pushNode(&waiting_requests, connfd);
				printList(waiting_requests->next);
				pthread_cond_signal(&c);
				pthread_mutex_unlock(&m);
			}
			else if (strcmp(schedalg, "dh") == 0) {
				RemoveOldestNode(&waiting_requests);
				waiting_queue = queueSize(&waiting_requests);
				working_queue = queueSize(&working_requests);
				fprintf(stdout, "%d, %d, %d\n", waiting_queue, working_queue, queue_size);
				if (waiting_queue + working_queue < queue_size) {
					fprintf(stdout, "before pushing! dh \n");
					pushNode(&waiting_requests, connfd);
					printList(waiting_requests->next);
					pthread_cond_signal(&c);
				}
			}
			else if (strcmp(schedalg, "random") == 0) {
				
			}
			else if (strcmp(schedalg, "dt") == 0) {
				
			}
		}
	}
	destroyList(&waiting_requests);
	destroyList(&working_requests);
}
