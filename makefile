CFLAGS=-ansi -pedantic-errors -Wall -g -std=c99 -D_GNU_SOURCE

all: mail_server mail_client
	

mail_client: mail_client.o messages.o
	gcc $(CFLAGS) -g mail_client.o messages.o -o mail_client

mail_server: mail_server.o mail.o messages.o
	gcc $(CFLAGS) -g mail_server.o mail.o messages.o -o mail_server

mail_client.o: mail_client.c messages.h
	gcc $(CFLAGS) -c mail_client.c

mail_server.o: mail_server.c mail.h messages.h
	gcc $(CFLAGS) -c mail_server.c

messages.o: messages.c messages.h constants.h protocol.h 
	gcc $(CFLAGS) -c messages.c

mail.o: mail.c mail.h
	gcc $(CFLAGS) -c mail.c
clean:
	rm -f *.o mail_server mail_client
