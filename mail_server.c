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
#include "messages.h" 
#include "mail.h"


user_details client_array[20];

void concat(char s1[], char s2[]);
int check_user(Node *node_to_check, const char* username);
int send_mail_to_client(int client_socket, Node *mailNode);
int handle_show_inbox(int client_socket);
int handle_compose(int client_socket, const char* client_input);
int handle_delete(int client_socket, const char* client_parameters);
int handle_getMail(int client_socket, const char* client_parameters); 
int handle_show_online_users(int client_socket, FILE* fp);
int handle_quit(int client_socket);
int check_username_passowrd_exist(int client_sock, const char* data, FILE* fp);
int command_login(int client_sock, const char* data, FILE* fp);
int server_to_client_new(int client_sock, char usersFilePath[]);
int initiate_server(unsigned short int port, char usersFilePath[]);





void concat(char s1[], char s2[]){
   int i, j;
 
   i = strlen(s1);
 
   for (j = 0; s2[j] != '\0'; i++, j++) {
      s1[i] = s2[j];
   }
 
   s1[i] = '\0';
}

//returns 0 if the mail in node_to_check is in the inbox of the current client,  and -1 otherwise
int check_user(Node *node_to_check, const char* username){
	if( strcmp(node_to_check->email.username, username) == 0 )
		return 0;
	return -1;
}


int send_mail_to_client(int client_socket, Node *mailNode){
	//send the mail_id
	protocol_message msg;
	msg.header.opcode = OPCODE_MAIL_ID;
	memset(msg.data, '\0', MAX_DATA_SIZE);
	sprintf(msg.data,"%d", mailNode->email.mail_id );
	msg.header.data_length = strlen(msg.data);
	
	if (send_protocol_message(client_socket, &msg)) 
		return STATE_ERROR;
	//send the mail sender
	protocol_message sender_msg;
	sender_msg.header.opcode = OPCODE_SENDER;
	sender_msg.header.data_length = strlen(mailNode->email.mail_sender);
	memset(sender_msg.data, '\0', MAX_DATA_SIZE);
	strncpy(sender_msg.data, mailNode->email.mail_sender, strlen(mailNode->email.mail_sender));
	if (send_protocol_message(client_socket, &sender_msg)) 
		return STATE_ERROR;
	//send the subject
	protocol_message subject_msg;
	subject_msg.header.opcode = OPCODE_SUBJECT;
	subject_msg.header.data_length = strlen(mailNode->email.subject);
	memset(subject_msg.data, '\0', MAX_DATA_SIZE);
	strncpy(subject_msg.data, mailNode->email.subject, strlen(mailNode->email.subject));
	if (send_protocol_message(client_socket, &subject_msg)) 
		return STATE_ERROR;

	return STATE_SUCCESS;	
}

int handle_show_inbox(int client_socket){
	//OPCODE_SHOW_INBOX_END		OPCODE_MAIL_ID		OPCODE_SENDER		OPCODE_SUBJECT
	//find the first Node in the linked list of mails(if exists) with the client's username:
	mail mailToFind;	Node* mailNode;	
	int i;
	user_details ud;
	char username[MAX_USERNAME+2];
	
	memset(username,'\0', MAX_USERNAME+2);
	memset(mailToFind.username, '\0', sizeof(mailToFind.username));
	
        for (i = 0 ; i < MAX_CLIENTS ; i++){
            ud = client_array[i];
	    if (ud.socket_fd == client_socket){
	      strncpy(username, ud.username, sizeof(ud.username));
	      username[MAX_USERNAME+1] = '\0';
	      break;
	    }
	}
	  	
	strcpy(mailToFind.username, username);
	mailNode = findUser(mailToFind);
	int state_flag = 0;
	if(mailNode != NULL)
	{
		//send the first mail to the client
		if(mailNode->email.is_deleted != -88) 
		{
			state_flag = send_mail_to_client(client_socket, mailNode);
			if (state_flag != STATE_SUCCESS) {
				return state_flag;
			}
		}
	}
	else {
		protocol_message msg;
		msg.header.opcode = OPCODE_SHOW_INBOX_END;
		msg.header.data_length = 0;
		memset(msg.data, '\0', MAX_DATA_SIZE);
		if (send_protocol_message(client_socket, &msg)) 
			return STATE_ERROR;
		return STATE_SUCCESS;	
	}

	//iterate over the linked list of mails until reaching a different username
	int user = 0;
	mailNode = mailNode->next;   
	while(mailNode != NULL){
		user = check_user(mailNode, username);
		if(user == -1)
			break;

		//make sure mail isn't marked deleted
		if(mailNode->email.is_deleted == -88){
			mailNode = mailNode->next;
			continue;
		}

		state_flag = send_mail_to_client(client_socket, mailNode);
		if (state_flag != STATE_SUCCESS) {
			return state_flag;
		}
		mailNode = mailNode->next;
	}

	//send end message to client
	protocol_message msg;
	msg.header.opcode = OPCODE_SHOW_INBOX_END;
	msg.header.data_length = 0;
	memset(msg.data, '\0', MAX_DATA_SIZE);
	if (send_protocol_message(client_socket, &msg)) 
		return STATE_ERROR;

	return STATE_SUCCESS;	 
}


int handle_compose(int client_socket, const char* client_input)
{
	mail mail_to_insert;
	int i;
	char username[MAX_USERNAME+2];
	user_details ud;
	
	memset(username,'\0', MAX_USERNAME+2);
	
	for (i = 0 ; i < MAX_CLIENTS ; i++){
            ud = client_array[i];
	    if (ud.socket_fd == client_socket){
	      strncpy(username, ud.username, sizeof(ud.username));
	      username[MAX_USERNAME+1] = '\0';
	      break;
	    }
	}
	
	memset(mail_to_insert.mail_sender, '\0', MAX_USERNAME_SIZE + 1 );
	strncpy(mail_to_insert.mail_sender, username, MAX_USERNAME);
	memset(mail_to_insert.mail_recievers, '\0', MAX_RECIEVERS * MAX_USERNAME_SIZE +19 +1 );
	strncpy(mail_to_insert.mail_recievers, client_input, strlen(client_input) -1 ); //last char in client_input is \n

	char str[MAX_RECIEVERS * MAX_USERNAME_SIZE +19 +1] = {0};
	strncpy(str, mail_to_insert.mail_recievers, MAX_RECIEVERS * MAX_USERNAME +19 );  //19 for the ","  ,  1 for \0.  no spaces allowed
    	const char s[2] = ",";
    	char *token;
	//recieve SUBJECT message from client:
	protocol_message subjectMsg;
	memset(subjectMsg.data, '\0', MAX_DATA_SIZE);
	int state_flag = recv_protocol_message(client_socket, &subjectMsg);
	if (state_flag != STATE_SUCCESS){
		return state_flag;
	}
	memset(mail_to_insert.subject, '\0', MAX_SUBJECT_SIZE + 1 );
	strncpy(mail_to_insert.subject, subjectMsg.data, strlen(subjectMsg.data) -1 );

	//recieve TEXT message from client:
	protocol_message textMsg;
	memset(textMsg.data, '\0', MAX_DATA_SIZE);
	state_flag = recv_protocol_message(client_socket, &textMsg);
	if (state_flag != STATE_SUCCESS){
		return state_flag;
	}
	memset(mail_to_insert.text, '\0', MAX_TEXT_SIZE + 1 );
	strncpy(mail_to_insert.text, textMsg.data, strlen(textMsg.data) -1 );

	protocol_message server_answer;
	memset(server_answer.data, '\0', MAX_DATA_SIZE);
	
	int error_occured = 0;
	token = strtok(str, s);
	while( token != NULL ){
	   	memset(mail_to_insert.username, '\0', MAX_USERNAME+1);
		strncpy(mail_to_insert.username, token , MAX_USERNAME);

		//add the mail to the mail list kept in the server:
		int is_error = addMail(mail_to_insert);
		if(is_error == -1){  //server already contains 32,000 mails (including deleted mails)  and cannot add another mail
			error_occured = -1;
		}
		
		token = strtok(NULL, s);
   	}//end of while loop

	if(error_occured == -1) {
		server_answer.header.opcode = OPCODE_COMPOSE_FAILED;
		strcpy(server_answer.data, "Server contains 32,000 mails and is full. Therefore COMPOSE failed");
		server_answer.header.data_length = strlen("Server contains 32,000 mails and is full. Therefore COMPOSE failed");
	}
	else {
		server_answer.header.opcode = OPCODE_COMPOSE_SUCCEED;
		strcpy(server_answer.data, "Mail sent");
		server_answer.header.data_length = strlen("Mail sent");
	}	

	if (send_protocol_message(client_socket, &server_answer)){
		return STATE_ERROR;
	 }
	return STATE_SUCCESS;
}


int handle_delete(int client_socket, const char* client_parameters){

	protocol_message msg;    short mail_id;
	memset(msg.data, '\0', MAX_DATA_SIZE);
	mail_id = atoi(client_parameters);

	mail mailToFind;
	int i;
	char username[MAX_USERNAME+2];
	user_details ud;
	
	memset(username,'\0', MAX_USERNAME+2);
	
	for (i = 0 ; i < MAX_CLIENTS ; i++){
            ud = client_array[i];
	    if (ud.socket_fd == client_socket){
	      strncpy(username, ud.username, sizeof(ud.username));
	      username[MAX_USERNAME+1] = '\0';
	      break;
	    }
	}
	
	
	
	memset(mailToFind.username, '\0', MAX_USERNAME_SIZE+1);
	strcpy(mailToFind.username, username);
	mailToFind.mail_id = mail_id;
	int result = delete_mail(mailToFind);
	//case where mail wasn't in the list:
	if(result == -1) 
	{
		msg.header.data_length = 0;
		msg.header.opcode = OPCODE_MAIL_NOT_FOUND;
		if (send_protocol_message(client_socket, &msg))
			return STATE_ERROR;
		return STATE_SUCCESS;
	}
	//OPCODE_MAIL_DELETED
	msg.header.data_length = 0;
	msg.header.opcode = OPCODE_MAIL_DELETED;
	if (send_protocol_message(client_socket, &msg))
		return STATE_ERROR;
	return STATE_SUCCESS;
}



int handle_getMail(int client_socket, const char* client_parameters){
	protocol_message msg;    short mail_id;
	memset(msg.data, '\0', MAX_DATA_SIZE);
	mail_id = atoi(client_parameters);

	mail mailToFind;
	int i;
	char username[MAX_USERNAME+2];
	user_details ud;
	
	memset(username,'\0', MAX_USERNAME+2);
	
	for (i = 0 ; i < MAX_CLIENTS ; i++){
            ud = client_array[i];
	    if (ud.socket_fd == client_socket){
	      strncpy(username, ud.username, sizeof(ud.username));
	      username[MAX_USERNAME+1] = '\0';
	      break;
	    }
	}
	
	
	memset(mailToFind.username, '\0', MAX_USERNAME_SIZE+1);
	strcpy(mailToFind.username, username);
	mailToFind.mail_id = mail_id;
	Node *mailNode = GetMail(mailToFind);
	//case where mail wasn't found
	if(mailNode == NULL){

		msg.header.data_length = 0;
		msg.header.opcode = OPCODE_MAIL_NOT_FOUND;
		if (send_protocol_message(client_socket, &msg))
			return STATE_ERROR;
		return STATE_SUCCESS;
	}
	//case where mail is marked deleted:
	if(mailNode->email.is_deleted == -88) {
		msg.header.data_length = 0;
		msg.header.opcode = OPCODE_MAIL_NOT_FOUND;
		if (send_protocol_message(client_socket, &msg))
			return STATE_ERROR;
		return STATE_SUCCESS;
	}
	
	//reminder:  from  to subject text
	msg.header.opcode = OPCODE_FROM;
	msg.header.data_length = strlen(mailNode->email.mail_sender);
	strncpy(msg.data, mailNode->email.mail_sender, MAX_USERNAME_SIZE);
	if (send_protocol_message(client_socket, &msg)) 
		return STATE_ERROR;

	msg.header.opcode = OPCODE_TO;
	msg.header.data_length = strlen(mailNode->email.mail_recievers);
	strcpy(msg.data, mailNode->email.mail_recievers);  //we can alwyas fit this in one message
	if (send_protocol_message(client_socket, &msg)) 
		return STATE_ERROR;

	msg.header.opcode = OPCODE_SUBJECT;
	msg.header.data_length = strlen(mailNode->email.subject);
	strcpy(msg.data, mailNode->email.subject);  //we can always fit this in one message
	if (send_protocol_message(client_socket, &msg)) 
		return STATE_ERROR;

	msg.header.opcode = OPCODE_TEXT;
	msg.header.data_length = strlen(mailNode->email.text);
	strcpy(msg.data, mailNode->email.text);  //we can always fit this in one message
	if (send_protocol_message(client_socket, &msg)) 
		return STATE_ERROR;

	return STATE_SUCCESS;
}



int handle_show_online_users(int client_socket, FILE* fp){
	int i;
	int users_online_counter = 0;
	user_details ud;
	char username[MAX_USERNAME+5];
	char username_from_file[MAX_USERNAME];
	char users_online[MAX_DATA_SIZE-1]= {0};
	char line[MAX_LINE_LENGTH];	
	char password[51];
	protocol_message msg;
	
   
	while (fgets(line, sizeof(line), fp)){
	    if (line[strlen(line)-1] == '\n'){
	      line[strlen(line)-1] = '\0' ;
	    }
	    memset(username_from_file,'\0', MAX_USERNAME);
	    sscanf(line, "%s %s", username_from_file, password); 
	    
	    for (i = 0 ; i < MAX_CLIENTS ; i++){
		ud = client_array[i];
		if ((strcmp(ud.username,username_from_file)==0) && ud.socket_fd != -1){ //means the user is online		  	
		    users_online_counter+=1;
		    if (users_online_counter >=2){	//2 users or more online therefore delimeter "," is needed	
			memset(username,'\0', MAX_USERNAME+5);
			concat(username,","); //will contain only ","
			concat(username,ud.username); //will contain ",username"
			concat(users_online, username);
		    }else{
			concat(users_online, ud.username);
		    }
		    break;
		}			      
	    }
	}
		
	msg.header.opcode = OPCODE_SHOW_ONLINE_USERS;
	msg.header.data_length = strlen(users_online);
	strncpy(msg.data, users_online, strlen(users_online));
	if (send_protocol_message(client_socket, &msg)) {
		return STATE_ERROR;
	}
	return STATE_SUCCESS;
}





int handle_send_online_chat_msg(int client_socket, int target_user_socket, const char* target_user, const char* chat_msg_text)
{
//      char *chat_msg_string = "New message from ";
//      protocol_message msg;
//
//      concat(chat_msg_string, username);
//      concat(chat_msg_string, ": ");
//      concat(chat_msg_string, chat_msg_text);
//
//      msg.header.opcode = OPCODE_SHOW_ONLINE_USERS;
//      msg.header.data_length = strlen(chat_msg_string);
//      strncpy(msg.data, chat_msg_string, strlen(chat_msg_string));
//
//      if (send_protocol_message(target_user_socket, &msg)) {
//		return STATE_ERROR;
//      }
      
      return STATE_SUCCESS;
}
  
  
  



int handle_send_offline_chat_msg(int client_socket, const char* target_user, const char* chat_msg_text)
{
	mail mail_to_insert;
	int i, is_error;
	char username[MAX_USERNAME+2];
	char *subject_string = "Messege received offline";
	user_details ud;
	
	memset(username,'\0', MAX_USERNAME+2);
	
	for (i = 0 ; i < MAX_CLIENTS ; i++){
            ud = client_array[i];
	    if (ud.socket_fd == client_socket){
	      strncpy(username, ud.username, sizeof(ud.username));
	      username[MAX_USERNAME+1] = '\0';
	      break;
	    }
	}
	
	/* sender */
	memset(mail_to_insert.mail_sender, '\0', MAX_USERNAME_SIZE + 1);
	strncpy(mail_to_insert.mail_sender, username, MAX_USERNAME);
	
	/* receiver */
	memset(mail_to_insert.mail_recievers, '\0', MAX_RECIEVERS * MAX_USERNAME_SIZE +19 +1);
	strncpy(mail_to_insert.mail_recievers, target_user, strlen(target_user)); 
	
	/* subject */
	memset(mail_to_insert.subject, '\0', MAX_SUBJECT_SIZE + 1);
	strncpy(mail_to_insert.subject, subject_string, strlen(subject_string));

	/* text */
	
	memset(mail_to_insert.text, '\0', MAX_TEXT_SIZE + 1);
	strncpy(mail_to_insert.text, chat_msg_text, strlen(chat_msg_text));

	protocol_message server_answer;
	memset(server_answer.data, '\0', MAX_DATA_SIZE);
	

	memset(mail_to_insert.username, '\0', MAX_USERNAME+1);
	strncpy(mail_to_insert.username, target_user, MAX_USERNAME);

	//add the mail to the mail list kept in the server:
	is_error = addMail(mail_to_insert);
	if(is_error == -1){  //server already contains 32,000 mails (including deleted mails)  and cannot add another mail
		 return STATE_ERROR;
	}

	return STATE_SUCCESS;
}


int handle_chat_msg(int client_socket, const char* client_parameters){
//	int i, state_flag;
//	char* target_user;
//	int target_user_socket;
//	char* chat_msg_text;
//	protocol_message server_answer;
//
//	memset(server_answer.data, '\0', MAX_DATA_SIZE);
//
//	target_user = strtok(client_parameters, ":"); //check
//	if (target_user == NULL){
//	  return STATE_ERROR;
//	}
//
//	chat_msg_text = strtok(NULL, "\n");
//	if (chat_msg_text == NULL){
//	    return STATE_ERROR;
//	 }
//
//	//first we check if the user is online now
//	for (i = 0 ; i < MAX_CLIENTS ; i++){
//            ud = client_array[i];
//	    if ((ud.socket_fd != -1) && (strcmp(target_user, ud.username) == 0)){ //the target user is currently ONLINE
//		target_user_socket = i;
//		state_flag = handle_send_online_chat_msg(client_socket, target_user_socket, target_user, chat_msg_text);
//
//	     //do here return
//	    }
//	}
//
//	// if we got here it means the user is OFFLINE now - send the msg as e-mail
//
//
//	state_flag = handle_send_offline_chat_msg(client_socket, target_user, chat_msg_text);
//	if (state_flag == STATE_ERROR){
//	    server_answer.header.opcode = OPCODE_COMPOSE_FAILED;
//	    strcpy(server_answer.data, "Server contains 32,000 mails and is full. Therefore operation failed");
//	    server_answer.header.data_length = strlen("Server contains 32,000 mails and is full. Therefore operation failed");
//
//	 }else{
//	    server_answer.header.opcode = OPCODE_COMPOSE_SUCCEED;
//	    strcpy(server_answer.data, "Mail sent");
//	    server_answer.header.data_length = strlen("Mail sent");
//	 }
//
//	if (send_protocol_message(client_socket, &server_answer)){
//	    return STATE_ERROR;
//	}
	return STATE_SUCCESS;

}





int handle_quit(int client_socket){
	int i;
	protocol_message msg;
			
	msg.header.opcode = OPCODE_QUIT_OK;
	msg.header.data_length = 0;
	if (send_protocol_message(client_socket, &msg)) {
		return STATE_ERROR;
	}
	
	 for (i = 0 ; i < MAX_CLIENTS ; i++){
	    if (client_array[i].socket_fd == client_socket){ //means that's the user that wants to QUIT
		client_array[i].socket_fd = -1; //for reuse	
		memset(client_array[i].username, '\0', MAX_USERNAME+1); //for reuse	
		break;
	    }
	}	
	return STATE_SUCCESS;
}





int check_username_passowrd_exist(int client_sock, const char* data, FILE* fp){
  
    char line[MAX_LINE_LENGTH];
    int i, line_length;
    char copyLine[MAX_LINE_LENGTH];
    char password[51];
    
    
    while (fgets(line, sizeof(line), fp)){
	 if (line[strlen(line)-1] == '\n'){
	  line[strlen(line)-1] = '\0' ;
	 }
	 
	 line_length = strlen(line);
	 if (strncmp(line, data, line_length) == 0){ 	
	     for (i = 0 ; i < MAX_CLIENTS ; i++){
		
		if (client_array[i].socket_fd == client_sock){		 		
		    strcpy(copyLine, line);  
		    memset(client_array[i].username,0,MAX_USERNAME);     
		    sscanf(copyLine, "%s %s", client_array[i].username, password); //set username of the client
		    return STATE_SUCCESS; //found username and password
		}		
	    }
	}
    }
    return STATE_ERROR; //didn't find username or password
}



int command_login(int client_sock, const char* data, FILE* fp){
  
    protocol_message msg;
    memset(msg.data, '\0', MAX_DATA_SIZE);  //ron added
    msg.header.data_length = 0;
    int state_flag = check_username_passowrd_exist(client_sock, data, fp);
    
    if (state_flag == STATE_SUCCESS){
	msg.header.opcode = OPCODE_LOGIN_SUCCEED;
	msg.header.data_length = strnlen(SERVER_LOGIN_SUCCEED_STRING, MAX_DATA_SIZE);
	strncpy(msg.data, SERVER_LOGIN_SUCCEED_STRING, msg.header.data_length);
    }
    else{
      	msg.header.opcode = OPCODE_LOGIN_FAILED;
	msg.header.data_length = strnlen(SERVER_LOGIN_FAILED_STRING, MAX_DATA_SIZE);
	strncpy(msg.data, SERVER_LOGIN_FAILED_STRING, msg.header.data_length);
    }
      
    if (send_protocol_message(client_sock, &msg) != STATE_SUCCESS){
	return STATE_ERROR;
    }else{
	return STATE_SUCCESS;
    }

}




int server_to_client_new(int client_sock, char usersFilePath[]){
  
    protocol_message msg;
    memset(msg.data, '\0', MAX_DATA_SIZE);  
    int state_flag = 0;
    FILE *fp, *fp2;

    msg.header.opcode = OPCODE_WELCOME;
    msg.header.data_length = strnlen(SERVER_WELCOME_STRING, MAX_DATA_SIZE);
    strncpy(msg.data, SERVER_WELCOME_STRING, msg.header.data_length);


    state_flag = recv_protocol_message(client_sock, &msg);
    if (state_flag == STATE_ERROR){
	return STATE_ERROR;
    } else if (state_flag == STATE_SHUTDOWN) {
	return STATE_SHUTDOWN;
    }
    
    switch (msg.header.opcode){
	case OPCODE_LOGIN:	
	fp = fopen(usersFilePath , "r");
	if (fp == NULL){
	    printf("Error in openning users_file: %s\n", strerror(errno));
	    return STATE_ERROR;		  
	}	
	state_flag = command_login(client_sock, msg.data, fp);
	fclose(fp);
	break;
	
	case OPCODE_SHOW_INBOX:
	    state_flag = handle_show_inbox(client_sock);
	    break;
	case OPCODE_GET_MAIL:
	    state_flag = handle_getMail(client_sock, msg.data);
	    break;
	case OPCODE_COMPOSE:
	    state_flag = handle_compose(client_sock, msg.data);
	    break;
	case OPCODE_DELETE_MAIL:
	    state_flag = handle_delete(client_sock, msg.data);
	    break;
	case OPCODE_SHOW_ONLINE_USERS:
	    fp2 = fopen(usersFilePath , "r");
	    if (fp2 == NULL){
		printf("Error in openning users_file: %s\n", strerror(errno));
		return STATE_ERROR;		  
	    }	
	    state_flag = handle_show_online_users(client_sock, fp2);
	    fclose(fp2);
	    break;
	case OPCODE_CHAT_MSG:
	    state_flag = handle_chat_msg(client_sock, msg.data);
	    break;
	case OPCODE_QUIT:
	    state_flag = handle_quit(client_sock);
	    break;

	default:
	    state_flag = STATE_ERROR;
	    break;
    }

    return state_flag;
}



int initiate_server(unsigned short int port, char usersFilePath[]){ 
    struct sockaddr_in server_addr;
    int server_sock; // listening socket descriptor
    int client_sock; // newly accept()ed socket descriptor
    int state_flag;

    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int i, fd, check_select;
    protocol_message msg;
    
    // clear the temp set
    FD_ZERO(&read_fds);

    //initialize client_array[]
    for (i = 0; i < MAX_CLIENTS; i++){
        client_array[i].socket_fd = -1;
    }
    
    //get a socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0){
	printf("Error in socket function: %s\n", strerror(errno));
	return STATE_ERROR;
    }
    
    //bind socket
    memset(&server_addr, '0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);     

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
	close(server_sock);
	printf("Error in bind function: %s\n", strerror(errno));
	return STATE_ERROR;
    }

    //listen on socket
    if (listen(server_sock, 5) < 0){
	close(server_sock);
	printf("Error in listen function: %s\n", strerror(errno));
	return STATE_ERROR;
    }

    //main loop
    //main idea: 1. each time fill the set "read_fds" with all the active clients.
    //(plus the listener). (clients data including fds, is saved in client_array).
    //2. then call "select",
    //this will change the set so that only the fds containing data will stay in it.
    while (1)
    {
        // add the listener to the temp set
        FD_SET(server_sock, &read_fds);

        // keep track of the biggest file descriptor
        fdmax = server_sock; // so far, it's this one


		//copy all active client fd's, into set "read_fds".
        //the set is relevant only for this iteration ("select" might delete fd's).
		for (i = 0 ; i < MAX_CLIENTS ; i++)
		{
			fd = client_array[i].socket_fd;

			if(fd > 0){    //if an active socket descriptor - add to the set
				FD_SET(fd ,&read_fds);
			}

			if(fd > fdmax){	//update max socket fd
				fdmax = fd;
			}
		}


    	//select
		check_select = select(fdmax + 1 , &read_fds , NULL , NULL , NULL);
		if ((check_select < 0) && (errno!=EINTR))
		{
		   close(server_sock);
		   printf("Error in select function: %s\n", strerror(errno));
		   return STATE_ERROR;
		}

		//check for data on listener socket = new client
		if (FD_ISSET(server_sock, &read_fds))
		{
			//TODO for debug add more params to "accept" in order to get client ip/port.
			//details in example
			//accept new client
			client_sock = accept(server_sock, NULL , NULL);

			if (client_sock < 0){
			  printf("Error in accept function: %s\n", strerror(errno));
			}
			else
			{			//accepted new client - success

			   //add the new connected client socket to the client sockets array
				for (i = 0; i < MAX_CLIENTS; i++)
				{
					//TODO handle case when array if full (too many clients)
					if (client_array[i].socket_fd == -1)//if position is empty
					{
						client_array[i].socket_fd = client_sock;
						break;
					}
				}

				memset(msg.data, '\0', MAX_DATA_SIZE);
				state_flag = 0;

				/******  send welcome message to new client!   ******/
				msg.header.opcode = OPCODE_WELCOME;
				msg.header.data_length = strnlen(SERVER_WELCOME_STRING, MAX_DATA_SIZE);
				strncpy(msg.data, SERVER_WELCOME_STRING, msg.header.data_length);

				state_flag = send_protocol_message(client_sock, &msg);

				if (state_flag == STATE_ERROR)
				{ //error - close client sock - check if it's enough
					printf("check for me");
					if (close(client_sock) < 0){
					printf("Error in close function: %s\n", strerror(errno));
					}
					else
					{
						client_array[i].socket_fd = -1 ; //for reuse
						memset(client_array[i].username, '\0', MAX_USERNAME+1);
					}
				}
			} //END accepted new client - success
		}//END check for data on listener socket = new client



		// run through the existing connections looking for data to read
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			fd = client_array[i].socket_fd;

			//skip empty clients
			if (fd==-1)
			{
				continue;
			}

			//data on client socket - handle it
			if (FD_ISSET(fd ,&read_fds))
			{ //means the client sent something to server
				state_flag = server_to_client_new(fd, usersFilePath);
				if (state_flag == STATE_ERROR)
				{
					continue;
					//TODO 1. check if to do more in case of ERROR
					//TODO 2. handle shutdown client (normal quit is already handled)

				}
			}// END data on client socket.
		}// END looping through file descriptors

    }//END main loop

        
        
    if (close(server_sock) == -1){
	printf("Error in close function: %s\n", strerror(errno));
    }
    
    return state_flag;
}



int main(int argc, char** argv){
  
    unsigned short int port = DEFAULT_PORT;
    char usersFilePath[MAX_FILE_PATH_LENGTH+1];
   
    if (argc < 2 || argc > 3){
	printf("Illegal number of arguments\n");
	return STATE_ERROR;
    }
    strncpy(usersFilePath, argv[1], MAX_FILE_PATH_LENGTH);
    
    usersFilePath[MAX_FILE_PATH_LENGTH] = '\0'; 
    
    if (argc == 3){
	port = (unsigned short int) atoi(argv[2]);
    }
    
   
    makeEmptyList ();   // creates an empty list of mails

    //createListForDebug();  	
    
    if (initiate_server(port, usersFilePath) == STATE_ERROR){
	return STATE_ERROR;
    }
    return STATE_SUCCESS;

}
