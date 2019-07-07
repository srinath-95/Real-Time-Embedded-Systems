/****************************************************************************/
/*                                                                          */
/* Sam Siewert - 10/14/97                                                   */
/*modified the code to support this file in Linux
 * by adding threads instead of tasks                                       */
/*                                                                          */
/****************************************************************************/

#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"
#include <mqueue.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>        
#include <sys/stat.h>    

// Added / to support in Linux
#define SNDRCV_MQ "/myheap_mq"
#define ERROR -1

struct mq_attr mq_attr;
static mqd_t mymq;

/* receives pointer to heap, reads it, and deallocate heap memory */

void receiver(void)
{
  char buffer[sizeof(void *)+sizeof(int)];
  void *buffptr; 
  int prio;
  int nbytes;
  int count = 0;
  int id;
 
  while(1) {

    /* read oldest, highest priority msg from the message queue */

    printf("Reading %ld bytes\n", sizeof(void *));
  
    if((nbytes = mq_receive(mymq, buffer, (size_t)(sizeof(void *)+sizeof(int)), &prio)) == ERROR)
/*
    if((nbytes = mq_receive(mymq, (void *)&buffptr, (size_t)sizeof(void *), &prio)) == ERROR)
*/
    {
      perror("mq_receive");
    }
    else
    {
      memcpy(&buffptr, buffer, sizeof(void *));
      memcpy((void *)&id, &(buffer[sizeof(void *)]), sizeof(int));
      printf("receive: ptr msg 0x%X received with priority = %d, length = %d, id = %d\n", buffptr, prio, nbytes, id);

      printf("contents of ptr = \n%s\n", (char *)buffptr);

      free(buffptr);

      printf("heap space memory freed\n");

    }
    
  }

}


static char imagebuff[4096];

void sender(void)
{
  char buffer[sizeof(void *)+sizeof(int)];
  void *buffptr;
  int prio;
  int nbytes;
  int id = 999;


  while(1) {

    /* send malloc'd message with priority=30 */

    buffptr = (void *)malloc(sizeof(imagebuff));
    strcpy(buffptr, imagebuff);
    printf("Message to send = %s\n", (char *)buffptr);

    printf("Sending %ld bytes\n", sizeof(buffptr));

    memcpy(buffer, &buffptr, sizeof(void *));
    memcpy(&(buffer[sizeof(void *)]), (void *)&id, sizeof(int));

    if((nbytes = mq_send(mymq, buffer, (size_t)(sizeof(void *)+sizeof(int)), 30)) == ERROR)
    {
      perror("mq_send");
    }
    else
    {
      printf("send: message ptr 0x%X successfully sent\n", buffptr);
    }
	
    sleep(3);
    //taskDelay(3000);

  }
  
}


static int sid, rid;

int main(void)
{
  int i, j;
  char pixel = 'A';

  for(i=0;i<4096;i+=64) {
    pixel = 'A';
    for(j=i;j<i+64;j++) {
      imagebuff[j] = (char)pixel++;
    }
    imagebuff[j-1] = '\n';
  }
  imagebuff[4095] = '\0';
  imagebuff[63] = '\0';

  printf("buffer =\n%s", imagebuff);

  /* setup common message q attributes */
  mq_attr.mq_maxmsg = 100;
  mq_attr.mq_msgsize = sizeof(void *)+sizeof(int);

  mq_attr.mq_flags = 0;

  /* Changed the mode to support in linux*/
  mymq = mq_open(SNDRCV_MQ, O_CREAT|O_RDWR, 0664, &mq_attr);

  if(mymq == (mqd_t)ERROR)
    perror("mq_open");

  // Added threads for receive and sender fucntion and set priorities
	
	 pthread_t pid1, pid2;

	 pthread_attr_t pid1attrs, pid2attrs;
	 struct sched_param pid1_sched_param, pid2_sched_param;
	
 	pthread_attr_init(&pid1attrs);
        pthread_attr_init(&pid2attrs);
	
	pid1_sched_param.sched_priority = 99;
        pid2_sched_param.sched_priority = 98;

	pthread_attr_setschedparam(&pid1attrs,&pid1_sched_param);
        pthread_attr_setschedparam(&pid2attrs,&pid2_sched_param);
	 
	pthread_create(&pid1,&pid1attrs,receiver,NULL);
	pthread_create(&pid2,&pid2attrs,sender,NULL);

	pthread_join(pid1, NULL);
	pthread_join(pid2, NULL);

	mq_close(mymq);

	return 0;

}

