#!/bin/sh

set -x
BIN_DIR=/usr/local/bin

#cd /root/litao/compile/dcl
make com && make $1 && scp dcl-$1/src/$1 root@$2:$BIN_DIR/$1.tmp

ssh root@$2 <<EOF
killall $1 && rm -rf $BIN_DIR/$1
mv $BIN_DIR/$1.tmp $BIN_DIR/$1 && chmod +x $BIN_DIR/$1
EOF
#mv $BIN_DIR/$1.tmp $BIN_DIR/$1 && chmod +x $BIN_DIR/$1 && /usr/local/bin/$1
