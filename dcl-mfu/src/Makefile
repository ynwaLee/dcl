
ALL: mfu 

CC = gcc

INC = -I../../include
CFLAGS =  -g -O3 -Wall -Werror
CPPFLSGS = 
LDFLAGS = -L../../lib -lyaml -lpcap -lpthread -ljson

SRCS = $(wildcard ./*.c)
OBJECT = $(SRCS:%.c=%.o)
MACROS = -D VERSION=$(VERSION_STR)

VER = $(shell cat ../package/mfu-info.yaml | grep " version:" | cut -b 12-15)
ifneq ($(shell git status | grep "to be committed:\|Untracked files:\|not updated:"),)
	COMMIT_SEQ = $(shell git log --pretty=oneline | wc -l)M
else
	COMMIT_SEQ = $(shell git log --pretty=oneline | wc -l)
endif
COMMIT_ID = $(shell git rev-list HEAD -n 1 | cut -c 1-6)
VERSION_STR = \"$(VER).$(COMMIT_SEQ)-$(COMMIT_ID)\"

COMM-SRCS = $(wildcard ../../common/*.c)
COMM-OBJECT = $(COMM-SRCS:%.c=%.o)

mfu : $(OBJECT)
	$(CC) -o mfu $(OBJECT) $(COMM-OBJECT) $(LDFLAGS) $(INC)

%.o : %.c 
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< $(INC) $(MACROS)

clean:
	rm -rf *.o mfu
