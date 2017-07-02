
#ALL:  

CC = gcc

CFLAGS =  -g -O3 -Wall -Werror -I../include
CPPFLSGS = 
LDFLAGS = -lyaml -lpcap -lpthread

SRCS = $(wildcard *.c)
OBJECT = $(SRCS:%.c=%.o)

ALL: $(OBJECT)

%.o : %.c 
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< 

clean:
	rm -f *.o

