#ifndef MESSAGES_H_
#define MESSAGES_H_

#include "protocol.h"
#include "constants.h"

int send_protocol_message(int socket, protocol_message* msg);
int recv_buffer(int socket, void* buff, int len);
int recv_protocol_message(int socket, protocol_message* msg);



#endif
