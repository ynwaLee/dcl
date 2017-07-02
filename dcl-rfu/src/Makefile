
src_file = $(wildcard ./*.c)
src_file += ../../common/applog.c
src_file += ../../common/ma_pkt.c
src_file += ../../common/daemon.c
src_file += ../../common/sock_cli.c
src_file += ../../common/conf.c
src_file += ../../common/util-strlcatu.c
src_file += ../../common/util-strlcpyu.c

MACROS = -D VERSION=$(VERSION_STR)

VER = $(shell cat ../package/rfu-info.yaml | grep " version:" | cut -b 12-15)
ifneq ($(shell git status | grep "to be committed:\|Untracked files:\|not updated:"),)
	COMMIT_SEQ = $(shell git log --pretty=oneline | wc -l)M
else
	COMMIT_SEQ = $(shell git log --pretty=oneline | wc -l)
endif
COMMIT_ID = $(shell git rev-list HEAD -n 1 | cut -c 1-6)
VERSION_STR = \"$(VER).$(COMMIT_SEQ)-$(COMMIT_ID)\"

#flag = -L../lib/ -lpthread -lxml2 -Werror -Wall
flag = -L../../lib -lpthread -lxml2 -lyaml -lcurl -ljson -Werror -Wall -I../../include -g

rfu: $(src_file)
	gcc -g $(src_file) $(flag) $(MACROS) -o rfu

clean:
	rm -rf *.o rfu



