#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#define SERVER_WELCOME_STRING "Welcome! I am simple-mail server."
#define SERVER_LOGIN_SUCCEED_STRING "Connected to server"
#define SERVER_LOGIN_FAILED_STRING "Login failed - wrong username or password" 
#define MAX_WELCOME_LENGTH 1024
#define MAX_MESSAGE_SIZE 4096
#define HEADER_SIZE (sizeof(protocol_opcode))
#define MAX_DATA_SIZE (MAX_MESSAGE_SIZE - HEADER_SIZE)

#pragma pack(push, 1)

typedef struct{
    short opcode;
    short data_length;
} protocol_opcode; 


typedef struct{
    protocol_opcode header;
    char data[MAX_DATA_SIZE];
} protocol_message;


typedef struct{
    int socket_fd;
    char username[51];
} user_details; 



#pragma pack(pop)

typedef enum{
	
	OPCODE_LOGIN = 0x00,
	OPCODE_SHOW_INBOX = 0x10,
	OPCODE_GET_MAIL = 0x20,
	OPCODE_DELETE_MAIL = 0x30,
	OPCODE_COMPOSE = 0x40,
	OPCODE_QUIT = 0x50,
	OPCODE_QUIT_OK = 0x60,
	OPCODE_SHOW_ONLINE_USERS = 0x70,
	OPCODE_CHAT_MSG = 0xB0,

	
	OPCODE_WELCOME = 0x80,
	OPCODE_LOGIN_SUCCEED = 0x90,
	OPCODE_LOGIN_FAILED = 0xA0,

	OPCODE_FROM = 0x21,
	OPCODE_TO = 0x22,
	OPCODE_SUBJECT = 0x23,
	OPCODE_TEXT = 0x24,
	OPCODE_MAIL_ID = 0x25,
	OPCODE_SENDER = 0x26,
	OPCODE_MAIL_NOT_FOUND = 0x27,
	OPCODE_MAIL_DELETED = 0x28,
	OPCODE_COMPOSE_SUCCEED = 0x29,
	OPCODE_COMPOSE_FAILED = 0x30,
	OPCODE_SHOW_INBOX_END = 0x31

} opcode;

#endif
