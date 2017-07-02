

VER = $(shell cat package/cfu-info.yaml | grep " version:" | cut -b 12-15)

BSTR = $(shell svnversion)

ALL: package

build:
	make -C src

clean:
	make -C src clean
	rm -f package/*.tar.gz package/cfu.yaml package/bin/* package/etc/*

package: src/cfu
	if [ ! -d "package/bin" ]; then mkdir -p "package/bin"; fi
	if [ ! -d "package/lib" ]; then mkdir -p "package/lib"; fi
	if [ ! -d "package/etc" ]; then mkdir -p "package/etc"; fi
	cp src/cfu package/bin/
	cp etc/* package/etc/
	awk '($$2== "BUILDSTR") gsub("BUILDSTR","$(BSTR)")' package/cfu-info.yaml > package/cfu.yaml
	cd package && tar cpfz cfu-$(VER).$(BSTR).tar.gz bin etc lib Makefile cfu.yaml


