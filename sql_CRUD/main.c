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
#define INPUT_SIZE 100

#define SQL_SHOW_TB 103
#define SQL_SHOW_TB_LIST 104
#define SQL_SHOW_TB_DEL 105
#define SQL_SHOW_TB_INT 106
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
void clear_input_buffer();

int main()
{
    pthread_t recv_thread_id, send_thread_id;
    connect_to_server();
        
    /* create thread */
    pthread_create(&recv_thread_id, NULL, recv_thread, (void*)&sock_info.fd); // recive
        
    char send_buffer[1024];
	while (1) {
        int num;
        char input[INPUT_SIZE];

        printf("<Select menu>\n");
        printf("1. All SHOW TABLE\n");
        printf("2. TABLE SHOW LIST\n");
        printf("3. Delete data\n");
        printf("4. Insert data\n");
        printf("99. quit\n");
        printf("select number: ");
        
		if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Input error.\n");
            continue;
        }

        input[strcspn(input, "\n")] = '\0';

        if (sscanf(input, "%d", &num) != 1) {
            printf("Invalid input. Please enter a number.\n");
            clear_input_buffer();
            continue;
        }

        if (strlen(input) == sizeof(input) - 1 && input[BUF_SIZE - 2] != '\0') {
            printf("Input too long. Please enter a shorter number.\n");
            clear_input_buffer();
            continue;
        }

		switch (num) {
            char input_tbname[254];
            char input_column[254];
            char input_coldata[254];
            char input_deldata1[254];
            char input_deldata2[254];

            case 1:
                snprintf(send_buffer, sizeof(send_buffer), "");
                send_packet(sock_info.fd, SQL_SHOW_TB, send_buffer);
                sleep(1);
                break;
            case 2:
                printf("Input table name: ");
                scanf("%s", &input_tbname);
                snprintf(send_buffer, sizeof(send_buffer), input_tbname);
                send_packet(sock_info.fd, SQL_SHOW_TB_LIST, send_buffer);
                sleep(1);
                clear_input_buffer();
                break;
            case 3:
                printf("Delete table name: ");
                scanf("%s", &input_tbname);
                printf("Column name: ");
                scanf("%s", &input_column);
                printf("Column data: ");
                scanf("%s", &input_coldata);
                
                snprintf(send_buffer, sizeof(send_buffer), "%s %s %s", input_tbname, input_column, input_coldata);
                send_packet(sock_info.fd, SQL_SHOW_TB_DEL, send_buffer);
                sleep(1);
                clear_input_buffer();
                break;
            case 4:
                printf("Insert table name: ");
                scanf("%s", &input_tbname);
                printf("Column int data1: ");
                scanf("%s", &input_deldata1);
                printf("Column char data2: ");
                scanf("%s", &input_deldata2);
                
                snprintf(send_buffer, sizeof(send_buffer), "%s %s %s", input_tbname, input_deldata1, input_deldata2);
                send_packet(sock_info.fd, SQL_SHOW_TB_INT, send_buffer);
                sleep(1);
                clear_input_buffer();
                break;
            case 99:
                return 0;
		}
    }
    pthread_join(recv_thread_id, NULL);
    return 0;
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
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
	struct sockaddr_in serv_adr;

	sock=socket(PF_INET, SOCK_STREAM, 0);   
	if(sock==-1)
		error_handling("socket() error");
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=inet_addr("10.0.2.4");
	serv_adr.sin_port=htons(8888);

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
	
    switch (packet.header.type){

    case SQL_SHOW_TB:
        printf("%s", packet.buf);
        break;
    case SQL_SHOW_TB_LIST:
        printf("%s", packet.buf);
        break;
    case SQL_SHOW_TB_DEL:
        printf("%s", packet.buf);
        break;
    case SQL_SHOW_TB_INT:
        printf("%s", packet.buf);
        break;
    case EVT_WARNING:
       	printf("[WARNING] %s\n", packet.buf);
        break;
    default:
        break;
    }
}

