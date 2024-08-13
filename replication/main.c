#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>

/*DEFINE*/
#define BUF_SIZE 1024

#define REP_CHECK 201
#define EVT_WARNING 400
#define EVT_ERROR 401

/*STRUCT*/
typedef struct _socket_info{
    int fd;
} SOCKET_INFO;

typedef struct _header{
    int length;
    int type;
} Header;

typedef struct _packet{
    Header header;
    char buf[BUF_SIZE];
} Packet;

/*GLOBAL*/
SOCKET_INFO sock_info;

/*FUNCTION*/
void error_handling(char *message);
int connect_to_server();
void* recv_thread(void*);
void* send_thread(void*);
void send_packet(int, int, const char*);
void type_categorizer(Packet, int);


int main()
{
    pthread_t recv_thread_id, send_thread_id;
    connect_to_server();
    
    /* create thread */
    pthread_create(&recv_thread_id, NULL, recv_thread, (void*)&sock_info.fd); // recive
    pthread_create(&send_thread_id, NULL, send_thread, (void*)&sock_info.fd); // send
    
    pthread_join(recv_thread_id, NULL);
    pthread_join(send_thread_id, NULL);

	return 0;
}

void send_packet(int sock, int msg_type, const char* message){
    Packet msg;

    msg.header.type = msg_type;
    strncpy(msg.buf, message, BUF_SIZE -1);
    msg.buf[BUF_SIZE - 1] = '\0';

    msg.header.length = strlen(msg.buf);

    send(sock, &msg, sizeof(msg.header) + msg.header.length, 0);
}

int connect_to_server()
{
    int sock;
	struct sockaddr_in serv_adr;

	sock=socket(PF_INET, SOCK_STREAM, 0);   
	if(sock==-1)
		error_handling("socket() error");
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=inet_addr("10.0.2.4");
	serv_adr.sin_port=htons(9999);

	if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		error_handling("connect() error!");
	else
		puts("Connected");
    
    sock_info.fd = sock;
	
}

void *recv_thread(void *arg) {
    int sock = *((int *)arg);
    Packet recv_msg;

    while (1) {
        int total_received = 0;
        int remaining_data;
        
        // 먼저 헤더를 수신
        int str_len = recv(sock, &recv_msg.header, sizeof(recv_msg.header), 0);
        if (str_len <= 0) {
            break;  // 서버가 연결을 종료하거나 에러가 발생한 경우
        }

        remaining_data = recv_msg.header.length;
        
        // 메시지가 큰 경우 여러 번 recv 호출
        while (remaining_data > 0) {
            str_len = recv(sock, recv_msg.buf + total_received, remaining_data, 0);
            if (str_len <= 0) {
                break;  // 서버가 연결을 종료하거나 에러가 발생한 경우
            }
            total_received += str_len;
            remaining_data -= str_len;
        }

        recv_msg.buf[total_received] = '\0';
        printf("recv msg info\n len: %d, type: %d\n", recv_msg.header.length, recv_msg.header.type);
	    type_categorizer(recv_msg, sock);
    }

    return NULL;
}

void *send_thread(void *arg){
    int sock = *((int *)arg);
    while(1){
        send_packet(sock_info.fd, REP_CHECK, "\n");
        sleep(5);
    }
    return NULL;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

void type_categorizer(Packet packet, int fd){
	if(packet.header.type == REP_CHECK){
        char *slave_io_running;
        char *slave_sql_running;

        char *Last_IO_Errno;
        char *Last_IO_Error;
        char *Last_SQL_Errno;
        char *Last_SQL_Error;

        char *token = strtok(packet.buf, ",");

        // Left buf
        if(token != NULL){
            slave_io_running = token;
            token = strtok(NULL, ","); // move next
        }
        // Right buf
        if(token != NULL){
            slave_sql_running = token;
            token = strtok(NULL, ","); // move next
        }

        // Right buf
        if(token != NULL){
            Last_IO_Errno = token;
            token = strtok(NULL, ","); // move next
        }

        if(token != NULL){
            Last_IO_Error = token;
            token = strtok(NULL, ","); // move next
        }

        if(token != NULL){
            Last_SQL_Errno = token;
            token = strtok(NULL, ","); // move next
        }

        if(token != NULL){
            Last_SQL_Error = token;
        }
        system("clear"); // screen clear

        printf("=======================================\n");
        printf("\tSlave_IO_Running: %s\n", slave_io_running);
        printf("\tSlave_SQL_Running: %s\n", slave_sql_running);
        printf("\tLast_IO_Errno: %s\n", Last_IO_Errno);
        printf("\tLast_IO_Error: %s\n", Last_IO_Error);
        printf("\tLast_SQL_Errno: %s\n", Last_SQL_Errno);
        printf("\tLast_SQL_Error: %s\n", Last_SQL_Error);
        printf("=======================================\n\n");

        fflush(stdout); // buffer
    }
}