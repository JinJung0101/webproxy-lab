# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread
all: echoclient echoserveri
csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c
echoclient.o: echoclient.c csapp.h
	$(CC) $(CFLAGS) -c echoclient.c
echoserveri.o: echoserveri.c csapp.h
	$(CC) $(CFLAGS) -c echoserveri.c
echoclient: echoclient.o csapp.o
	$(CC) $(CFLAGS) echoclient.o csapp.o -o echoclient $(LDFLAGS)
echoserveri: echoserveri.o csapp.o
	$(CC) $(CFLAGS) echoserveri.o csapp.o -o echoserveri $(LDFLAGS)
# Creates a tarball in ../echoclientlab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-echoclientlab-handin.tar echoclientlab-handout --exclude tiny --exclude nop-server.py --exclude echoclient --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")
	
clean:
	rm -f *~ *.o echoclient echoserveri core *.tar *.zip *.gzip *.bzip *.gz