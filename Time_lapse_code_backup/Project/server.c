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

typedef struct{
	char command_choice[8];
	char filename[20];
	char username[50];
	int pieces;
	int packetcount;
	long int file_piece_length;
	char subfolder[20];
}packet;

packet packet_info;

int main(int argc, char *argv[]) 
{

	int length = 0;
	int server_socket =0;
	int portno = 0;
	int listen_status = 0;
	
	struct sockaddr_in server_address,client_address;
	memset(&packet_info,0,sizeof(packet_info));
	//char user_validation[100];

	portno = atoi(argv[2]);
	length = sizeof(client_address);

	server_socket = socket(AF_INET,SOCK_STREAM,0);
        if (server_socket<0)
                printf("\n ERROR:Opening socket");

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY); //host to network long
	server_address.sin_port = htons((unsigned short)portno); //host to network short

	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set));
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

	while(1)
	{
		client_socket = accept(server_socket, (struct sockaddr *) &client_address,(socklen_t*) &length);
		while(client_socket > 0)
		{
			printf("\n Client connection success with Server");
		}	
	}

}
