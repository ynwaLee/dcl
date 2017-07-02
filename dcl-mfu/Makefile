
VER = $(shell cat package/mfu-info.yaml | grep " version:" | cut -b 12-15)
BUILD    = $(shell git log | grep -cE 'Author:')
BUILDSHA = $(shell git rev-parse --short HEAD)

BSTR = $(shell svnversion)

ALL: package

build:
	make -C src

clean:
	make -C src clean
	rm -f package/*.tar.gz package/mfu.yaml package/bin/* package/etc/*

package: src/mfu
	if [ ! -d "package/bin" ]; then mkdir -p "package/bin"; fi
	if [ ! -d "package/lib" ]; then mkdir -p "package/lib"; fi
	if [ ! -d "package/etc" ]; then mkdir -p "package/etc"; fi
	cp src/mfu package/bin/
	cp etc/* package/etc/
	awk '($$2== "BUILDSTR") gsub("BUILDSTR","$(BSTR)")' package/mfu-info.yaml > package/mfu.yaml
	cd package && tar cpfz mfu-$(VER).$(BSTR).tar.gz bin etc lib Makefile mfu.yaml


