
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "messages.h" 

int send_protocol_message(int socket, protocol_message* msg){
    /* returns STATE_SUCCESS if message was sent succesfully, STATE_ERROR on error */
    
    int msg_length = HEADER_SIZE + msg->header.data_length;
    int num_bytes_left = msg_length;
    int total_bytes_sent = 0;
    int num_bytes_sent_now = 0;
    protocol_message m;
	
    m.header.opcode = htons(msg->header.opcode);
    m.header.data_length = htons(msg->header.data_length);
    memcpy(m.data, msg->data, msg->header.data_length);	  

    while (total_bytes_sent < msg_length){
	num_bytes_sent_now = send(socket, &m+total_bytes_sent, num_bytes_left, 0);
	if (num_bytes_sent_now < 0){
	    printf("Error in send function: %s\n", strerror(errno));
	    return STATE_ERROR;
	}
	total_bytes_sent += num_bytes_sent_now;
	num_bytes_left -= num_bytes_sent_now;
    }
     
    
    return STATE_SUCCESS;
}

int recv_buffer(int socket, void* buff, int len){
  /* returns STATE_SUCCESS if message received, STATE_ERROR on error,  STATE_SHUTDOWN if the other side was shut down */
  
    int total_bytes_sent = 0;
    int num_bytes_left = len;
    int num_bytes_recv_now;
    
    while (total_bytes_sent < len){
	num_bytes_recv_now = recv(socket, (void*)((long)buff + total_bytes_sent), num_bytes_left, 0);
	if (num_bytes_recv_now < 0) { // error
	    printf("Error in recv function: %s\n", strerror(errno));
	    return STATE_ERROR;
	} else if (num_bytes_recv_now == 0) {  // other side performed a shutdown
	    return STATE_SHUTDOWN;
	}
	 total_bytes_sent += num_bytes_recv_now;
	num_bytes_left -= num_bytes_recv_now;
    }
    return STATE_SUCCESS;
}


int recv_protocol_message(int socket, protocol_message* msg){
    
    int state_flag = recv_buffer(socket, &msg->header, HEADER_SIZE);
    if (state_flag != STATE_SUCCESS){
	return state_flag;
    }
    msg->header.opcode = ntohs(msg->header.opcode);
    msg->header.data_length = ntohs(msg->header.data_length);

	
    if (msg->header.data_length > MAX_DATA_SIZE){
	printf("Error: header.length is too long\n");
	return STATE_ERROR;
    }
    
    memset(msg->data, '\0', MAX_DATA_SIZE);
    state_flag = recv_buffer(socket, msg->data, msg->header.data_length);

    return state_flag;
}

