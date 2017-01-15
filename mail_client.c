#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include "messages.h"


#define CLIENT_INPUT_MAX_SIZE 512 
#define STR_USERNAME "User:"


typedef enum {
	OPERATION_LOGIN,OPERATION_SHOW_INBOX, OPERATION_GET_MAIL, OPERATION_DELETE_MAIL, OPERATION_COMPOSE, 
	OPERATION_SHOW_ONLINE_USERS, OPERATION_CHAT_MSG, OPERATION_QUIT, OPERATION_UNKNOWN
} operation;

typedef struct {
	operation oper;
	char client_input[CLIENT_INPUT_MAX_SIZE];
} client_command;


void concat(char s1[], char s2[]);
int get_colon_index(char s[]);
int hostname_to_ip(char *hostname , char *ip);
int recv_welcome_message(int client_sock);
int recv_login_message(int client_sock, int* check_login);
int recv_show_inbox_message(int client_sock);
int recv_get_mail_message(int client_sock);
int recv_delete_mail_message(int client_sock);
int recv_compose_message(int client_sock);
int recv_show_online_users_message(int client_sock);
int recv_quit_status(int client_sock);
void get_client_command(client_command* c);
int send_data_to_server(int client_sock, char* parameters, int opcode);
int send_compose_message_to_server(int client_sock, int opcode);
int client_to_server(int client_sock);
int initiate_client(unsigned short int port, char* hostname);


void concat(char s1[], char s2[]){
   int i, j;
 
   i = strlen(s1);
 
   for (j = 0; s2[j] != '\0'; i++, j++) {
      s1[i] = s2[j];
   }
 
   s1[i] = '\0';
}


int get_colon_index(char s[]){
    int i = 0;
    
    while (i < strlen(s)){
	if (s[i] == ':'){
	    break;
	}
	
	i+=1;      
    }
    
    return i;
}


int hostname_to_ip(char *hostname , char *ip){
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in *h;
    int rv;
 
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM;
 
    if ( (rv = getaddrinfo( hostname , "http" , &hints , &servinfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) 
    {
        h = (struct sockaddr_in *) p->ai_addr;
        strcpy(ip , inet_ntoa( h->sin_addr ) );
    }
     
    freeaddrinfo(servinfo); 
    return 0;
}



int recv_welcome_message(int client_sock){ 
  
    protocol_message msg;
    memset(msg.data, '\0', MAX_DATA_SIZE);
    int state_flag = recv_protocol_message(client_sock, &msg);
    
    if (state_flag != STATE_SUCCESS){
	return state_flag;
    }
    
    if (msg.header.opcode != OPCODE_WELCOME){
	printf("Unexpected operation from server: %d", msg.header.opcode);
	return STATE_ERROR;
    }
    msg.data[msg.header.data_length] = '\0'; 
    printf("%s\n", msg.data);
    return STATE_SUCCESS;
}

int recv_login_message(int client_sock, int* check_login){
  
    protocol_message msg;
    memset(msg.data, '\0', MAX_DATA_SIZE);
    int state_flag = recv_protocol_message(client_sock, &msg);
    
    if (state_flag != STATE_SUCCESS){
	return state_flag;
    }
  
    if ((msg.header.opcode != OPCODE_LOGIN_SUCCEED) && (msg.header.opcode != OPCODE_LOGIN_FAILED)){
	printf("Unexpected operation from server: %d", msg.header.opcode);
	return STATE_ERROR;
    }
    
    msg.data[msg.header.data_length] = '\0'; 
    if (strcmp(msg.data, SERVER_LOGIN_SUCCEED_STRING) == 0){
	*check_login = 1;
    }
    printf("%s\n", msg.data);
    return STATE_SUCCESS;  
  
}

int recv_show_inbox_message(int client_sock){
        
    protocol_message msg;
    memset(msg.data, '\0', MAX_DATA_SIZE);
    int state_flag = recv_protocol_message(client_sock, &msg);
      
    while ((msg.header.opcode != OPCODE_SHOW_INBOX_END) && (state_flag == STATE_SUCCESS)){
	if (msg.header.opcode == OPCODE_MAIL_ID){
	  
	    msg.data[msg.header.data_length] = '\0'; 
	    printf("%s ", msg.data);
	    
	} else if (msg.header.opcode == OPCODE_SENDER){
	  
	    msg.data[msg.header.data_length] = '\0'; 
	    printf("%s ", msg.data);	  
	  
	} else if (msg.header.opcode == OPCODE_SUBJECT){
	    msg.data[msg.header.data_length] = '\0'; 
	    printf("\"%s\"\n", msg.data);	  
	}
	state_flag = recv_protocol_message(client_sock, &msg);
	
    }
 
  return state_flag; 
}


int recv_get_mail_message(int client_sock){ 
    
    protocol_message msg;
    int state_flag = recv_protocol_message(client_sock, &msg);
    
    if (state_flag != STATE_SUCCESS){
	return state_flag;
    }
    if ((msg.header.opcode != OPCODE_MAIL_NOT_FOUND) && (msg.header.opcode != OPCODE_FROM)){
	printf("Unexpected operation from server: %d", msg.header.opcode);
	return STATE_ERROR;	  
    } else if (msg.header.opcode == OPCODE_MAIL_NOT_FOUND){
	msg.data[msg.header.data_length] = '\0'; 
	
	printf("A mail with this id doesn't exist\n");   
	return STATE_SUCCESS;
    } else{ //m.header.opcode == OPCODE_FROM
	
	/*printing FROM message */
	msg.data[msg.header.data_length] = '\0'; 
	printf("From: %s\n", msg.data);
	    
	state_flag = recv_protocol_message(client_sock, &msg);
    
	if (state_flag != STATE_SUCCESS){
	    return state_flag;
	}
	    
	if (msg.header.opcode != OPCODE_TO){
	    printf("Unexpected operation from server: %d", msg.header.opcode);
	    return STATE_ERROR;	      	      
	}
	      
	/*printing TO message */
	msg.data[msg.header.data_length] = '\0'; 
	printf("To: %s\n", msg.data);		
		
	state_flag = recv_protocol_message(client_sock, &msg);
    
	if (state_flag != STATE_SUCCESS){
	    return state_flag;
	}
	    
	if (msg.header.opcode != OPCODE_SUBJECT){
	    printf("Unexpected operation from server: %d", msg.header.opcode);
	    return STATE_ERROR;	      	      
	}
	      
	/*printing SUBJECT message */
	msg.data[msg.header.data_length] = '\0'; 
	printf("Subject: %s\n", msg.data);
	    
	state_flag = recv_protocol_message(client_sock, &msg);
    
	if (state_flag != STATE_SUCCESS){
	    return state_flag;
	}
	    
	if (msg.header.opcode != OPCODE_TEXT){
	    printf("Unexpected operation from server: %d", msg.header.opcode);
	    return STATE_ERROR;	      	      
	}
	      
	/*printing TEXT message */
	msg.data[msg.header.data_length] = '\0'; 
	printf("Text: %s\n", msg.data);		
    }
    return STATE_SUCCESS; 
}


int recv_delete_mail_message(int client_sock){
  
    protocol_message msg;
    int state_flag = recv_protocol_message(client_sock, &msg);
    
    if (state_flag != STATE_SUCCESS){
	return state_flag;
    }
  
    if ((msg.header.opcode != OPCODE_MAIL_NOT_FOUND) && (msg.header.opcode != OPCODE_MAIL_DELETED)){
	printf("Unexpected operation from server: %d", msg.header.opcode);
	return STATE_ERROR;
    }
    
    return STATE_SUCCESS;  
  
}


int recv_compose_message(int client_sock){
  
    protocol_message msg;
    int state_flag = recv_protocol_message(client_sock, &msg);
    
    if (state_flag != STATE_SUCCESS){
	return state_flag;
    }
  
    if ((msg.header.opcode != OPCODE_COMPOSE_SUCCEED) && (msg.header.opcode != OPCODE_COMPOSE_FAILED)){
	printf("Unexpected operation from server: %d", msg.header.opcode);
	return STATE_ERROR;
    }
    
    msg.data[msg.header.data_length] = '\0'; 
    printf("%s\n", msg.data);
    return STATE_SUCCESS;  
  
}




int recv_show_online_users_message(int client_sock){
  
    protocol_message msg;
    int state_flag = recv_protocol_message(client_sock, &msg);
    
    if (state_flag != STATE_SUCCESS){
	return state_flag;
    }
  
    if (msg.header.opcode != OPCODE_SHOW_ONLINE_USERS){
	printf("Unexpected operation from server: %d", msg.header.opcode);
	return STATE_ERROR;
    }
    
    msg.data[msg.header.data_length] = '\0'; 
    printf("Online users: %s\n", msg.data);	
    
    return STATE_SUCCESS;  
}
  
  
  
int recv_status_chat_msg(int client_sock){
  
    protocol_message msg;
    int state_flag = recv_protocol_message(client_sock, &msg);
    
    if (state_flag != STATE_SUCCESS){
	return state_flag;
    }
  
    if ((msg.header.opcode != OPCODE_COMPOSE_SUCCEED) && (msg.header.opcode != OPCODE_COMPOSE_FAILED)){
	printf("Unexpected operation from server: %d", msg.header.opcode);
	return STATE_ERROR;
    }
    return STATE_SUCCESS;  
  
}

  
int recv_quit_status(int client_sock){
  
    protocol_message msg;
    int state_flag = recv_protocol_message(client_sock, &msg);
    
    if (state_flag != STATE_SUCCESS){
	return state_flag;
    }
  
    if (msg.header.opcode != OPCODE_QUIT_OK){
	printf("Unexpected operation from server: %d", msg.header.opcode);
	return STATE_ERROR;
    }
    	   
    return STATE_SUCCESS;  
}





void get_client_command(client_command* c){
  
    char input[CLIENT_INPUT_MAX_SIZE+1] = {0};
    char operation[CLIENT_INPUT_MAX_SIZE+1];
    char *word, *params;
    
    fgets(input, CLIENT_INPUT_MAX_SIZE, stdin);
	
    c->oper = OPERATION_UNKNOWN;
    memset(c->client_input, '\0', CLIENT_INPUT_MAX_SIZE);

    char username[MAX_USERNAME+1];
    char password[MAX_PASSWORD+1];
    char username_password[MAX_USERNAME+MAX_PASSWORD+1];
    
    word = strtok(input, " "); //supposed to contain the word Username: 

    if (word == NULL){
      return;
    }
    
    strncpy(operation, word, CLIENT_INPUT_MAX_SIZE);
    operation[CLIENT_INPUT_MAX_SIZE+1] ='\0';
	  
    if (strncmp(operation, STR_USERNAME, 5) == 0){
      
	word = strtok(NULL, "\n");
	
	if (word == NULL){
	  return;
	}
	strncpy(username, word, MAX_USERNAME);//the username itself
	username[strlen(username)] = '\0'; 
	
	fgets(input, CLIENT_INPUT_MAX_SIZE, stdin);
	
	word = strtok(input, " "); //supposed to contain the word Password: 
	if (word == NULL){
	  return;
	}
	
	word = strtok(NULL, "\n");
	
	if (word == NULL){
	  return;
	}
	strncpy(password, word, MAX_PASSWORD);
	password[strlen(password)]='\0'; //the password itself

	strncpy(username_password, username, MAX_USERNAME);
	concat(username_password, "\t" );
	concat(username_password, password); //contain username tab password
	
	c->oper = OPERATION_LOGIN;
	strncpy(c->client_input, username_password, CLIENT_INPUT_MAX_SIZE);

    
    }
    else if (strncmp(operation, "SHOW_INBOX", 10) == 0){
	c->oper = OPERATION_SHOW_INBOX;
    }
    else if (strncmp(operation, "GET_MAIL", 8) == 0){
	
	c->oper = OPERATION_GET_MAIL;
	params = strtok(NULL, "\n");
	
	strncpy(c->client_input, params, CLIENT_INPUT_MAX_SIZE);
	
    }
    else if (strncmp(operation, "DELETE_MAIL", 11) == 0){
	c->oper = OPERATION_DELETE_MAIL;
	params = strtok(NULL, "\n");
	strncpy(c->client_input, params, CLIENT_INPUT_MAX_SIZE);
    }
    else if (strncmp(operation, "COMPOSE", 7) == 0){
	c->oper = OPERATION_COMPOSE;
    }	
    else if (strncmp(operation, "SHOW_ONLINE_USERS", 17) == 0){
	c->oper = OPERATION_SHOW_ONLINE_USERS;
    }
    else if (strncmp(operation, "MSG", 3) == 0){
	c->oper = OPERATION_CHAT_MSG;
    }	    
    else if (strncmp(operation, "QUIT", 4) == 0){
	c->oper = OPERATION_QUIT;
    }
}


int send_data_to_server(int client_sock, char* parameters, int opcode){
    protocol_message msg;
    memset(msg.data, '\0', MAX_DATA_SIZE);
    msg.header.opcode = opcode;
    msg.header.data_length = strnlen(parameters, CLIENT_INPUT_MAX_SIZE);
    if (msg.header.data_length > CLIENT_INPUT_MAX_SIZE){
	printf("Input is too long, must be < %d\n", CLIENT_INPUT_MAX_SIZE);
	return STATE_ERROR; //error      
    }
    strncpy(msg.data, parameters, MAX_DATA_SIZE-1); 
 
    if (send_protocol_message(client_sock, &msg)){
	return STATE_ERROR;
    }
    return STATE_SUCCESS;
}


int send_compose_message_to_server(int client_sock, int opcode){
    char input[CLIENT_INPUT_MAX_SIZE+1] = {0};
    protocol_message msg;
    memset(msg.data, '\0', MAX_DATA_SIZE);
    int colon_index = 0;
    msg.header.opcode = opcode;
    fgets(input, CLIENT_INPUT_MAX_SIZE, stdin); //getting To: data from client
    msg.header.data_length = strlen(input);
    if (msg.header.data_length > CLIENT_INPUT_MAX_SIZE){
	printf("'To' field is too long, must be < %d\n", CLIENT_INPUT_MAX_SIZE);
	return STATE_ERROR; //error      
    }
    
    colon_index = get_colon_index(input);
    strncpy(msg.data, &input[colon_index+2], MAX_DATA_SIZE-1); 
    /*send COMPOSE message that contains 'TO' details*/
    if (send_protocol_message(client_sock, &msg)){
	return STATE_ERROR;
    }
       
    fgets(input, CLIENT_INPUT_MAX_SIZE, stdin);
    msg.header.opcode = OPCODE_SUBJECT;
    msg.header.data_length = strlen(input);
    if (msg.header.data_length > CLIENT_INPUT_MAX_SIZE){
	printf("'Subject' field is too long, must be < %d\n", CLIENT_INPUT_MAX_SIZE);
	return STATE_ERROR; //error      
    }
    
    colon_index = get_colon_index(input);
    memset(msg.data, '\0', MAX_DATA_SIZE);
    strncpy(msg.data, &input[colon_index+2], MAX_DATA_SIZE-1); 
    /*send SUBJECT message*/
    if (send_protocol_message(client_sock, &msg)){
	return STATE_ERROR;
    }
    
    fgets(input, CLIENT_INPUT_MAX_SIZE, stdin);
    msg.header.opcode = OPCODE_TEXT;
    msg.header.data_length = strnlen(input, CLIENT_INPUT_MAX_SIZE);
    if (msg.header.data_length > CLIENT_INPUT_MAX_SIZE){
	printf("'Text' field is too long, must be < %d\n", CLIENT_INPUT_MAX_SIZE);
	return STATE_ERROR; //error      
    }
    
    colon_index = get_colon_index(input);
    memset(msg.data, '\0', MAX_DATA_SIZE);
    strncpy(msg.data, &input[colon_index+2], MAX_DATA_SIZE-1);
    /*send TEXT message*/
    if (send_protocol_message(client_sock, &msg)){
	return STATE_ERROR;
    }
    
    return STATE_SUCCESS;
}


int client_to_server(int client_sock){ 
    int state_flag = 0;
    int first_use = 1;
    int check_login;
    client_command c;

    state_flag = recv_welcome_message(client_sock); // receive welcome message
    if (state_flag != STATE_SUCCESS){
	return state_flag;
    }
    
    c.oper = OPERATION_UNKNOWN;
    
    while (c.oper != OPERATION_QUIT && state_flag == STATE_SUCCESS){ 
	get_client_command(&c);
	
	while (first_use == 1){
	    check_login = 0;
	    while (c.oper != OPERATION_LOGIN){ //as long as the client doesn't type login details
		printf("Please enter your username and password to login.\n");
		get_client_command(&c);
	    }
	    //client typed login details, now check if client has valid username and password
	    state_flag = send_data_to_server(client_sock, c.client_input, OPCODE_LOGIN);
	    if (state_flag == STATE_SUCCESS){ //only if the data was sent to server successfully , recv data back from server
		state_flag = recv_login_message(client_sock, &check_login);
		
	    }	   	    
	    if (check_login == 1){ //login succeed 
		first_use = 0; 
	    }
	    get_client_command(&c);
	}
		
	switch(c.oper){	  
	  case OPERATION_SHOW_INBOX:
	      state_flag = send_data_to_server(client_sock, c.client_input, OPCODE_SHOW_INBOX);
	      if (state_flag == STATE_SUCCESS){
		  state_flag = recv_show_inbox_message(client_sock);
	      }
	      break;
	  case OPERATION_GET_MAIL:
	      state_flag = send_data_to_server(client_sock, c.client_input, OPCODE_GET_MAIL);
	      if (state_flag == STATE_SUCCESS){ //only if the data was sent to server successfully , recv data back from server
		  state_flag = recv_get_mail_message(client_sock);
	      }
	      break;
	  case OPERATION_DELETE_MAIL:
	      state_flag = send_data_to_server(client_sock, c.client_input, OPCODE_DELETE_MAIL);
	      if (state_flag == STATE_SUCCESS){ //only if the data was sent to server successfully , recv data back from server
		  state_flag = recv_delete_mail_message(client_sock);
	      }
	      break;
	  case OPERATION_COMPOSE:
	      state_flag = send_compose_message_to_server(client_sock, OPCODE_COMPOSE);
	      if (state_flag == STATE_SUCCESS){ //only if the data was sent to server successfully , recv data back from server
		  state_flag = recv_compose_message(client_sock);
	      }
	      break;
	  case OPERATION_SHOW_ONLINE_USERS:
	      state_flag = send_data_to_server(client_sock, c.client_input, OPCODE_SHOW_ONLINE_USERS);
	      if (state_flag == STATE_SUCCESS){ //only if the data was sent to server successfully , recv data back from server
		  state_flag = recv_show_online_users_message(client_sock);
	      }
	      break;
	  case OPERATION_CHAT_MSG:
	      state_flag = send_data_to_server(client_sock, c.client_input, OPCODE_CHAT_MSG);
	      if (state_flag == STATE_SUCCESS){ //only if the data was sent to server successfully , recv data back from server
		  state_flag = recv_status_chat_msg(client_sock);		
	      }
	      break;
	  case OPERATION_QUIT:
	      state_flag = send_data_to_server(client_sock, c.client_input, OPCODE_QUIT);
	      if (state_flag == STATE_SUCCESS){ //only if the data was sent to server successfully , recv data back from server
		  state_flag = recv_quit_status(client_sock);
	      }
	      break;
	 default:
	     printf("Invalid command, try one of these: 'SHOW_INBOX', 'GET_MAIL', 'DELETE_MAIL', 'COMPOSE', 'SHOW_ONLINE_USERS', 'MSG', 'QUIT'\n");
	     break;
	}
	
    }
    return state_flag;
}

int initiate_client(unsigned short int port, char* hostname){
    struct sockaddr_in server_addr;
    int client_sock;
    int state_flag;
   
    client_sock = socket(AF_INET, SOCK_STREAM, 0); //create the socket
    if (client_sock < 0){ //error 
	printf("Error in socket function: %s\n", strerror(errno));
	return 1;
    }
    
     memset(&server_addr, '0', sizeof(server_addr)); 

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port); 
    if (strchr(hostname, '.') == NULL){ //name
	char ip[100];    
	hostname_to_ip(hostname , ip);
	server_addr.sin_addr.s_addr = inet_addr(ip);
	
    }else{ //ip number
	server_addr.sin_addr.s_addr = inet_addr(hostname); 
    }
    if (connect(client_sock,(struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){ 
       	printf("Error in connect function: %s\n", strerror(errno));
	return 1;
    } 
    
    state_flag = client_to_server(client_sock);
    
    if (close(client_sock) < 0){
	  printf("Error in close function: %s\n", strerror(errno));		      
    }
    return state_flag;
}
    
    
int main(int argc, char** argv){
  
    unsigned short int port = DEFAULT_PORT;
    char hostname[MAX_HOSTNAME_LENGTH+1] = DEFAULT_HOSTNAME;
 
    if (argc == 0 || argc > 3){ //mail_client [hostname [port]] cant write port without hostname 
	printf("Illegal number of arguments\n");
	return STATE_ERROR;
    }
	
    if (argc == 3){
	port = (unsigned short int) atoi(argv[2]);
	strncpy(hostname, argv[1], MAX_HOSTNAME_LENGTH);
	hostname[strlen(hostname)] = '\0' ;
    }

    if (argc == 2){
	if (atoi(argv[1]) != 0){ // argv[1] is a number	
	    if (strchr(argv[1], '.') == NULL){ //dot wasn't found = port number -> the case where the client wrote port without a hostname
		printf("Invalid input: you can't write port without hostname\n");
		return STATE_ERROR;
	    }else{ //dot was found - it's an IP number = hostname -> the case of only hostname 
		strncpy(hostname, argv[1], MAX_HOSTNAME_LENGTH);
		hostname[strlen(hostname)] = '\0' ;
	    }
	}else{ // the case of only hostname 
	    strncpy(hostname, argv[1], MAX_HOSTNAME_LENGTH);
	    hostname[strlen(hostname)] = '\0' ;     
	}
    }
    
    if (initiate_client(port, hostname) == STATE_ERROR){ //error
      return STATE_ERROR;
    }
    return STATE_SUCCESS;
}
