#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

/*DEFINE*/
#define BUF_SIZE 1024
#define INPUT_SIZE 100

#define SQL_SELECT 101
#define REP_CHECK 201
#define REP_ON 202
#define REP_OFF 203
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
void send_packet(int, int, const char*);
void menu();
void clear_input_buffer();
void type_categorizer(Packet, int);

int main()
{
    pthread_t recv_thread_id;
    connect_to_server();
    pthread_create(&recv_thread_id, NULL, recv_thread, (void*)&sock_info.fd);
    menu();
	return 0;
}

void menu() {
	int num;
    char input[INPUT_SIZE];
	while (1) {
        printf("<Select menu>\n");
        printf("1. Select All\n");
        printf("2. show replication status\n");
        printf("3. replication ON\n");
        printf("4. replication OFF\n");
        printf("99. quit\n");

        printf("select number:");
		if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Input error.\n");
            continue;
        }

        // 개행 문자를 제거
        input[strcspn(input, "\n")] = '\0';

        // 입력값을 숫자로 변환, 실패하면 에러 처리
        if (sscanf(input, "%d", &num) != 1) {
            printf("Invalid input. Please enter a number.\n");
            clear_input_buffer();  // 입력 버퍼를 비운다
            continue;
        }

        // 입력 버퍼에 남아 있는 데이터가 있는지 확인하고, 비운다
        if (strlen(input) == sizeof(input) - 1 && input[BUF_SIZE - 2] != '\0') {
            printf("Input too long. Please enter a shorter number.\n");
            clear_input_buffer();
            continue;
        }

        printf("\n");

		switch (num) {
            case 1:
                send_packet(sock_info.fd, SQL_SELECT, "\n");
                sleep(1);
                break;
            case 2:
                send_packet(sock_info.fd, REP_CHECK, "\n");
                sleep(1);
                break;
            case 3:
                send_packet(sock_info.fd, REP_ON, "\n");
                sleep(1);
                break;
            case 4:
                send_packet(sock_info.fd, REP_OFF, "\n");
                sleep(1);
                break;
            case 99:
                return;
		}
		printf(" ------------------------------- \n");

	}
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
        // 입력 버퍼를 비운다.
    }
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
	char message[BUF_SIZE];
	int str_len;
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

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

void type_categorizer(Packet packet, int fd){
	switch(packet.header.type){
		case SQL_SELECT:
			printf("<<<------result----->>>\n%s\n", packet.buf);
            break;
        case REP_CHECK: {
            char *slave_io_running;
            char *slave_sql_running;

            char *token = strtok(packet.buf, " ");

            // Left buf
            if(token != NULL){
                slave_io_running = token;
                token = strtok(NULL, " "); // move next
            }
            // Right buf
            if(token != NULL){
                slave_sql_running = token;
            }

            printf("=======================================\n");
            printf("\tSlave_IO_Running: %s\n", slave_io_running);
            printf("\tSlave_SQL_Running: %s\n", slave_sql_running);
			printf("=======================================\n\n");
            break;
        }
        case REP_ON:
			printf("<<<------result----->>>\n%s\n", packet.buf);
            break;
        case REP_OFF:
			printf("<<<------result----->>>\n%s\n", packet.buf);
            break;
        case EVT_WARNING:
       		printf("[WARNING] %s\n", packet.buf);
            break;
	}
}











	// while(1) 
	// {
	// 	fputs("Input(Q to quit): ", stdout);
	// 	fgets(message, BUF_SIZE, stdin);
		
	// 	if(!strcmp(message,"q\n") || !strcmp(message,"Q\n")){
    //         printf("input Q. exit program\n");
	// 		break;
    //     }

    //     if (message[strlen(message) - 1] != '\n') {
    //         strcat(message, "\n");
    //     }

    //     send_packet(sock, 1, message);
	// }
