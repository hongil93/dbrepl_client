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

#define MAX_DB_STATUS 12
#define MAX_LINE_LENGTH 40

#define REP_CHECK 402
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
        
        int str_len = recv(sock, &recv_msg.header, sizeof(recv_msg.header), 0);
        if (str_len <= 0) {
            break;
        }

        remaining_data = recv_msg.header.length;
        
        
        while (remaining_data > 0) {
            str_len = recv(sock, recv_msg.buf + total_received, remaining_data, 0);
            if (str_len <= 0) {
                break;
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

// /* Over line --> \n */
// void print_with_line_break(const char *label, const char *value) {
//     printf("\t  %s: ", label);
//     int len = strlen(value);
//     int start = 0;

//     while (start < len) {
//         int to_print_length = (len - start > MAX_LINE_LENGTH) ? MAX_LINE_LENGTH : (len - start);
        
//         if(to_print_length > 5){
//             printf("\n\t  %.*s\n", to_print_length, value + start);
//         }else{
//             printf("%.*s\n", to_print_length, value + start);
//         }
        
//         start += to_print_length;
//     }
// }

// /* print db01, db02 status */
// void print_db_status(const char *db_prefix, char *db_status[]){
//     printf("===================== %s status =====================\n", db_prefix);
//     printf("\t  Master_Log_File: %s\n", db_status[0]);
//     printf("\t  Read_Master_Log_Pos: %s\n", db_status[1]);
//     printf("\t  Slave_IO_Running: %s\n", db_status[2]);
//     printf("\t  Slave_SQL_Running: %s\n", db_status[3]);
//     printf("\t  Last_IO_Errno: %s\n", db_status[4]);
//     print_with_line_break("Last_IO_Error", db_status[5]);
//     printf("\t  Last_SQL_Errno: %s\n", db_status[6]);
//     print_with_line_break("Last_SQL_Error", db_status[7]);
// }
// /* print db01, db02 "MASTER" status */
// void print_db_master_status(const char *db_prefix, char *db_status[]){
//     printf("===================== %s status =====================\n", db_prefix);
//     printf("\t  File: %s\n", db_status[8]);
//     printf("\t  Position: %s\n", db_status[9]);
//     printf("\t  Binlog_Do_DB: %s\n", db_status[10]);
//     printf("\t  Binlog_Ignore_DB: %s\n", db_status[11]);
// }

void type_categorizer(Packet packet, int fd){
	if(packet.header.type == REP_CHECK){
        
        printf("%s", packet.buf);

    //     char *db_status[2] [MAX_DB_STATUS]; // db01, db02
    //     char *token = strtok(packet.buf, ",");

    //     /* read db01, db02 status */
    //     for(int db = 0; db < 2; db++){
    //         for(int i = 0; i < MAX_DB_STATUS; i++){
    //             if(token != NULL){
    //                 db_status[db][i] = token;
    //                 token = strtok(NULL, ","); // next token
    //             }
    //         }
    //     }
        
    //     /* db replication status */
    //     const char *DB_replication_status =
    //         (strcmp(db_status[0][2], "Yes") == 0) && (strcmp(db_status[0][3], "Yes") == 0)
    //         && (strcmp(db_status[1][2], "Yes") == 0) && (strcmp(db_status[1][3], "Yes") == 0)?
    //         "SUCCESS" : "FAIL";
        

    //     system("clear"); // screen clear
    //     printf("==================== DB replication ===================\n");
    //     printf("\t\t\t %s\n", DB_replication_status);

    //     print_db_status("db01", db_status[0]);
    //     print_db_master_status("db01", db_status[0]);
    //     print_db_status("db02", db_status[1]);
    //     print_db_master_status("db02", db_status[1]);
    //     printf("=======================================================\n\n");
    //     fflush(stdout); // buffer clear
    }
}