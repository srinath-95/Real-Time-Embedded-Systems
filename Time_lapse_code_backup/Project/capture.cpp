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
#include <sys/types.h>
#include <sys/socket.h>
#include<netdb.h>
#include<arpa/inet.h>

#include <sys/utsname.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

#define margin 1.1

#define NUM_THREADS (3+1)
#define NUM_CPU_CORES (1)
//#define FRAME_COUNT (200)
#define TRUE (1)
#define FALSE (0)
#define USEC_PER_MSEC (1000)
#define NANOSEC_PER_SEC (1000000000)
#define HRES (640)
#define VRES (480)
#define BUF_SIZE (921713)
#define NUM_OF_BUFFERS 10

//#define HZ
struct utsname unameData;

int socket_enable=0;
static int hz_enable =0;
int FRAME_COUNT=0;
/*socket_parameters*/
int length = 0;
int server_socket =0;
int portno = 0;
int set =0;
int listen_status = 0;
int ret_status;
int fread_status;
long int file_size =0;
int client_socket =0;
FILE *f;
int status =0;

struct sockaddr_in server_address;

	
/* Global variables*/
int c;
int lowThreshold=0;
int const max_lowThreshold = 100;
int kernel_size = 3;
int edgeThresh = 1;
int ratio = 3;
Mat canny_frame, cdst, timg_gray, timg_grad;
char ppm[70];
char image_header[500];


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
struct timespec start_frame_cap_time, end_frame_cap_time;
struct timespec delay_time;

Mat mat_frame;


typedef struct{
	char file_name[70];
	char buffer[BUF_SIZE];
}packet;

packet packet_info;

typedef struct
{
    int threadIdx;
    unsigned long long sequencePeriods;
} threadParams_t;


enum STATUS {
    SUCCESS = 1,
    FAIL = -1,
    BUFF_FULL =-2,
    BUFF_EMPTY =-3
};

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

/*********************************
*********GLOBAL STRUCTURE*********
**********************************/
 struct message_queue
 {
	Mat frame_buf[200];
	int Length;
 	int Ini;
 	int Outi;
	int seq_number;
	int buffer_empty;
 };

struct message_queue queue;

int queue_init()
{
	queue.Length =200;
	queue.Ini =0;
	queue.Outi =0;

}

int add_frame(Mat *frame)
{
	//printf("\n ######## check add_frame");
	queue.frame_buf[queue.Ini] = *frame;
	//queue.frame_buf[queue.Ini].release(); 
	//imwrite("test_add_frame.ppm",queue.frame_buf[queue.Ini]);
	queue.Ini++;
	queue.seq_number++;
	//printf("\n Value of Ini: %d\n",queue.Ini); 
	if(queue.Ini == 199)
	{
		//printf("\n Entered the roll over condition for Ini \n");
		queue.Ini = 0;
	}
	if(queue.Ini == queue.Outi-1)
	{
		//printf("\n value of ini and outi %d", queue.Ini - queue.Outi);
		//printf("\n ######## Buffer is full");
	}
	
}


Mat remove_frame()
{
	//printf("\n######## check remove_frame");
	Mat remove_frame = queue.frame_buf[queue.Outi];
	queue.Outi++;
	queue.buffer_empty = 1;
	//printf("\n Value of Outi: %d\n",queue.Outi); 
	//imwrite("test_remove_frame.ppm",remove_frame);
	if(queue.Outi == 199)
	{
		//printf("\n Entered the roll over condition for Outi \n");
		queue.Outi = 0;
	}
	
	if(queue.Outi == queue.Ini-1)
	{
		//printf("\n ######## Buffer is Empty");
		queue.Outi--;
	}
	
	return remove_frame;
}



void *Service_1(void *threadp)
{
    struct timeval current_time_val, previous_time_val, difference_time_val[FRAME_COUNT];
    double current_time;
    double average_time;
    unsigned long long S1Cnt=0;
    double total_time_sec, total_time_msec =0;
    int count = 0;
    int i = 0;
    threadParams_t *threadParams = (threadParams_t *)threadp;
    
    double diff1[FRAME_COUNT];
    
   
    //gettimeofday(&current_time_val, (struct timezone *)0);
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
        //gettimeofday(&current_time_val, (struct timezone *)0);
	
	clock_gettime(CLOCK_REALTIME, &start_frame_cap_time);
        syslog(LOG_CRIT, "Frame_Capture1 %llu @ sec= %f \n", S1Cnt, ((double)start_frame_cap_time.tv_sec + (double) ((start_frame_cap_time.tv_nsec)/(double)1000000000)));
	//printf( "Frame_Capture start_time %llu @ sec= %f \n", S1Cnt, ((double)start_frame_cap_time.tv_sec + (double) ((start_frame_cap_time.tv_nsec)/(double)1000000000)));
	frame=cvQueryFrame(capture);
	mat_frame = cvarrToMat(frame);
	add_frame(&mat_frame);
	//previous_time_val = current_time_val;
	//gettimeofday(&current_time_val, (struct timezone *)0);
	clock_gettime(CLOCK_REALTIME, &end_frame_cap_time);
	//printf( "Frame_Capture end_time %llu @ sec= %f \n", S1Cnt, ((double)end_frame_cap_time.tv_sec + (double) ((end_frame_cap_time.tv_nsec)/(double)1000000000)));
	if(count >5)
	{
				
		diff1[i] = ((double)end_frame_cap_time.tv_sec + (double) ((end_frame_cap_time.tv_nsec)/(double)1000000000)) - ((double)start_frame_cap_time.tv_sec + (double) ((start_frame_cap_time.tv_nsec)/(double)1000000000));
		//printf("\n The difference in Frame_Capture time is : %f \n", diff1[i]);
		total_time_sec = total_time_sec + diff1[i];
		i++;

		if(count == FRAME_COUNT)
		{
			for(int j=0; j < FRAME_COUNT-5; ++j)
			    {
				if(diff1[0] < diff1[j])
				{
					diff1[0] = diff1[j];
				}

				
			    }
	    
	    		printf("***Frame_Capture WCET @ sec= %f\n",diff1[0]);
			syslog(LOG_CRIT, "***Frame_Capture WCET @ sec= %f\n",diff1[0]);
			printf("***Frame_Capture Total_time sec= %f\n", total_time_sec);
			average_time = ((double)total_time_sec)/(FRAME_COUNT-5);
			printf("***Frame_Capture Average_time %f\n seconds", average_time);
			//syslog(LOG_CRIT, "***Sequencer cycle WCET @ sec= %d, msec=%d\n",(int)difference_time_val[0].tv_sec,(int)difference_time_val[0].tv_usec/USEC_PER_MSEC);
			//syslog(LOG_CRIT, "***Sequencer cycle Total_time sec=%d, msec=%d\n", total_time_sec, total_time_msec);

		}

	}
	sem_post(&semS2);	
    }
    
    cvReleaseCapture(&capture);
    cvDestroyWindow("Capture Example");
    pthread_exit((void *)0);
}


void *Service_2(void *threadp)
{
	//FILE *f = fopen(test.csv, "w"); 
	//if (f == NULL) return -1; 
	Mat frame_dump;
	struct timeval current_time_val, previous_time_val, difference_time_val[FRAME_COUNT], total_time;
	double current_time;
	double average_time;
	double total_time_sec, total_time_msec =0;

	struct timespec start_frame_dump_time, end_frame_dump_time;
	double diff1[FRAME_COUNT];
	unsigned long long S2Cnt=0;
	threadParams_t *threadParams = (threadParams_t *)threadp; 

	int count = 0;
	int i = 0;
	//printf("\n Entered Service_2 \n");
	//gettimeofday(&current_time_val, (struct timezone *)0);
	//syslog(LOG_CRIT, "Frame_Dump @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
	//printf("Frame_Dump @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);

	while(!abortS2)
	{
		sem_wait(&semS2);
		S2Cnt++;
		count++;
		//printf("\n Entered Service_2 \n");
		//gettimeofday(&current_time_val, (struct timezone *)0);
		if(count < FRAME_COUNT)
		{
		clock_gettime(CLOCK_REALTIME, &start_frame_dump_time);
		syslog(LOG_CRIT, "Frame_Dump %llu @ sec= %f \n", S2Cnt, ((double)start_frame_dump_time.tv_sec + (double) ((start_frame_dump_time.tv_nsec)/(double)1000000000)));
		//printf( "Frame_Capture start_time %llu @ sec= %f \n", S2Cnt, ((double)start_frame_dump_time.tv_sec + (double) ((start_frame_dump_time.tv_nsec)/(double)1000000000)));
		//sprintf(ppm,"img%d_%fsec.ppm",count, ((double)end_frame_cap_time.tv_sec + (double) ((end_frame_cap_time.tv_nsec)/(double)1000000000)));
		//syslog(LOG_CRIT, "Frame_Dump %llu @ sec=%d, msec=%d\n", S2Cnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
		sprintf(ppm,"img%d.ppm",count);
		frame_dump = remove_frame();
		/*		
		if(queue.seq_number != S2Cnt)
		{
			queue.Outi--;	
			goto R;
		}*/
		uname(&unameData);
		sprintf(image_header,"%s,%f ",unameData.nodename,((double)end_frame_cap_time.tv_sec + (double) ((end_frame_cap_time.tv_nsec)/(double)1000000000)));
		putText(frame_dump, image_header, Point(5,100), FONT_HERSHEY_PLAIN, 1, Scalar(255,0,0), 2);
		//imshow("frame_dump",frame_dump);
		
		imwrite(ppm,frame_dump);

		clock_gettime(CLOCK_REALTIME, &end_frame_dump_time);
		//printf( "Frame_Capture end_time %llu @ sec= %f \n", S2Cnt, ((double)end_frame_dump_time.tv_sec + (double) ((end_frame_dump_time.tv_nsec)/(double)1000000000)));

		if(count >5)
		{			
			diff1[i] = ((double)end_frame_dump_time.tv_sec + (double) ((end_frame_dump_time.tv_nsec)/(double)1000000000)) - ((double)start_frame_dump_time.tv_sec + (double) ((start_frame_dump_time.tv_nsec)/(double)1000000000));
			//printf("\n The difference in Frame_Capture time is : %f \n", diff1[i]);
			total_time_sec = total_time_sec + diff1[i];
			i++;

			if(count == FRAME_COUNT)
			{
				for(int j=0; j < FRAME_COUNT-5; ++j)
			    	{
					if(diff1[0] < diff1[j])
					{
						diff1[0] = diff1[j];
					}

					
				}
		    
		    		printf("***Frame_Dump WCET @ sec= %f\n",diff1[0]);
				syslog(LOG_CRIT, "***Frame_Dump WCET @ sec= %f\n",diff1[0]);
				printf("***Frame_Dump Total_time sec= %f\n", total_time_sec);
				average_time = ((double)total_time_sec)/(FRAME_COUNT-5);
				printf("***Frame_Dump Average_time %f\n seconds", average_time);
				//syslog(LOG_CRIT, "***Sequencer cycle WCET @ sec= %d, msec=%d\n",(int)difference_time_val[0].tv_sec,(int)difference_time_val[0].tv_usec/USEC_PER_MSEC);
				//syslog(LOG_CRIT, "***Sequencer cycle Total_time sec=%d, msec=%d\n", total_time_sec, total_time_msec);

			}

		}
		}
	if(socket_enable == 1)
	{
		sem_post(&semS3);
	}	
	}


	pthread_exit((void *)0);
}

void *Service_3(void *threadp)
{
	FILE *file_ptr; 

	double current_time;
	double average_time;
	double total_time_sec, total_time_msec =0;

	struct timespec start_socket_time, end_socket_time;
	double diff1[FRAME_COUNT];
	unsigned long long S3Cnt=0;
	threadParams_t *threadParams = (threadParams_t *)threadp; 

	int count = 0;
	int i = 0;
	printf("\n Entered Service_3 \n");
	//gettimeofday(&current_time_val, (struct timezone *)0);
	//syslog(LOG_CRIT, "Frame_Dump @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
	//printf("Frame_Dump @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);

	while(!abortS3)
	{
		sem_wait(&semS3);
		S3Cnt++;
		count++;
		if(count < FRAME_COUNT)
		{
		//printf("\n Entered Service_2 \n");
		//gettimeofday(&current_time_val, (struct timezone *)0);
		clock_gettime(CLOCK_REALTIME, &start_socket_time);
		syslog(LOG_CRIT, "Socket_start_time %llu @ sec= %f \n", S3Cnt, ((double)start_socket_time.tv_sec + (double) ((start_socket_time.tv_nsec)/(double)1000000000)));
		//printf( "Socket_start_time %llu @ sec= %f \n", S3Cnt, ((double)start_socket_time.tv_sec + (double) ((start_socket_time.tv_nsec)/(double)1000000000)));
		/* Insert Socket program*/

R:			f = fopen(ppm,"rb");


			if(f != NULL)
			{

				fseek(f,0,SEEK_END);
				file_size = ftell(f);
				//printf("\n file_size_check: %ld",file_size);		
				fseek(f,0,SEEK_SET);
				strcpy(packet_info.file_name,ppm);
				
				//packet_info.file_size = file_size;

				fread_status = fread(packet_info.buffer,1,BUF_SIZE,f);
				//printf("\n The file read status is: %d", fread_status);
				int size = sizeof(packet_info);
				//printf("\n The size of packet is: %d", size);
				int send_status = send(client_socket,&packet_info,sizeof(packet_info),0);
				if(send_status < 0)
					//printf("ERROR: Sending file");
					syslog(LOG_CRIT,"ERROR: Sending file");
				else
					//printf("\n Image sent successfully");

				fclose(f);


			}
			else
			{
				
				//printf("\n File does not exist:  %s", ppm);
				goto R;
			}
		clock_gettime(CLOCK_REALTIME, &end_socket_time);
		//printf( "Socket_end_time %llu @ sec= %f \n", S3Cnt, ((double)end_socket_time.tv_sec + (double) ((end_socket_time.tv_nsec)/(double)1000000000)));

		}

	}


	pthread_exit((void *)0);
}

void *Sequencer(void *threadp)
{ 
	//for my while loop
	long long int loop=1;
	double average_time;
	//structure for current time in sec and msec
	struct timeval current_time_val,previous_time_val, difference_time_val[FRAME_COUNT];
	
	//#ifdef HZ
		//struct timespec delay_time = {0,99910000};	/*for 10 hz*/
	//#else
		//struct timespec delay_time = {0,999874500};	/*for 1hz*/
	//#endif	
	
	if(hz_enable == 10)
	{
		delay_time = {0,99910000};	/*for 10 hz*/
	}
	else
	{
		delay_time = {0,999874500};	/*for 1hz*/
	}
	int i = 0;
	//timing defined
	struct timespec remaining_time;
	int count = 0;
	double total_time_sec;
	int total_time_msec =0;
	//calling  structure from the above defined for thread ID
	threadParams_t *threadParams = (threadParams_t *)threadp;
	//timestamp parameters
	struct timespec start_seq_time, end_seq_time; 
	double diff1[FRAME_COUNT];
	//for difference of timestamp

	double current_time;
	double residual;
	int rc, delay_cnt=0;
	unsigned long long seqCnt=0;
	double gap1;
	char var;
	printf("\n Enter any character to start the capturing: ");
	var = getchar();


	do
    	{
		delay_cnt=0; residual=0.0;
		
		//gettimeofday(&current_time_val, (struct timezone *)0);
		//syslog(LOG_CRIT, "Sequencer thread prior to delay @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
		rc=nanosleep(&delay_time, &remaining_time);
		clock_gettime(CLOCK_REALTIME, &start_seq_time);
		//printf("\n Sequencer cycle before sem_post %llu @ sec=%f \n", seqCnt, ((double)start_seq_time.tv_sec + (double) ((start_seq_time.tv_nsec)/(double)1000000000)));
		//printf( "Sequencer cycle before sem_post %llu @ sec=%d, msec=%d\n", seqCnt, (int)(current_time_val.tv_sec), (int)(current_time_val.tv_usec)/USEC_PER_MSEC);
		seqCnt++;	
		count++;

		if(delay_cnt > 1) printf("Sequencer looping delay %d\n", delay_cnt);
		
		sem_post(&semS1);
		clock_gettime(CLOCK_REALTIME, &end_seq_time);
		//printf("\n Sequencer cycle before sem_post %llu @ sec=%f \n", seqCnt, ((double)end_seq_time.tv_sec + (double) ((end_seq_time.tv_nsec)/(double)1000000000)));
		
		if(count >5)
		{			
			diff1[i] = ((double)end_seq_time.tv_sec + (double) ((end_seq_time.tv_nsec)/(double)1000000000)) - ((double)start_seq_time.tv_sec + (double) ((start_seq_time.tv_nsec)/(double)1000000000));
			//printf("\n The difference in Sequencer time is : %f \n", diff1[i]);
			total_time_sec = total_time_sec + diff1[i];
			i++;

			if(count == FRAME_COUNT)
			{
				for(int j=0; j < FRAME_COUNT-5; ++j)
				{
					if(diff1[0] < diff1[j])
					{
						diff1[0] = diff1[j];
					}


				}
		    
		    		printf("***Sequencer_cycle WCET @ sec= %f\n",diff1[0]);
				printf("***Sequencer_cycle Total_time sec= %f\n", total_time_sec);
				average_time = ((double)total_time_sec)/(FRAME_COUNT-5);
				printf("***Sequencer_cycle Average_time %f\n seconds", average_time);
				//syslog(LOG_CRIT, "***Sequencer cycle WCET @ sec= %d, msec=%d\n",(int)difference_time_val[0].tv_sec,(int)difference_time_val[0].tv_usec/USEC_PER_MSEC);
				//syslog(LOG_CRIT, "***Sequencer cycle Total_time sec=%d, msec=%d\n", total_time_sec, total_time_msec);

			}

		}

    	} while(!abortTest && count < FRAME_COUNT);
	
	sem_post(&semS1); sem_post(&semS2); sem_post(&semS3);
 	abortS1=TRUE; abortS2=TRUE; abortS3=TRUE;

    	pthread_exit((void *)0);
}


int socket_init()
{
	// creating socket for the client
        client_socket = socket(AF_INET,SOCK_STREAM,0);

        if(client_socket<0)
                printf("\n ERROR:Opening Socket");
	else
		printf("\n SUCCESS:Opening Socket");

	ret_status = connect(client_socket, (struct sockaddr *) &server_address, sizeof(server_address));
	if(ret_status < 0)
		printf("\n ++++++++ ERROR: Connection socket");	
	else
	{	
		printf("\n Connection Successful");
	}
	return 1;
}


int main(int argc, char *argv[])
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
	FRAME_COUNT = atoi(argv[1]);
	hz_enable = atoi(argv[2]);
	socket_enable = atoi(argv[3]);

	printf("Using CPUS=%d from total available.\n", CPU_COUNT(&allcpuset));
	
	if(socket_enable == 1)
	{
		portno = atoi(argv[5]);

		bzero((char*) &server_address,sizeof(server_address));

		// defining the server address
		server_address.sin_family = AF_INET;
		server_address.sin_addr.s_addr = inet_addr(argv[4]);
		server_address.sin_port = htons(portno);

		length = sizeof(struct sockaddr_in);

		socket_init();

	}
	
	// initialize the sequencer semaphores
	//
	if (sem_init (&semS1, 0, 0)) { printf ("Failed to initialize S1 semaphore\n"); exit (-1); }
	if (sem_init (&semS2, 0, 0)) { printf ("Failed to initialize S2 semaphore\n"); exit (-1); }
	if (sem_init (&semS3, 0, 0)) { printf ("Failed to initialize S3 semaphore\n"); exit (-1); }
	/*if (sem_init (&semS4, 0, 0)) { printf ("Failed to initialize S4 semaphore\n"); exit (-1); }
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
		if(i ==0 || i==1)
			CPU_SET(3, &threadcpu);
		else 
			CPU_SET(2, &threadcpu);

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

	rt_param[2].sched_priority=rt_min_prio+1;
	pthread_attr_setschedparam(&rt_sched_attr[2], &rt_param[2]);
	rc=pthread_create(&threads[2], &rt_sched_attr[2], Service_2, (void *)&(threadParams[2]));
	if(rc < 0)
		perror("pthread_create for service 2");
	else
		printf("pthread_create successful for service 2\n");
	

	// Service_3 = RT_MAX-3	@ 0.5 Hz
	//
	rt_param[3].sched_priority=rt_min_prio;
	pthread_attr_setschedparam(&rt_sched_attr[3], &rt_param[3]);
	rc=pthread_create(&threads[3], &rt_sched_attr[3], Service_3, (void *)&(threadParams[3]));
	if(rc < 0)
		perror("pthread_create for service 3");
	else
		printf("pthread_create successful for service 3\n");

	// Create Sequencer thread, which like a cyclic executive, is highest prio
	printf("Start sequencer\n");
	threadParams[0].sequencePeriods=999;

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
