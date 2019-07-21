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
#include <sys/time.h>
#include <syslog.h>


#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

#define margin 1.1


typedef struct
{
    int threadIdx;
} threadParams_t;

/* Global variables*/
int HRES,VRES;
int c;
int lowThreshold=0;
int const max_lowThreshold = 100;
int kernel_size = 3;
int edgeThresh = 1;
int ratio = 3;
Mat canny_frame, cdst, timg_gray, timg_grad;


CvCapture* capture;
IplImage* frame;
int dev=0;
vector<Vec3f> circles;
vector<Vec4i> lines;
Mat gray;
static int frame_count = 51;

/* Function: Gives the real time*/
double get_time()
{
	
	struct timeval timeNow;
	double present_time;
	gettimeofday(&timeNow,0);
	present_time = ((double)timeNow.tv_sec+ (double)timeNow.tv_usec/1000000);
	return present_time;

}

/* Transform function for hough_eleptical*/

void hough_eleptical()
{
	vector<Vec3f> circles;
	Mat gray;
	int flag =0;
	int frame_count_each = 0;
	double current_time, previous_time, time_diff_frame,total_time, frame_rate, average_time, deadline = 0;
	
	cvNamedWindow("Capture Example", CV_WINDOW_AUTOSIZE);

	while(frame_count_each <= frame_count)
    	{		
		current_time = get_time();
		frame=cvQueryFrame(capture);
		Mat mat_frame = cvarrToMat(frame);


		if(frame_count_each > 1)
		{			
			time_diff_frame = current_time - previous_time;
			syslog(LOG_CRIT, "\n The time difference is:%lf", time_diff_frame);
			
			total_time += time_diff_frame;						
		}
		
		frame_count_each++;

		cvtColor(mat_frame, gray, CV_BGR2GRAY);
		GaussianBlur(gray, gray, Size(9,9), 2, 2);

		HoughCircles(gray, circles, CV_HOUGH_GRADIENT, 1, gray.rows/8, 100, 50, 0, 0);

		for( size_t i = 0; i < circles.size(); i++ )
		{
		  Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
		  int radius = cvRound(circles[i][2]);
		  // circle center
		  circle( mat_frame, center, 3, Scalar(0,255,0), -1, 8, 0 );
		  // circle outline
		  circle( mat_frame, center, radius, Scalar(0,0,255), 3, 8, 0 );
		}
		
		previous_time = current_time;
	     
		if(!frame) break;

		if(frame_count_each > frame_count)
		{
			frame_rate = (frame_count-1)/total_time;
			average_time = total_time/(frame_count-1);
			deadline = (average_time*margin);

			syslog(LOG_CRIT, "\n The average time is: %lf",average_time);
			syslog(LOG_CRIT, "\n The total time is: %lf",total_time);
			syslog(LOG_CRIT, "\n The frame rate is: %lf",frame_rate);
			syslog(LOG_CRIT,"\n The deadline is: %lf",deadline);
			flag =1;

		}

		cvShowImage("Capture Example", frame);
		char c = cvWaitKey(10);
		if( c == 27 ) break;
    	}
	
	if(flag ==1)
	{
		flag=0;
		frame_count_each = 0;

		int positive_jitter,negative_jitter=0;
		double positive_jitter_time, negative_jitter_time=0;
		while(frame_count_each <= frame_count)
    		{	
			flag =1;		
			current_time = get_time();
			frame=cvQueryFrame(capture);
			Mat mat_frame = cvarrToMat(frame);
			


			if(frame_count_each > 1)
			{				
				time_diff_frame = current_time - previous_time;

				
				if(time_diff_frame>deadline)
				{
					positive_jitter++;
					positive_jitter_time+= (time_diff_frame - deadline);
					syslog(LOG_CRIT, "\n MESSAGE:The deadline for frame %d was missed", frame_count_each);
				}
				else if (deadline > time_diff_frame)
				{
					negative_jitter++;
					negative_jitter_time+= (deadline - time_diff_frame);

				}

				total_time += time_diff_frame;					
			}
			
			frame_count_each++;

			cvtColor(mat_frame, gray, CV_BGR2GRAY);
			GaussianBlur(gray, gray, Size(9,9), 2, 2);

			HoughCircles(gray, circles, CV_HOUGH_GRADIENT, 1, gray.rows/8, 100, 50, 0, 0);


			for( size_t i = 0; i < circles.size(); i++ )
			{
			  Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
			  int radius = cvRound(circles[i][2]);
			  // circle center
			  circle( mat_frame, center, 3, Scalar(0,255,0), -1, 8, 0 );
			  // circle outline
			  circle( mat_frame, center, radius, Scalar(0,0,255), 3, 8, 0 );
			}
			
			previous_time = current_time;
		     
			if(!frame) break;

			if(frame_count_each > frame_count)
			{

				syslog(LOG_CRIT, "\n The total deadlines missed are: %d", positive_jitter);
				syslog(LOG_CRIT, "\n The total positive jitters are: %lf", positive_jitter_time);
				syslog(LOG_CRIT, "\n The total negtive jitters are: %lf ", negative_jitter_time);
			}

			cvShowImage("Capture Example", frame);

			char c = cvWaitKey(10);
			if( c == 27 ) break;
	    	}

	}
}

void CannyThreshold(int, void*)
{
    Mat mat_frame = cvarrToMat(frame);

    cvtColor(mat_frame, timg_gray, CV_RGB2GRAY);

    /// Reduce noise with a kernel 3x3
    blur( timg_gray, canny_frame, Size(3,3) );

    /// Canny detector
    Canny( canny_frame, canny_frame, lowThreshold, lowThreshold*ratio, kernel_size );

    /// Using Canny's output as a mask, we display our result
    timg_grad = Scalar::all(0);

    mat_frame.copyTo( timg_grad, canny_frame);

    imshow( "Capture Example", timg_grad );

}

/* Transform function: canny-interactive*/
void canny_interactive()
{
	int flag =0;
	int frame_count_each = 0;
	double current_time, previous_time, time_diff_frame,total_time, frame_rate, average_time, deadline = 0;
	//cvNamedWindow("Capture Example", CV_WINDOW_AUTOSIZE);
	namedWindow( "Capture Example", CV_WINDOW_AUTOSIZE );
	while(frame_count_each <= frame_count)
	    {
		current_time = get_time();
		frame=cvQueryFrame(capture);
		if(!frame) break;
		if(frame_count_each > 1)
		{			
			time_diff_frame = current_time - previous_time;

			syslog(LOG_CRIT, "\n The time difference is:%lf", time_diff_frame);
			total_time += time_diff_frame;						
		}
		frame_count_each++;
		CannyThreshold(0, 0);
		previous_time = current_time;
		if( frame_count_each > frame_count)
		{
			frame_rate = (frame_count-1)/total_time;
			average_time = total_time/(frame_count-1);
			deadline = (average_time*margin);

			syslog(LOG_CRIT, "\n The average time is: %lf", average_time);
			syslog(LOG_CRIT, "\n The total time is: %lf",total_time);
			syslog(LOG_CRIT, "\n The frame rate is: %lf", frame_rate);
			syslog(LOG_CRIT, "\n The deadline is: %lf", deadline);
			flag =1;

		}

		char q = cvWaitKey(33);
		if( q == 'q' )
		{
		    printf("got quit\n"); 
		    break;
		}
	    }

	if(flag ==1)
	{
		flag=0;
		frame_count_each = 0;
		int positive_jitter =0;
		int negative_jitter=0;
		double positive_jitter_time, negative_jitter_time=0;
		while(frame_count_each <= frame_count)
    		{
			current_time = get_time();
			frame=cvQueryFrame(capture);
			if(!frame) break;
			if(frame_count_each > 1)
			{				
				time_diff_frame = current_time - previous_time;
				
				if(time_diff_frame>deadline)
				{
					positive_jitter++;
					positive_jitter_time+= (time_diff_frame - deadline);
					syslog(LOG_CRIT, "\n MESSAGE:The deadline for frame %d was missed", frame_count_each);
				}
				else if (deadline > time_diff_frame)
				{
					negative_jitter++;
					negative_jitter_time+= (deadline - time_diff_frame);
				}

				total_time += time_diff_frame;					
			}
			frame_count_each++;
			CannyThreshold(0, 0);
			previous_time = current_time;
			if( frame_count_each > frame_count)
			{

				syslog(LOG_CRIT, "\n The total deadlines missed are: %d", positive_jitter);
				syslog(LOG_CRIT, "\n The total positive jitters are: %lf", positive_jitter_time);
				syslog(LOG_CRIT, "\n The total negtive jitters are: %lf ", negative_jitter_time);
			}

			char q = cvWaitKey(33);
			if( q == 'q' )
			{
			    printf("got quit\n"); 
			    break;
			}
		}
	}
}

/*tranform function : hough-interactive*/
void hough_interactive()
{
	int flag =0;
	int frame_count_each = 0;
	double current_time, previous_time, time_diff_frame,total_time, frame_rate, average_time, deadline = 0;

	 while(frame_count_each <= frame_count)
	{
		current_time = get_time();
		frame=cvQueryFrame(capture);

		Mat mat_frame = cvarrToMat(frame);

		if(frame_count_each > 1)
		{			
			time_diff_frame = current_time - previous_time;
			syslog(LOG_CRIT, "\n The time difference is:%lf", time_diff_frame);
			total_time += time_diff_frame;						
		}
		
		frame_count_each++;
		Canny(mat_frame, canny_frame, 50, 200, 3);

		HoughLinesP(canny_frame, lines, 1, CV_PI/180, 50, 50, 10);

		for( size_t i = 0; i < lines.size(); i++ )
		{
		  Vec4i l = lines[i];
		  line(mat_frame, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
		}

		
		if(!frame) break;

		cvShowImage("Capture Example", frame);
		previous_time = current_time;
		if(frame_count_each > frame_count)
		{
			frame_rate = (frame_count-1)/total_time;
			average_time = total_time/(frame_count-1);
			deadline = (average_time*margin);

			syslog(LOG_CRIT, "\n The average time is: %lf", average_time);
			syslog(LOG_CRIT, "\n The total time is: %lf",total_time);
			syslog(LOG_CRIT, "\n The frame rate is: %lf", frame_rate);
			syslog(LOG_CRIT, "\n The deadline is: %lf", deadline);
			flag =1;

		}
		char c = cvWaitKey(10);
		if( c == 27 ) break;
	}

	if(flag ==1)
	{
		flag=0;
		frame_count_each = 0;
		int positive_jitter,negative_jitter=0;
		double positive_jitter_time, negative_jitter_time=0;
		while(frame_count_each <= frame_count)
    		{	
			flag =1;		
			current_time = get_time();
			frame=cvQueryFrame(capture);

			Mat mat_frame = cvarrToMat(frame);

			if(frame_count_each > 1)
			{				
				time_diff_frame = current_time - previous_time;
				
				if(time_diff_frame>deadline)
				{
					positive_jitter++;
					positive_jitter_time+= (time_diff_frame - deadline);
					syslog(LOG_CRIT, "\n MESSAGE:The deadline for frame %d was missed", frame_count_each);
				}
				else if (deadline > time_diff_frame)
				{
					negative_jitter++;
					negative_jitter_time+= (deadline - time_diff_frame);
				}

				total_time += time_diff_frame;					
			}
			
			frame_count_each++;
			Canny(mat_frame, canny_frame, 50, 200, 3);

			HoughLinesP(canny_frame, lines, 1, CV_PI/180, 50, 50, 10);

			for( size_t i = 0; i < lines.size(); i++ )
			{
			  Vec4i l = lines[i];
			  line(mat_frame, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
			}

			
			if(!frame) break;

			cvShowImage("Capture Example", frame);
			previous_time = current_time;
			if( frame_count_each > frame_count)
			{
				syslog(LOG_CRIT, "\n The total deadlines missed are: %d", positive_jitter);
				syslog(LOG_CRIT, "\n The total positive jitters are: %lf", positive_jitter_time);
				syslog(LOG_CRIT, "\n The total negtive jitters are: %lf ", negative_jitter_time);
				
			}
			char c = cvWaitKey(10);
			if( c == 27 ) break;


		}

	}

}

/* Thread */
void *myThread1(void *) 
{ 
	switch (c)
	{
		case 1: canny_interactive();
			break;
		case 2: hough_eleptical();
			break;
		case 3: hough_interactive();
			break;
	}
}


int main( int argc, char** argv )
{

	if(argc < 3)
	{
		printf("\n Enter right number of arguments");
		exit(-1);
	}
	
	else
	{
		printf("\n Right number of arguments passed");
		c = atoi(argv[1]);
		HRES = atoi(argv[2]);
		VRES = atoi(argv[3]);

	
	}

	capture = (CvCapture *)cvCreateCameraCapture(dev);
	cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, HRES);
	cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, VRES);

	pthread_t pid1; 
	pthread_attr_t pid1attrs;
	struct sched_param pid1_sched_param;

	pthread_attr_init(&pid1attrs);

	pthread_attr_setschedpolicy(&pid1attrs,SCHED_FIFO);

	pid1_sched_param.sched_priority = 98;

	pthread_attr_setschedparam(&pid1attrs,&pid1_sched_param);

	pthread_create(&pid1, NULL, myThread1, (void *)0);

	pthread_join(pid1, NULL);

	cvReleaseCapture(&capture);
	cvDestroyWindow("Capture Example");

	return 0;
    
};
