
ALL: cfu

CC = gcc

CFLAGS =  -g -O3 -Wall -Werror -I../../include
CPPFLSGS = 
LDFLAGS = -L../../lib -lyaml -lpcap -lpthread -ljson -lxml2 -lprotobuf-c

SRCS = $(wildcard *.c)
OBJECT = $(SRCS:%.c=%.o)
MACROS = -D VERSION=$(VERSION_STR)

#拼接版本号
VER = $(shell cat ../package/cfu-info.yaml | grep " version:" | cut -b 12-15)
ifneq ($(shell git status | grep "to be committed:\|Untracked files:\|not updated:"),)
	COMMIT_SEQ = $(shell git log --pretty=oneline | wc -l)M
else
	COMMIT_SEQ = $(shell git log --pretty=oneline | wc -l)
endif
COMMIT_ID = $(shell git rev-list HEAD -n 1 | cut -c 1-6)
VERSION_STR = \"$(VER).$(COMMIT_SEQ)-$(COMMIT_ID)\"

COMM-SRCS = $(wildcard ../../common/*.c)
COMM-OBJECT = $(COMM-SRCS:%.c=%.o)

cfu : $(OBJECT)
	$(CC) -o cfu $(OBJECT) $(COMM-OBJECT) $(LDFLAGS)

%.o : %.c 
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(MACROS) $< 

clean:
	rm -f *.o cfu

