#include "mail.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>

//create a new node
Node* createNode(mail email, Node* next){
	Node *result;
	result = (Node *)malloc (sizeof(Node));
	result ->email = email;
	result ->next = next;
	return result;
}


void makeEmptyList ()  {
	mail dummyMail;		
	dummyMail.username_length = 4;
	strncpy(dummyMail.username, "head", 4);
	dummy_head = createNode(dummyMail, NULL);
	list_size = 1;  
}


//returns -1 if mail_to_delete wasn't in the list.  otherwise returns 0
int delete_mail(mail mail_to_delete){
	Node *temp;  
	temp = GetMail(mail_to_delete);
	if(temp == NULL)
		return -1;

	temp->email.is_deleted = -88;   //means mail is marked deleted
	return 0;	
	
}


//find_previous_node is called only when we know mail_to_delete is in the linked list
Node* find_previous_node(mail mailToFind){
	Node *temp;  Node *prev;	Node *head = dummy_head;	  

	temp = head->next;   //we know temp!= NULL
	//case where we return the head of the list
	if( strcmp(mailToFind.username, temp->email.username) == 0  &&
		mailToFind.mail_id == temp->email.mail_id )  {
		return head;
	}
	while(temp->next != NULL)  {
		prev = temp;
		temp = temp->next;
		if( strcmp(mailToFind.username, temp->email.username) == 0  &&
			mailToFind.mail_id == temp->email.mail_id )  {
			return prev;
		}
	}
	//check the last node
	if(temp != NULL) {
		if( strcmp(mailToFind.username, temp->email.username) == 0  &&
			mailToFind.mail_id == temp->email.mail_id )  {
			return prev;
		}
	}
	return NULL;
}

/*
void createListForDebug() {   
	mail someMail;
	someMail.username_length = 5;
	strncpy(someMail.username, "moshe", 5);

	memset(someMail.mail_recievers, '\0', sizeof(someMail.mail_recievers));
   	strcpy(someMail.mail_recievers, "moshe,Yoav");
	memset(someMail.subject, '\0', sizeof(someMail.subject));
   	strcpy(someMail.subject, "Funny pictures");
	memset(someMail.mail_sender, '\0', sizeof(someMail.mail_sender));
   	strcpy(someMail.mail_sender, "Yossi");
	memset(someMail.text, '\0', sizeof(someMail.text));
   	strcpy(someMail.text, "How are you ?  Long time no see!");
	addMail(someMail);
	

	someMail.username_length = 4;
	memset(someMail.username, '\0', sizeof(someMail.username));
	strcpy(someMail.username, "Yoav");

	addMail(someMail);


	memset(someMail.mail_recievers, '\0', sizeof(someMail.mail_recievers));
   	strcpy(someMail.mail_recievers, "Yoav");
	memset(someMail.subject, '\0', sizeof(someMail.subject));
   	strcpy(someMail.subject, "Vacation at the Beach");
	memset(someMail.mail_sender, '\0', sizeof(someMail.mail_sender));
   	strcpy(someMail.mail_sender, "moshe");
	memset(someMail.text, '\0', sizeof(someMail.text));
   	strcpy(someMail.text, "Are you still at the beach resort ?   Having loads of fun ?  ");
	
	addMail(someMail);

	int i;
	for(i=0; i< 30; i++) 
		addMail(someMail);
	
	
	memset(someMail.mail_recievers, '\0', sizeof(someMail.mail_recievers));
   	strcpy(someMail.mail_recievers, "Yoav,Kobi,yaakov,Joe");
	memset(someMail.subject, '\0', sizeof(someMail.subject));
   	strcpy(someMail.subject, "Give me my money");
	memset(someMail.mail_sender, '\0', sizeof(someMail.mail_sender));
   	strcpy(someMail.mail_sender, "moshe");
	memset(someMail.text, '\0', sizeof(someMail.text));
   	strcpy(someMail.text, "I need it now!  ");
	
	addMail(someMail);

	int j;
	for(j=0; j< 800; j++) 
		addMail(someMail);
	
	memset(someMail.mail_recievers, '\0', sizeof(someMail.mail_recievers));
   	strcpy(someMail.mail_recievers, "Yoav,Kobi,yaakov,Joe");
	memset(someMail.subject, '\0', sizeof(someMail.subject));
   	strcpy(someMail.subject, "You are a good friend");
	memset(someMail.mail_sender, '\0', sizeof(someMail.mail_sender));
   	strcpy(someMail.mail_sender, "moshe");
	memset(someMail.text, '\0', sizeof(someMail.text));
   	strcpy(someMail.text, "Have a nice day  ");
	
	addMail(someMail);

	int k;
	for(k=0; k< 300; k++) 
		addMail(someMail);
		
}
*/


//find the 1st node (if it exists) that belongs to mailToAdd.username
Node* findUser (mail mailToAdd){
	Node *temp;		Node *head = dummy_head;		
	if(head->next == NULL)  {   //case where list was empty
		return NULL;
	}
	temp = head->next;   //we know temp!= NULL

	if( strcmp(mailToAdd.username, temp->email.username) == 0  )  {
		return temp;
	}
	while(temp->next != NULL)  {
		temp = temp->next;
		if( strcmp(mailToAdd.username, temp->email.username) == 0  )  {
			return temp;
		}
	}
	//check the last node in the list:
	if(temp != NULL) {
		if( strcmp(mailToAdd.username, temp->email.username) == 0  )  {
			return temp;
		}
	}

	return NULL;
}


//returns NULL in case the mail isn't in the list.  otherwise returns the Node that contains the mail
Node* GetMail (mail mailToFind){
	Node *temp;		Node *head = dummy_head;		
	if(head->next == NULL)  {   //case where list was empty
		return NULL;
	}

	temp = head->next;   //we know temp!= NULL
	if( strcmp(mailToFind.username, temp->email.username) == 0  &&
		mailToFind.mail_id == temp->email.mail_id )  {
		return temp;
	}
	while(temp->next != NULL)  {
		temp = temp->next;
		if( strcmp(mailToFind.username, temp->email.username) == 0  &&
			mailToFind.mail_id == temp->email.mail_id )  {
			return temp;
		}
	}
	//check the last node in the list:
	if(temp != NULL) {
		if( strcmp(mailToFind.username, temp->email.username) == 0  &&
			mailToFind.mail_id == temp->email.mail_id )  {
			return temp;
		}
	}
	return NULL;
}



//returns -1 on error,   1 on success
int addMail (mail mailToAdd) {

	Node *temp, *prev;		Node *head = dummy_head;	
	
	if(list_size >= 32001) {  //case where list already contains 32,000 mails,  and therefore we don't add the new mail
		return -1;
	}
 
	if(head->next == NULL)  {   //case where list was empty
		mailToAdd.mail_id = 1;
		head-> next = createNode(mailToAdd, NULL);
		++list_size;
		return 1;
	}
	//find the 1st node (if it exists) that belongs to mailToAdd.username
	temp = findUser(mailToAdd);
	if(temp == NULL)  {   //case where mailToAdd.username isn't in the list
		temp = dummy_head->next;
		mailToAdd.mail_id = 1;
		dummy_head->next = createNode(mailToAdd, temp);
		++list_size;
		return 1;
	}
	else 
	{  //we insert the node after the last node that contains an email in the inbox of the user 
		prev = temp;   
		while(temp->next != NULL){
			prev = temp;  
			temp = temp->next;
			if( strcmp(mailToAdd.username, temp->email.username) != 0  ){
				int tempID = (prev->email.mail_id)+1;
				mailToAdd.mail_id = tempID;
				prev->next = createNode(mailToAdd, temp);
				++list_size;
				return 1;
			}
		}
		//case where the new added mail is the last in the entire list
		int temp_id = temp->email.mail_id + 1;
		mailToAdd.mail_id = temp_id;
		temp->next = createNode(mailToAdd, NULL);   
		++list_size;
		return 1; 
	}

}
//function for debugg
void print_list() {
	Node *temp;

	temp = dummy_head->next;
	while(temp != NULL){
		printf("user = %s\n", temp->email.username);
		printf("mail_id = %d\n", temp->email.mail_id);
		temp = temp->next;
	}
}

void print_node(Node* temp) {
	printf("user = %s\n", temp->email.username);
	printf("mail_id = %d\n", temp->email.mail_id);
}

