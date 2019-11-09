/****************************************************************************/
/* Function: Basic POSIX message queue demo from VxWorks Prog. Guide p. 78  */
/*                                                                          */
/* Sam Siewert - 9/24/97                                                    */
/*                                                                          */
/* Modified the code to support in Linux by adding threads instead          */
/* of tasks                                                                 */
/*                                                                          */
/****************************************************************************/
                                                                    
//#include "msgQLib.h"
//#include "mqueue.h"
//#include "errnoLib.h" 
//#include "ioLib.h" 

#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"
#include <mqueue.h>
#include <time.h>
#include <fcntl.h>           
#include <sys/stat.h>     

// Added / beforeposix_mq ro create the queue in linux
#define SNDRCV_MQ "/posix_mq"
#define MAX_MSG_SIZE 128
#define ERROR -1

struct mq_attr mq_attr;

void *receiver(void)
{
  mqd_t mymq;
  char buffer[MAX_MSG_SIZE];
  int prio;
  int nbytes;

  /* Changed the mode to support in Linux*/
  mymq = mq_open(SNDRCV_MQ, O_CREAT|O_RDWR, 0664, &mq_attr);

  if(mymq == (mqd_t)ERROR)
    perror("mq_open");

  /* read oldest, highest priority msg from the message queue */
  if((nbytes = mq_receive(mymq, buffer, MAX_MSG_SIZE, &prio)) == ERROR)
  {
    perror("mq_receive");
  }
  else
  {
    buffer[nbytes] = '\0';
    printf("receive: msg %s received with priority = %d, length = %d\n",
           buffer, prio, nbytes);
  }
    
}

static char canned_msg[] = "this is a test, and only a test, in the event of a real emergency, you would be instructed ...";

void *sender(void)
{
  mqd_t mymq;
  int prio;
  int nbytes;

  /* Changed the mode to support in Linux*/
  mymq = mq_open(SNDRCV_MQ, O_RDWR, 0664, &mq_attr);

  if(mymq == (mqd_t)ERROR)
    perror("mq_open");

  /* send message with priority=30 */
  if((nbytes = mq_send(mymq, canned_msg, sizeof(canned_msg), 30)) == ERROR)
  {
    perror("mq_send");
  }
  else
  {
    printf("send: message successfully sent\n");
  }
  
}


int main(void)
{

  /* setup common message q attributes */
  mq_attr.mq_maxmsg = 100;
  mq_attr.mq_msgsize = MAX_MSG_SIZE;

  mq_attr.mq_flags = 0;
	
// Added the threads to support the function in Linux  
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
	
	return 0;
}
