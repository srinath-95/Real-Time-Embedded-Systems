/****************************************************************************/
/* Function: Socket Server  */
/*                                                                          */
/* Sam Siewert -                                                    */
/*                                                                          */
/* Modified the code to support in Linux by adding threads instead          */
/* of tasks                                                                 */
/*                                                                          */
/****************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<limits.h>
#include<dirent.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<dirent.h>
#include<sys/stat.h>
#include<syslog.h>
#include <time.h>

#define BUF_SIZE 921713

typedef struct{
	char file_name[70];
	char buffer[BUF_SIZE];
}packet;

packet packet_info;

int client_socket =0;
struct timespec start_socket_time;

int main(int argc, char *argv[]) 
{

	int length = 0;
	int server_socket =0;
	int portno = 0;
	int set =1;
	int reset =0;
	int listen_status = 0;
	int send_status,status =0;
	int recv_status =0;
	int Total_image_size =0;
	int flag = 1;
	FILE *f;
	struct sockaddr_in server_address,client_address;
	memset(&packet_info,0,sizeof(packet_info));
	//char user_validation[100];

	//portno = atoi(argv[1]);
	portno = atoi("5002");
	length = sizeof(client_address);

	server_socket = socket(AF_INET,SOCK_STREAM,0);
        if (server_socket<0)
                printf("\n ERROR:Opening socket");
	else
		printf("\n SUCCESS: Opening socket");

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY); //host to network long
	server_address.sin_port = htons((unsigned short)portno); //host to network short
	
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
		                                          &set, sizeof(set))) 
	{ 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	}
	// binding the socket
        if(bind(server_socket, (struct sockaddr *) &server_address , sizeof(server_address))<0)
                printf("\n ERROR:Binding Failed");
        else
                printf("\n Binding done");
        // listening for requests from client
        listen_status = listen(server_socket,8);

	if(listen_status <0)
		printf("\n Not listening");
	else
		printf("\n Listening");

	int loop_count;
		client_socket = accept(server_socket, (struct sockaddr *) &client_address,(socklen_t*) &length);
		printf("\n Client connection success with Server");
		while (client_socket > 0)
		{
			
			do {
				recv_status = recv(client_socket,&packet_info,sizeof(packet_info),0);
				if(recv_status != 0)
				{
					clock_gettime(CLOCK_REALTIME, &start_socket_time);
					syslog(LOG_CRIT, "Socket_server_start_time @ sec= %f \n", ((double)start_socket_time.tv_sec + (double) ((start_socket_time.tv_nsec)/(double)1000000000)));
					//syslog(LOG_CRIT,"Server: Running in loop number:%d",loop_count);
					//loop_count++;
					Total_image_size +=  recv_status;
					
					//printf("\n The size of the file is: %ld", packet_info.file_size);
					
					if(flag == 1)
					{
						//printf("\n check 1");
						f = fopen(packet_info.file_name,"w+");
						if(f == NULL)
						{
							//printf("\n Error in opening file");
							syslog(LOG_CRIT,"\n Error in opening file");		
						}
						flag = 0;
					
					}
						
					status = fwrite(packet_info.buffer,1,recv_status,f);
					syslog(LOG_CRIT,"\nServer: The status of file copied is: %d",status);		
					memset(packet_info.buffer,0,strlen(packet_info.buffer));		
					
				}
			} while(Total_image_size < BUF_SIZE);
		fclose(f);
		flag = 1;
		Total_image_size =0;
		syslog(LOG_CRIT,"Server: Running in loop number:%d",loop_count);
		loop_count++;
		
		}	
		
	

}
