/*
 *Author: Srinath
  Rference: From Dr. Seiwart question 2 code
 *
 * */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

struct update_axes
{
	double X,Y,Z,Roll,Pitch,Yaw;	
	struct timespec timestamp;	
}axes;

pthread_mutex_t lock;
void *myThread1(void *);
void *myThread2(void *);
void *myThread3(void *);


void *myThread1(void *arg)
{
		pthread_mutex_lock(&lock);
		printf("\n mutex lock for thread 1");
		
		axes.X = rand() % 100 + 1;
		axes.Y = rand() % 100 + 1;
		axes.Z = rand() % 100 + 1;
		axes.Roll = rand() % 100 + 1;
		axes.Pitch = rand() % 100 + 1;
		axes.Yaw = rand() % 100 + 1;
		sleep(22);
		clock_gettime(CLOCK_REALTIME, &axes.timestamp);
		printf("\n Timestamp secs: %ld  nsecs: %ld:", axes.timestamp.tv_sec,  axes.timestamp.tv_nsec);
		printf(" \t Updated values : X: %f, Y: %f, Z: %f, Roll: %f, Pitch: %f, Yaw: %f", axes.X, axes.Y, axes.Z, axes.Roll, axes.Pitch, axes.Yaw);
		pthread_mutex_unlock(&lock);
		printf("\n mutex unlock for thread 1");
		
	
}

void *myThread2(void *arg)
{
		pthread_mutex_lock(&lock);
		printf("\n mutex lock for thread 2");
		clock_gettime(CLOCK_REALTIME, &axes.timestamp);

		printf("\n Timestamp secs: %ld  nsecs: %ld:", axes.timestamp.tv_sec,  axes.timestamp.tv_nsec);
		printf(" \t Read values : X: %f, Y: %f, Z: %f, Roll: %f, Pitch: %f, Yaw: %f", axes.X, axes.Y, axes.Z, axes.Roll, axes.Pitch, axes.Yaw);
		pthread_mutex_unlock(&lock);
		printf("\n mutex unlock for thread 2");
	

	
}

void *myThread3(void *arg)
{
    
        printf("\n Entered thread 3");
	    struct timespec *wait;
	    int ret;
	    wait=(struct timespec *)(malloc(sizeof(struct timespec)));
	    wait->tv_sec=10;
	    wait->tv_nsec=0;
	    clock_gettime(CLOCK_REALTIME, &axes.timestamp);

	    ret=pthread_mutex_timedlock(&lock,wait);
	    printf("\n The return value is: %d", ret);
	    if(ret!=0)
	    {
		printf(" \n ERROR: Wait timed out at: secs: %ld  nsecs: %ld:",axes.timestamp.tv_sec,  axes.timestamp.tv_nsec); 
	    }
	    else
	    {
	        pthread_mutex_unlock(&lock);
	        printf("\n mutex unlock for thread 3"); 
	    }
    
	
}

int main() 
{ 
	 
	pthread_t pid1, pid2, pid3;
	srand(time(0));
	
	if (pthread_mutex_init(&lock, NULL) != 0)
	{
		printf("\n ERROR: pthread_mutex_init");
	}
	for(int i =0 ; i<3 ;i++)
	{
	    pthread_create(&pid1,NULL,myThread1,NULL);
    	sleep(1);
    	pthread_create(&pid2,NULL,myThread2,NULL);
	sleep(1);
	pthread_create(&pid3,NULL,myThread3,NULL);
		    
    	pthread_join(pid1, NULL);
    	pthread_join(pid2, NULL);
    	pthread_join(pid3, NULL);
	}
	
	
	pthread_mutex_destroy(&lock);


	return 0; 
}
