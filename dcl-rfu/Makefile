
VER = $(shell cat package/rfu-info.yaml | grep " version:" | cut -b 12-15)
BUILD    = $(shell git log | grep -cE 'Author:')
BUILDSHA = $(shell git rev-parse --short HEAD)

BSTR = $(shell svnversion)


build:
	make -C src

clean:
	make -C src clean
	rm -f package/*.tar.gz package/rfu.yaml package/bin/* package/etc/*

package: src/rfu
	if [ ! -d "package/bin" ]; then mkdir -p "package/bin"; fi
	if [ ! -d "package/lib" ]; then mkdir -p "package/lib"; fi
	if [ ! -d "package/etc" ]; then mkdir -p "package/etc"; fi
	cp src/rfu package/bin/
	cp etc/* package/etc/
	awk '($$2== "BUILDSTR") gsub("BUILDSTR","$(BSTR)")' package/rfu-info.yaml > package/rfu.yaml
	cd package && tar cpfz rfu-$(VER).$(BSTR).tar.gz bin etc lib Makefile rfu.yaml
