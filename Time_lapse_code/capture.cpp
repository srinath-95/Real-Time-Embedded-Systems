/*
 *
 *  Author: Srinath S
 *  Rference: Code from Dr. Sam Seiwert
 */

/************
* Includes  *
*************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <syslog.h>
#include <sched.h>
#include <sched.h>
#include <time.h>
#include <sys/sysinfo.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

#define margin 1.1

#define NUM_THREADS (3)
#define NUM_CPU_CORES (1)
#define FRAME_COUNT (10)
#define TRUE (1)
#define FALSE (0)
#define USEC_PER_MSEC (1000)
#define NANOSEC_PER_SEC (1000000000)
#define HRES (640)
#define VRES (480)
//#define HZ


	
/* Global variables*/
int c;
int lowThreshold=0;
int const max_lowThreshold = 100;
int kernel_size = 3;
int edgeThresh = 1;
int ratio = 3;
Mat canny_frame, cdst, timg_gray, timg_grad;
char ppm[70];


CvCapture* capture;
IplImage* frame;
int dev=0;
vector<Vec3f> circles;
vector<Vec4i> lines;
Mat gray;
static int frame_count = 51;


/*New Set of variables*/

int abortTest=FALSE;
int abortS1=FALSE, abortS2=FALSE, abortS3=FALSE, abortS4=FALSE, abortS5=FALSE, abortS6=FALSE, abortS7=FALSE;
sem_t semS1, semS2, semS3, semS4, semS5, semS6, semS7;
struct timeval start_time_val;

Mat mat_frame;



typedef struct
{
    int threadIdx;
    unsigned long long sequencePeriods;
} threadParams_t;


/* Function: Gives the real time*/
double get_time()
{
	
	struct timeval timeNow;
	double present_time;
	gettimeofday(&timeNow,0);
	present_time = ((double)timeNow.tv_sec+ (double)timeNow.tv_usec/1000000);
	return present_time;

}


void print_scheduler(void)
{
   int schedType;

   schedType = sched_getscheduler(getpid());

   switch(schedType)
   {
       case SCHED_FIFO:
           printf("Pthread Policy is SCHED_FIFO\n");
           break;
       case SCHED_OTHER:
           printf("Pthread Policy is SCHED_OTHER\n"); exit(-1);
         break;
       case SCHED_RR:
           printf("Pthread Policy is SCHED_RR\n"); exit(-1);
           break;
       default:
           printf("Pthread Policy is UNKNOWN\n"); exit(-1);
   }
}


void *Service_1(void *threadp)
{
    struct timeval current_time_val, previous_time_val, difference_time_val[FRAME_COUNT];
    double current_time;
    double average_time;
    unsigned long long S1Cnt=0;
    int total_time_sec, total_time_msec =0;
    int count = 0;
    int i = 0;
    threadParams_t *threadParams = (threadParams_t *)threadp;
    
   
    gettimeofday(&current_time_val, (struct timezone *)0);
    //syslog(LOG_CRIT, "Frame_Capture @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
   //printf("Frame_Capture @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
 
    capture = (CvCapture *)cvCreateCameraCapture(0);
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, HRES);
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, VRES);

    cvNamedWindow("Capture Window",CV_WINDOW_AUTOSIZE);

    while(!abortS1)
    {
        sem_wait(&semS1);
        S1Cnt++;
	count++;
        gettimeofday(&current_time_val, (struct timezone *)0);
        //syslog(LOG_CRIT, "Frame_Capture %llu @ sec=%d, msec=%d\n", S1Cnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
	//printf( "Frame_Capture %llu @ sec=%d, msec=%d\n", S1Cnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
	frame=cvQueryFrame(capture);
	mat_frame = cvarrToMat(frame);
	
	previous_time_val = current_time_val;
	gettimeofday(&current_time_val, (struct timezone *)0);
	if(count>3)
	{
		difference_time_val[i].tv_sec = current_time_val.tv_sec - previous_time_val.tv_sec;
		difference_time_val[i].tv_usec = current_time_val.tv_usec - previous_time_val.tv_usec;
		total_time_msec = total_time_msec+(int)difference_time_val[i].tv_usec/USEC_PER_MSEC;
		//printf("***total_time_msec sec=%d\n",total_time_msec);
		if(total_time_msec > 999)
		{
			total_time_sec++;
			total_time_msec = total_time_msec - 1000;
		}
		//printf( "Frame_Capture Execution time%llu @ sec=%d, msec=%d\n", S1Cnt, (int)(difference_time_val[i].tv_sec), (int)difference_time_val[i].tv_usec/USEC_PER_MSEC);
		syslog(LOG_CRIT, "Frame_Capture Execution time%llu @ sec=%d, msec=%d\n", S1Cnt, (int)(difference_time_val[i].tv_sec), (int)difference_time_val[i].tv_usec/USEC_PER_MSEC);

		i++;
	}
	
	if(count == FRAME_COUNT)
	{
		for(int j=0; j < FRAME_COUNT-3; ++j)
		    {
			if(difference_time_val[0].tv_usec < difference_time_val[j].tv_usec)
			{
				difference_time_val[0].tv_usec = difference_time_val[j].tv_usec;
			}
			
		    }
    
    		printf("***Frame_Capture WCET @ sec= 0, msec=%d\n",(int)difference_time_val[0].tv_usec/USEC_PER_MSEC);
		printf("***Frame_Capture Total_time sec=%d, msec=%d\n", total_time_sec, total_time_msec);
		
		average_time = ((double)total_time_sec+((double)total_time_msec/1000))/(FRAME_COUNT-3);
		printf("***Frame_Capture Average_time %f\n seconds", average_time);

	}
	sem_post(&semS2);	
    }
    
    cvReleaseCapture(&capture);
    cvDestroyWindow("Capture Example");
    pthread_exit((void *)0);
}


void *Service_2(void *threadp)
{
    struct timeval current_time_val, previous_time_val, difference_time_val[FRAME_COUNT], total_time;
    double current_time;
    double average_time;
    int total_time_sec, total_time_msec =0;

    unsigned long long S2Cnt=0;
    threadParams_t *threadParams = (threadParams_t *)threadp; 
    
    int count = 0;
    int i = 0;
    printf("\n Entered Service_2 \n");
    gettimeofday(&current_time_val, (struct timezone *)0);
    //syslog(LOG_CRIT, "Frame_Dump @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
    //printf("Frame_Dump @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);

    while(!abortS2)
    {
        sem_wait(&semS2);
        S2Cnt++;
	count++;
	//printf("\n Entered Service_2 \n");
	gettimeofday(&current_time_val, (struct timezone *)0);
	//sprintf(ppm,"img%d_%dsec_%dmsec.ppm",count, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
	//syslog(LOG_CRIT, "Frame_Dump %llu @ sec=%d, msec=%d\n", S2Cnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
	sprintf(ppm,"img%d.ppm",count);
	imwrite(ppm,mat_frame);

	previous_time_val = current_time_val;
        gettimeofday(&current_time_val, (struct timezone *)0);
	//printf( "Frame_Dump %llu @ sec=%d, msec=%d\n", S2Cnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
        //syslog(LOG_CRIT, "Frame_Dump %llu @ sec=%d, msec=%d\n", S2Cnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
	if(count>1)
	{
		difference_time_val[i].tv_sec = current_time_val.tv_sec - previous_time_val.tv_sec;
		difference_time_val[i].tv_usec = current_time_val.tv_usec - previous_time_val.tv_usec;
		total_time_msec = total_time_msec+(int)difference_time_val[i].tv_usec/USEC_PER_MSEC;
		printf("***total_time_msec sec=%d\n",total_time_msec);
		if(total_time_msec >999)
		{
			total_time_sec++;
			total_time_msec = total_time_msec - 1000;
		}
		printf( "Frame_Dump Execution time%llu @ sec=%d, msec=%d\n", S2Cnt, (int)(difference_time_val[i].tv_sec), (int)difference_time_val[i].tv_usec/USEC_PER_MSEC);
		syslog(LOG_CRIT, "Frame_Dump Execution time%llu @ sec=%d, msec=%d\n", S2Cnt, (int)(difference_time_val[i].tv_sec), (int)difference_time_val[i].tv_usec/USEC_PER_MSEC);
		i++;
	}
	
	if(count == FRAME_COUNT)
	{
		for(int j=0; j < FRAME_COUNT-1; ++j)
		    {
			if(difference_time_val[0].tv_usec < difference_time_val[j].tv_usec)
			{
				difference_time_val[0].tv_usec = difference_time_val[j].tv_usec;
			}
			
		    }
    
    		printf("***Frame_Dump WCET @ sec= 0, msec=%d\n",(int)difference_time_val[0].tv_usec/USEC_PER_MSEC);
		printf("***Frame_Dump Total_time sec=%d, msec=%d\n", total_time_sec, total_time_msec);
		average_time = ((double)total_time_sec+((double)total_time_msec/1000))/(FRAME_COUNT-1);
		printf("***Frame_Dump Average_time %f\n seconds", average_time);		
		syslog(LOG_CRIT, "***Frame_Dump WCET @ sec= 0, msec=%d\n",(int)difference_time_val[0].tv_usec/USEC_PER_MSEC);
		syslog(LOG_CRIT, "***Frame_Dump Total_time sec=%d, msec=%d\n", total_time_sec, total_time_msec);
		

	}
	
    }
    
    
    pthread_exit((void *)0);
}


void *Sequencer(void *threadp)
{ 
	//for my while loop
	long long int loop=1;
	//structure for current time in sec and msec
	struct timeval current_time_val,previous_time_val, difference_time_val[FRAME_COUNT];

	#ifdef HZ
		struct timespec delay_time = {0,99910000};	/*for 10 hz*/
	#else
		struct timespec delay_time = {0,999874500};	/*for 1hz*/
	#endif	
	// struct timespec delay_time = {0,999874500};	/*for 1hz*/
	int i = 0;
	//timimng defined
	struct timespec remaining_time;
	int count = 0;
	int total_time_sec =1;
	int total_time_msec =0;
	//calling  structure from the above defined for thread ID
	threadParams_t *threadParams = (threadParams_t *)threadp;
	//timestamp parameters
	struct timespec start1, end1;
	//for difference of timestamp

	double current_time;
	double residual;
	int rc, delay_cnt=0;
	unsigned long long seqCnt=0;
	double gap1;

	do
    	{
		delay_cnt=0; residual=0.0;
		
		//gettimeofday(&current_time_val, (struct timezone *)0);
		//syslog(LOG_CRIT, "Sequencer thread prior to delay @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
		rc=nanosleep(&delay_time, &remaining_time);
		gettimeofday(&current_time_val, (struct timezone *)0);
		syslog(LOG_CRIT, "Sequencer cycle %llu @ sec=%d, msec=%d\n", seqCnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);

		//ddprintf( "Sequencer cycle before sem_post %llu @ sec=%d, msec=%d\n", seqCnt, (int)(current_time_val.tv_sec), (int)(current_time_val.tv_usec)/USEC_PER_MSEC);
		seqCnt++;	
		count++;

		if(delay_cnt > 1) printf("Sequencer looping delay %d\n", delay_cnt);
		
		sem_post(&semS1);
		/*
		previous_time_val = current_time_val;
		gettimeofday(&current_time_val, (struct timezone *)0);
		printf( "Sequencer cycle after sem_post %llu @ sec=%d, msec=%d\n", seqCnt, (int)(current_time_val.tv_sec), (int)(current_time_val.tv_usec)/USEC_PER_MSEC);
		printf( "Sequencer cycle %llu @ sec=%d, msec=%d\n", seqCnt, (int)(current_time_val.tv_sec-previous_time_val.tv_sec), (int)(current_time_val.tv_usec - previous_time_val.tv_usec)/USEC_PER_MSEC);
		//syslog(LOG_CRIT, "Frame_Dump %llu @ sec=%d, msec=%d\n", S2Cnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
		if(count>1)
		{
			difference_time_val[i].tv_sec = current_time_val.tv_sec - previous_time_val.tv_sec;
			difference_time_val[i].tv_usec = current_time_val.tv_usec - previous_time_val.tv_usec;
			printf( "Sequencer cycle Execution time%llu @ sec=%d, msec=%d\n", seqCnt, (int)(difference_time_val[i].tv_sec), (int)difference_time_val[i].tv_usec/USEC_PER_MSEC);
			total_time_msec = total_time_msec+(int)difference_time_val[i].tv_usec/USEC_PER_MSEC;
			total_time_sec = total_time_sec+(int)difference_time_val[i].tv_sec;
			printf("***total_time_msec sec=%d\n",total_time_msec);
			printf("***total_time_sec sec=%d\n",total_time_sec);
			if(total_time_msec >999)
			{
				total_time_sec++;
				total_time_msec = total_time_msec - 1000;
			}
			
			syslog(LOG_CRIT, "Sequencer cycle Execution time%llu @ sec=%d, msec=%d\n", seqCnt, (int)(difference_time_val[i].tv_sec), (int)difference_time_val[i].tv_usec/USEC_PER_MSEC);
			i++;
		}
		
		if(count == FRAME_COUNT)
		{
			for(int j=0; j < FRAME_COUNT-1; ++j)
			    {
				printf("\n check 1\n");
				if(difference_time_val[0].tv_usec < difference_time_val[j].tv_usec)
				{
					difference_time_val[0].tv_usec = difference_time_val[j].tv_usec;
				}

				if(difference_time_val[0].tv_sec < difference_time_val[j].tv_sec)
				{
					difference_time_val[0].tv_sec = difference_time_val[j].tv_sec;
					
				}
				
			    }
	    
	    		printf("***Sequencer cycle WCET @ sec= 0, msec=%d\n",(int)difference_time_val[0].tv_usec/USEC_PER_MSEC);
			printf("***Sequencer cycle Total_time sec=%d, msec=%d\n", total_time_sec, total_time_msec);
			syslog(LOG_CRIT, "***Sequencer cycle WCET @ sec= %d, msec=%d\n",(int)difference_time_val[0].tv_sec,(int)difference_time_val[0].tv_usec/USEC_PER_MSEC);
			syslog(LOG_CRIT, "***Sequencer cycle Total_time sec=%d, msec=%d\n", total_time_sec, total_time_msec);

		}*/

    	} while(!abortTest && count < FRAME_COUNT);
	
    pthread_exit((void *)0);
}

int main(void)
{
    struct timeval current_time_val;
    int i, rc, scope;
    cpu_set_t threadcpu;
    pthread_t threads[NUM_THREADS];
    threadParams_t threadParams[NUM_THREADS];
    pthread_attr_t rt_sched_attr[NUM_THREADS];
    int rt_max_prio, rt_min_prio;
    struct sched_param rt_param[NUM_THREADS];
    struct sched_param main_param;
    pthread_attr_t main_attr;
    pid_t mainpid;
    cpu_set_t allcpuset;

    printf("Starting Sequencer Demo\n");
    gettimeofday(&start_time_val, (struct timezone *)0);
    gettimeofday(&current_time_val, (struct timezone *)0);
    syslog(LOG_CRIT, "Sequencer @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);

   printf("System has %d processors configured and %d available.\n", get_nprocs_conf(), get_nprocs());

   CPU_ZERO(&allcpuset);
   CPU_SET(i, &allcpuset);

   printf("Using CPUS=%d from total available.\n", CPU_COUNT(&allcpuset));


    // initialize the sequencer semaphores
    //
    if (sem_init (&semS1, 0, 0)) { printf ("Failed to initialize S1 semaphore\n"); exit (-1); }
    if (sem_init (&semS2, 0, 0)) { printf ("Failed to initialize S2 semaphore\n"); exit (-1); }
    /*if (sem_init (&semS3, 0, 0)) { printf ("Failed to initialize S3 semaphore\n"); exit (-1); }
    if (sem_init (&semS4, 0, 0)) { printf ("Failed to initialize S4 semaphore\n"); exit (-1); }
    if (sem_init (&semS5, 0, 0)) { printf ("Failed to initialize S5 semaphore\n"); exit (-1); }
    if (sem_init (&semS6, 0, 0)) { printf ("Failed to initialize S6 semaphore\n"); exit (-1); }
    if (sem_init (&semS7, 0, 0)) { printf ("Failed to initialize S7 semaphore\n"); exit (-1); }*/

    mainpid=getpid();

    rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    rt_min_prio = sched_get_priority_min(SCHED_FIFO);

    rc=sched_getparam(mainpid, &main_param);
    main_param.sched_priority=rt_max_prio;
    rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);
    if(rc < 0) perror("main_param");
    print_scheduler();

    pthread_attr_getscope(&main_attr, &scope);

    if(scope == PTHREAD_SCOPE_SYSTEM)
      printf("PTHREAD SCOPE SYSTEM\n");
    else if (scope == PTHREAD_SCOPE_PROCESS)
      printf("PTHREAD SCOPE PROCESS\n");
    else
      printf("PTHREAD SCOPE UNKNOWN\n");

    printf("rt_max_prio=%d\n", rt_max_prio);
    printf("rt_min_prio=%d\n", rt_min_prio);

    for(i=0; i < NUM_THREADS; i++)
    {

      CPU_ZERO(&threadcpu);
      CPU_SET(3, &threadcpu);

      rc=pthread_attr_init(&rt_sched_attr[i]);
      rc=pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
      rc=pthread_attr_setschedpolicy(&rt_sched_attr[i], SCHED_FIFO);
      rc=pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &threadcpu);

      rt_param[i].sched_priority=rt_max_prio-i;
      pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);

      threadParams[i].threadIdx=i;
    }
   
    printf("Service threads will run on %d CPU cores\n", CPU_COUNT(&threadcpu));

    // Create Service threads which will block awaiting release for:
    //

    // Servcie_1 = RT_MAX-1	@ 3 Hz
    //
    rt_param[1].sched_priority=rt_max_prio-1;
    pthread_attr_setschedparam(&rt_sched_attr[1], &rt_param[1]);
    rc=pthread_create(&threads[1],               // pointer to thread descriptor
                      &rt_sched_attr[1],         // use specific attributes
                      //(void *)0,               // default attributes
                      Service_1,                 // thread function entry point
                      (void *)&(threadParams[1]) // parameters to pass in
                     );
    if(rc < 0)
        perror("pthread_create for service 1");
    else
        printf("pthread_create successful for service 1\n");


    // Service_2 = RT_MAX-2	@ 1 Hz
    // 
    
    rt_param[2].sched_priority=rt_max_prio-2;
    pthread_attr_setschedparam(&rt_sched_attr[2], &rt_param[2]);
    rc=pthread_create(&threads[2], &rt_sched_attr[2], Service_2, (void *)&(threadParams[2]));
    if(rc < 0)
        perror("pthread_create for service 2");
    else
        printf("pthread_create successful for service 2\n");

    /*
    // Service_3 = RT_MAX-3	@ 0.5 Hz
    //
    rt_param[3].sched_priority=rt_max_prio-3;
    pthread_attr_setschedparam(&rt_sched_attr[3], &rt_param[3]);
    rc=pthread_create(&threads[3], &rt_sched_attr[3], Service_3, (void *)&(threadParams[3]));
    if(rc < 0)
        perror("pthread_create for service 3");
    else
        printf("pthread_create successful for service 3\n");


    // Service_4 = RT_MAX-2	@ 1 Hz
    //
    rt_param[4].sched_priority=rt_max_prio-3;
    pthread_attr_setschedparam(&rt_sched_attr[4], &rt_param[4]);
    rc=pthread_create(&threads[4], &rt_sched_attr[4], Service_4, (void *)&(threadParams[4]));
    if(rc < 0)
        perror("pthread_create for service 4");
    else
        printf("pthread_create successful for service 4\n");


    // Service_5 = RT_MAX-3	@ 0.5 Hz
    //
    rt_param[5].sched_priority=rt_max_prio-3;
    pthread_attr_setschedparam(&rt_sched_attr[5], &rt_param[5]);
    rc=pthread_create(&threads[5], &rt_sched_attr[5], Service_5, (void *)&(threadParams[5]));
    if(rc < 0)
        perror("pthread_create for service 5");
    else
        printf("pthread_create successful for service 5\n");


    // Service_6 = RT_MAX-2	@ 1 Hz
    //
    rt_param[6].sched_priority=rt_max_prio-2;
    pthread_attr_setschedparam(&rt_sched_attr[6], &rt_param[6]);
    rc=pthread_create(&threads[6], &rt_sched_attr[6], Service_6, (void *)&(threadParams[6]));
    if(rc < 0)
        perror("pthread_create for service 6");
    else
        printf("pthread_create successful for service 6\n");


    // Service_7 = RT_MIN	0.1 Hz
    //
    rt_param[7].sched_priority=rt_min_prio;
    pthread_attr_setschedparam(&rt_sched_attr[7], &rt_param[7]);
    rc=pthread_create(&threads[7], &rt_sched_attr[7], Service_7, (void *)&(threadParams[7]));
    if(rc < 0)
        perror("pthread_create for service 7");
    else
        printf("pthread_create successful for service 7\n");

    */
    // Wait for service threads to initialize and await relese by sequencer.
    //
    // Note that the sleep is not necessary of RT service threads are created wtih 
    // correct POSIX SCHED_FIFO priorities compared to non-RT priority of this main
    // program.
    //
    // usleep(1000000);
 
    // Create Sequencer thread, which like a cyclic executive, is highest prio
    printf("Start sequencer\n");
    threadParams[0].sequencePeriods=900;

    // Sequencer = RT_MAX	@ 30 Hz
    //
    rt_param[0].sched_priority=rt_max_prio;
    pthread_attr_setschedparam(&rt_sched_attr[0], &rt_param[0]);
    rc=pthread_create(&threads[0], &rt_sched_attr[0], Sequencer, (void *)&(threadParams[0]));
    if(rc < 0)
        perror("pthread_create for sequencer service 0");
    else
        printf("pthread_create successful for sequeencer service 0\n");
   
   for(int i = 0; i < NUM_THREADS; i++)
   {

       CPU_ZERO(&threadcpu);
       if(pthread_getaffinity_np(threads[i], sizeof(cpu_set_t),
       &threadcpu) == 0)
       {
           if(CPU_ISSET(0, &threadcpu) != 0)
           printf("Thread %d's Affinity: CPU 0\n", i);
           else if(CPU_ISSET(1, &threadcpu) != 0)
           printf("Thread %d's Affinity: CPU 1\n",i);
           else if(CPU_ISSET(2, &threadcpu) != 0)
           printf("Thread %d's Affinity: CPU 2\n",i);
           else if(CPU_ISSET(3, &threadcpu) != 0)
           printf("Thread %d's Affinity: CPU 3\n",i);
           else if(CPU_ISSET(4, &threadcpu) != 0)
           printf("Thread %d's Affinity: CPU 2\n",i);
           else
           printf("No affinity set for thread %d\n",i);
       }
   }


   for(i=0;i<NUM_THREADS;i++)
       pthread_join(threads[i], NULL);

   printf("\nTEST COMPLETE\n");
}
