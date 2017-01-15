#ifndef MAIL_H_
#define MAIL_H_

#define MAX_USERNAME_SIZE 50
#define MAX_RECIEVERS 20   // an email can be sent to up to 20 clients
#define MAX_SUBJECT_SIZE 100
#define MAX_TEXT_SIZE 2000

#define DEBUG_LIST 1002

#pragma pack(push, 1)

typedef struct mail{
	char username[MAX_USERNAME_SIZE+1];
	short username_length;  //length of username.  For example for moshe the length is 5
	short mail_id;  // an integer between 1 and 32,000
	char mail_sender[MAX_USERNAME_SIZE + 1];
	short mail_sender_username_length;
	char mail_recievers[MAX_RECIEVERS * MAX_USERNAME_SIZE +19 +1];   //19 for the ","  ,  1 for \0.  no spaces allowed
	char subject[MAX_SUBJECT_SIZE + 1];
	char text[MAX_TEXT_SIZE + 1];
	short subject_length;  //an integer between 0 and 100
	short text_length;  //an integer between 0 and 2000
	short num_of_recievers;  //an integer between 1 and 20
	int is_deleted;  //  -88 represents deleted mail.  Used in GetMail function in mail.c
} mail;


typedef struct node {
	struct mail email;
	struct node *next;
} Node;



typedef struct list {
	Node* head;
	short list_size;   // an integer between 1 and 32,001   (the extra 1 is for the dummy head)
} List;

#pragma pack(pop)


Node *dummy_head;  //dummy head for our mail list
int list_size;  //an int between 1 and 32,001   (the extra 1 is for the dummy head of the mail list)

void makeEmptyList ();
Node* createNode(mail email, Node* next);
Node* findUser (mail mailToAdd);
Node* GetMail (mail mailToFind);
int addMail (mail mailToAdd);
void print_list();
void print_node(Node* temp);
int delete_mail(mail mail_to_delete);
Node* find_previous_node(mail mail_to_delete);
//void createListForDebug();  

#endif
