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
struct request_s {
    int connfd;
    time_t arrival_time_sec;
    suseconds_t arrival_time_usec;
    time_t dispatch_time_sec;
    suseconds_t dispatch_time_usec;
};

// A linked list node
struct Node {
    struct request_s* data;
    struct Node* next;
    int size;
};

/* Given a reference (pointer to pointer) to the head of a
   list and an int, inserts a new node on the front of the
   list. */
void pushNode(struct Node** head_ref, struct request_s* new_data)
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
    if (temp != NULL && temp->data->connfd == key) {
        *head_ref = temp->next; // Changed head
        free(temp); // free old head
        return;
    }
 
    // Search for the key to be deleted, keep track of the
    // previous node as we need to change 'prev->next'
    while (temp != NULL && temp->data->connfd != key) {
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

int removeRandomElement(struct Node** head_ref) {
	    /* Intializes random number generator */
	    time_t t;
        srand((unsigned) time(&t));
        
        int index = (rand() % (*head_ref)->size) + 1;
        
        struct Node *current = (*head_ref);
        struct Node *prev = current;
        for (int i = 0; i < index; i++) {
            prev = current;
            current = current->next;
        }
        prev->next = current->next;
        struct request_s* data = current->data;
        free(current);
        (*head_ref)->size = (*head_ref)->size -1;
        return data->connfd;
    }

void printList(struct Node* node)
{
    while (node != NULL) {
        fprintf(stdout, " %d \n", node->data->connfd);
        node = node->next;
    }
}
 
void destroyList(struct Node** ptr) {
	while(*ptr) {
		struct Node *toDelete = *ptr;
		*ptr = (*ptr)->next;
		free(toDelete->data);
		free(toDelete);
	}
}

int isEmpty(struct Node** head)
{
	return (*head)->size == 0;
}

struct request_s* RemoveOldestNode(struct Node** head) //return data of node was deleted
{
	if (!isEmpty(head))
	{
		struct Node* real_head = (*head)->next;
		struct request_s* data = real_head->data;
		struct Node* new_head = real_head->next;
		free(real_head);
		(*head)->next = new_head;
	    (*head)->size = (*head)->size -1;
		return data;
	}
	return NULL;
}

int queueSize(struct Node** head)
{
	return (*head)->size;
}

pthread_cond_t c = PTHREAD_COND_INITIALIZER;
pthread_cond_t c_overload = PTHREAD_COND_INITIALIZER;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

struct Node s_waiting_requests = {NULL, NULL, 0};
struct Node s_working_requests = {NULL, NULL, 0};
struct Node* waiting_requests = &(s_waiting_requests);
struct Node* working_requests = &(s_working_requests);

// A thread struct
struct thread_s {
    int id;
    int thread_counter;
    int thread_static_counter;
    int thread_dynamic_counter;
};

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

void* handle(void* ptr)
{
	struct thread_s* thread = (struct thread_s*)ptr;
    while (1) {
		pthread_mutex_lock(&m);
		while(isEmpty(&waiting_requests)) // stay in the loop if there is no waiting requests
		{
			pthread_cond_wait(&c, &m);
		}
		fprintf(stdout, "thread!\n");
		//get connfd of the oldest in waiting queue (and delete it)
		struct request_s* req_info = RemoveOldestNode(&waiting_requests);
		int connfd = req_info->connfd;
		pushNode(&working_requests, req_info);
		
		pthread_mutex_unlock(&m);
		requestHandle(connfd, req_info->arrival_time_sec, req_info->arrival_time_usec, req_info->dispatch_time_sec, req_info->dispatch_time_usec, thread->id, &(thread->thread_counter), &(thread->thread_static_counter), &(thread->thread_dynamic_counter));
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
		struct thread_s* thread = (struct thread_s*)malloc(sizeof(struct thread_s));
		thread->id = i;
		thread->thread_counter = 0;
		thread->thread_static_counter = 0;
		thread->thread_dynamic_counter = 0;
		pthread_create(&threads[i], NULL, handle, (void*)thread);
	}
	
	listenfd = Open_listenfd(port);
	while (1) {
		clientlen = sizeof(clientaddr);
		fprintf(stdout, "STARTED!\n");
		struct request_s* req = (struct request_s*)malloc(sizeof(struct request_s));
		struct timeval current_time;
		connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
		req->connfd = connfd;
		gettimeofday(&current_time, NULL);
		req->arrival_time_sec = current_time.tv_sec;
		req->arrival_time_usec = current_time.tv_usec;
		waiting_queue = queueSize(&waiting_requests);
		working_queue = queueSize(&working_requests);
		fprintf(stdout, "%d, %d, %d\n", waiting_queue, working_queue, queue_size);
		if (waiting_queue + working_queue < queue_size) {
			pthread_mutex_lock(&m);
			fprintf(stdout, "before pushing!\n");
			pushNode(&waiting_requests, req);
			printList(waiting_requests->next);
			pthread_cond_signal(&c);
			pthread_mutex_unlock(&m);
		}
		else { //overload!!
			fprintf(stdout, "check!\n");
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
				pushNode(&waiting_requests, req);
				printList(waiting_requests->next);
				pthread_cond_signal(&c);
				pthread_mutex_unlock(&m);
			}
			else if (strcmp(schedalg, "dh") == 0) {
				pthread_mutex_lock(&m);
				waiting_queue = queueSize(&waiting_requests);
				if (waiting_queue == 0){
					Close(connfd);
					continue;
				}
				Close((RemoveOldestNode(&waiting_requests))->connfd);
				waiting_queue = queueSize(&waiting_requests);
				working_queue = queueSize(&working_requests);
				fprintf(stdout, "%d, %d, %d\n", waiting_queue, working_queue, queue_size);
				if (waiting_queue + working_queue < queue_size) {
					fprintf(stdout, "before pushing! dh \n");
					pushNode(&waiting_requests, req);
					printList(waiting_requests->next);
					pthread_cond_signal(&c);
				}
				pthread_mutex_unlock(&m);
			}
			else if (strcmp(schedalg, "random") == 0) {
				pthread_mutex_lock(&m);
				waiting_queue = queueSize(&waiting_requests);
				if (waiting_queue == 0){
					Close(connfd);
					continue;
				}
				int half_size = round(((double)queueSize(&waiting_requests))/2 + 0.01);
				for(int i =0; i<half_size; i++)
				{
				    Close(removeRandomElement(&waiting_requests));
				}
				waiting_queue = queueSize(&waiting_requests);
				working_queue = queueSize(&working_requests);
				if (waiting_queue + working_queue < queue_size) {
					pushNode(&waiting_requests, req);
					printList(waiting_requests->next);
					pthread_cond_signal(&c);
				}
				pthread_mutex_unlock(&m);
			}
			else if (strcmp(schedalg, "dt") == 0) {
				//should continue to the next iteration in order to accept new requests
				Close(connfd);
				continue;
			}
		}
	}
	destroyList(&waiting_requests);
	destroyList(&working_requests);
}
