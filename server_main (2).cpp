#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <queue>
#include <iostream>

#define MAX  80
#define PORT 8080
#define SA struct sockaddr
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

typedef void* (*thread_function) (void*);

class pool_function
{
private:
    thread_function function;
    void* argument;
public:
    pool_function(thread_function func): function(func), argument(NULL) {}

   void set_arg(void* arg) {argument = arg;}

    inline thread_function get_function()
    {
        return this->function;
    }

    inline void* get_argument()
    {
        return this->argument;
    }
};

class parallel_scheduler
{
private:
    std::queue<pool_function>* queue;
    std::size_t size;
    pthread_mutex_t* queue_lock;
    pthread_cond_t* queue_empty;
    pthread_t* threads;

    void initialize();
    static void* thread_function(void* arg);
public:
    parallel_scheduler(std::size_t capacity);
    void run(pool_function pf, void* arg);
    ~parallel_scheduler();
};

parallel_scheduler::parallel_scheduler(std::size_t capacity): size(capacity) 
{
    this->queue = new std::queue<pool_function>;
    this->threads = new pthread_t[size];
    this->queue_lock = new pthread_mutex_t;
    pthread_mutex_init(this->queue_lock, NULL);
    this->queue_empty = new pthread_cond_t;
    pthread_cond_init(this->queue_empty, NULL);
    initialize();    
}


void* parallel_scheduler::thread_function(void* arg)
{
    parallel_scheduler* pool = (parallel_scheduler*) arg;

    while(true)
    {
        pthread_mutex_lock(pool->queue_lock);
        if(pool->queue->size() == 0) 
        {
            pthread_cond_wait(pool->queue_empty, pool->queue_lock);
            pool_function next_function = pool->queue->front();
            pool->queue->pop();
            std::cout << "Executing next function" << std::endl;
            pthread_mutex_unlock(pool->queue_lock);

            next_function.get_function()(next_function.get_argument());

        }

    }
}

void parallel_scheduler::initialize()
{
    for(int i = 0; i < this->size; ++i)
    {
        int result = pthread_create(&(this->threads[i]), 
            NULL, thread_function, this);
        if(result < 0)
        {
            exit(result);
        }
    }
}

void parallel_scheduler::run(pool_function pf, void *arg)
{
    pthread_mutex_lock(this->queue_lock);
    pf.set_arg(arg);
    this->queue->push(pf);
    std::cout << "Adding function to queue" << std::endl;
    pthread_mutex_unlock(this->queue_lock);

    pthread_cond_signal(this->queue_empty);
}

parallel_scheduler::~parallel_scheduler()
{
    delete [] this->threads;
    delete this->queue;
    pthread_mutex_destroy(this->queue_lock);
    delete this->queue_lock;
    pthread_cond_destroy(queue_empty);
    delete this->queue_empty;
}


// Function designed for chat between client and server.
void * func(void* arg)
{
	int connfd = *((int *)arg);
	char buff[MAX];
	int n;
	// infinite loop for chat
	for (;;) {

		bzero(buff, MAX);

		// read the message from client and copy it in buffer
		read(connfd, buff, sizeof(buff));

		pthread_mutex_lock(&lock);
		// print buffer which contains the client contents
		printf("From client: %s\n", buff);
		pthread_mutex_unlock(&lock);

		if (strncmp("exit", buff, 4) == 0) {
			break;
		}
		
	}
   close(connfd);
   pthread_exit(NULL);	
}

// Driver function
int main()
{
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;
    
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 50)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");
    
    socklen_t len = sizeof(cli);

    pthread_t thread_ids[10];
    std::size_t pool_capacity = 10;
    parallel_scheduler scheduler(pool_capacity);

    int i = 0;
    while(1) {
        // Accept the data packet from client and verification
        connfd = accept(sockfd, (SA*)&cli, &len);
        if (connfd < 0) {
	    printf("server accept failed...\n");
	    exit(0);
        }
        else {
            printf("server has accepted the client...\n");
        }
        scheduler.run(pool_function(func), &connfd);
    }

    // After chatting close the socket
    close(sockfd);
}
