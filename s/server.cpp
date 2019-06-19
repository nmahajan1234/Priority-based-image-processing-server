#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include<string.h>
#include<pthread.h>
#include<string.h>
#include <unistd.h>
#include <stdlib.h>
#include <opencv2/opencv.hpp>

int listen_fd;
int thread_name=0;

#define QUEUESIZE 8
#define LOOP 5

#define PRIO_GROUP 2

using namespace cv;

typedef struct {
	int buf[QUEUESIZE];
	long head, tail;
	int full, empty;
	pthread_mutex_t *mut;
	pthread_cond_t *notFull, *notEmpty;
} queue;

queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, int in);
void queueDel (queue *q, int *out);
void millisleep(int milliseconds);

void *worker_thread(void *args);
void *master_thread(void *args);

int main(int argc, char **argv)
{

	/*Setup thread priorites*/
	printf("Main: Creating thread priorities...\n");
	struct sched_param my_param;
	pthread_attr_t lp_attr;
	pthread_attr_t hp_attr;
	int min_priority,policy;

	/* MAIN-THREAD WITH LOW PRIORITY */
	my_param.sched_priority = sched_get_priority_min(SCHED_FIFO);
	min_priority = my_param.sched_priority;
	pthread_setschedparam(pthread_self(), SCHED_RR, &my_param);
	pthread_getschedparam (pthread_self(), &policy, &my_param);
	
	/* SCHEDULING POLICY AND PRIORITY FOR OTHER THREADS */
	pthread_attr_init(&lp_attr);
	pthread_attr_init(&hp_attr);
	
	pthread_attr_setinheritsched(&lp_attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setinheritsched(&hp_attr, PTHREAD_EXPLICIT_SCHED);
	
	pthread_attr_setschedpolicy(&lp_attr, SCHED_FIFO);
	pthread_attr_setschedpolicy(&hp_attr, SCHED_FIFO);
	
	my_param.sched_priority = min_priority + 1;
	pthread_attr_setschedparam(&lp_attr, &my_param);
	my_param.sched_priority = min_priority + 3;
	pthread_attr_setschedparam(&hp_attr, &my_param);
	
	/*End thread priorites*/
	printf("Main: Done creating thread priorites...\n");
	printf("\n");
	
	printf("Main: Binding Socket...\n");
	int socketNum;
	sscanf(argv[1],"%d",&socketNum);
	printf("Main: Socket number: %d\n",socketNum);
	char str[100];
	int comm_fd;
 
	struct sockaddr_in servaddr;
 
	//create socket
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
 
	//   bzero( &servaddr, sizeof(servaddr));
 
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htons(INADDR_ANY);
	servaddr.sin_port = htons(socketNum);
 
	if(bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0){
		printf("Socket Bind Failed");
		return 1;
	}

	printf("Bind compelted \n");

	printf("Creating fifo... \n");
	queue *fifo;
	fifo=queueInit();
	if(fifo==NULL){
		printf("Queue Init failed \n");
		exit(1);
	}

	printf("Starting to listen... \n");
	listen(listen_fd, 10);

	printf("Creating master thread... \n");
	pthread_t master;
	pthread_create(&master,&hp_attr,master_thread,fifo);



	printf("Creating worker threads...\n");	
	pthread_t worker1,worker2,worker3;
	pthread_create(&worker1,&lp_attr,worker_thread,fifo);
	pthread_create(&worker2,&lp_attr,worker_thread,fifo);
	pthread_create(&worker2,&lp_attr,worker_thread,fifo);

  /*  
    while(  comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL))
    {
 
 	printf("Connection accepted \n");
        bzero( str, 100);
 
        read(comm_fd,str,100);
 
        printf("Echoing back - %s",str);
 
        write(comm_fd, str, strlen(str)+1);
 	
	close(comm_fd);
    }
*/
	while(1)
	{}
   	
}


void *worker_thread(void *q){
	int name = thread_name++;
	printf("worker_thread %d: spawned \n",name);
	
	pthread_t thread_id = pthread_self();
	struct sched_param param;
	int priority,policy,ret;
	
	//pthread_barrier_wait(&mybarrier);

	ret=pthread_getschedparam(thread_id,&policy,&param);
	priority=param.sched_priority;
	printf("worker_thread %d: prioirty-%d\n",priority);
	

	
	int fd;
	char str[100];

	queue *fifo;
	fifo=(queue *)q;
	while(1){
		pthread_mutex_lock(fifo->mut);
		while(fifo->empty){
			printf("worker_thread %d: queue EMPTY. \n",name);
			pthread_cond_wait(fifo->notEmpty, fifo->mut);
		}
		queueDel(fifo,&fd);
		pthread_mutex_unlock (fifo->mut);
		pthread_cond_signal (fifo->notFull);
		printf ("worker_thread %d: recieved fd:%d.\n",name, fd);
		
		/* RECIEVE DATA*/

		printf("worker_thread %d: receiving data... \n",name);
		int height = 330;
		int width = 586;
		Mat img=Mat::zeros(height,width,CV_8UC3);		
		int imgSize=img.total()*img.elemSize();
		uchar sockData[imgSize];
		int bytes=0;
		for (int i=0;i<imgSize; i+=bytes){
   			 if ((bytes = recv(fd, sockData +i, imgSize  - i, 0)) == -1)
			printf("recv failed \n");      		
		}

		int ptr=0;
		
		for(int i=0; i<img.rows;i++){
			for(int j=0; j<img.cols; j++){
				img.at<cv::Vec3b>(i,j) = cv::Vec3b(sockData[ptr+ 0],sockData[ptr+1],sockData[ptr+2]);
   				ptr=ptr+3;
   			}
  		}


		
		printf("worker_thread %d: Image Data... \n",name);
		printf("worker_thread %d: Type: %d\n",name,img.type());
		printf("worker_thread %d: Cols: %d\n",name,img.cols);
		printf("worker_thread %d: Rows: %d\n",name,img.rows);

		printf("\n");
		/* END RECIEVE DATA */


		/* PROCESS DATA */
		Mat gray_image;
		
 		cvtColor( img, gray_image, COLOR_BGR2GRAY );
		/* END PROCESS DATA */


		/* SEND DATA */
		printf("worker_thread %d: Sending gray image... \n",name);
		gray_image=(gray_image.reshape(0,1));
		imgSize=gray_image.total()*gray_image.elemSize();
		
		bytes=send(fd,gray_image.data,imgSize,0);
		/* END SEND DATA */
		printf("worker_thread %d: Gray image sent \n\n",name);

		millisleep(10000);	
		close(fd);
		printf("worker_thread %d: Client connection closed \n",name);
		
		

	}

	/*
	while(  fd = accept(listen_fd, (struct sockaddr*) NULL, NULL))
	{ 
		printf("Connection accepted on thread %d \n",name);
      	 	bzero( str, 100);
 
		read(fd,str,100);
 
	        printf("Echoing back - %s",str);
 
	        write(fd, str, strlen(str)+1);
 	
		close(fd);
	} 
	*/
	
}


void *master_thread(void *q){

	printf("Master thread spawned \n");

	pthread_t thread_id = pthread_self();
	struct sched_param param;
	int priority, policy, ret;

	ret = pthread_getschedparam (thread_id, &policy, &param);
	priority = param.sched_priority;	
	printf("master_thread: priority-%d \n", priority);
	
	int fd;
	queue *fifo;
	fifo=(queue *)q;
	while(  fd = accept(listen_fd, (struct sockaddr*) NULL, NULL)){
		pthread_mutex_lock (fifo->mut);
		printf("master_thread: Connection accepted fd:%d \n",fd);
		while(fifo->full){
			printf("master_thread: queue FULL. \n");
			pthread_cond_wait(fifo->notFull,fifo->mut);
		}
		queueAdd(fifo,fd);
		pthread_mutex_unlock (fifo->mut);
		pthread_cond_signal (fifo->notEmpty);
		millisleep (200);

	}
}





queue *queueInit (void)
{
	queue *q;

	q = (queue *)malloc (sizeof (queue));
	if (q == NULL) return (NULL);

	q->empty = 1;
	q->full = 0;
	q->head = 0;
	q->tail = 0;
	q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
	pthread_mutex_init (q->mut, NULL);
	q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
	pthread_cond_init (q->notFull, NULL);
	q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
	pthread_cond_init (q->notEmpty, NULL);
	
	return (q);
}

void queueDelete (queue *q)
{
	pthread_mutex_destroy (q->mut);
	free (q->mut);	
	pthread_cond_destroy (q->notFull);
	free (q->notFull);
	pthread_cond_destroy (q->notEmpty);
	free (q->notEmpty);
	free (q);
}

void queueAdd (queue *q, int in)
{
	q->buf[q->tail] = in;
	q->tail++;
	if (q->tail == QUEUESIZE)
		q->tail = 0;
	if (q->tail == q->head)
		q->full = 1;
	q->empty = 0;

	return;
}

void queueDel (queue *q, int *out)
{
	*out = q->buf[q->head];

	q->head++;
	if (q->head == QUEUESIZE)
		q->head = 0;
	if (q->head == q->tail)
		q->empty = 1;
	q->full = 0;

	return;
}

void millisleep(int milliseconds)
{
      usleep(milliseconds * 1000);
}





