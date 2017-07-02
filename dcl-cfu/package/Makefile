
install:
	if [ ! -d "/usr/local/etc/cfu" ]; then mkdir "/usr/local/etc/cfu"; fi
	cp -f etc/* /usr/local/etc/cfu/
	cp -f cfu.yaml /usr/local/etc/software/cfu.yaml
	cp -f bin/* /usr/local/bin/

update:
	cp -f bin/* /usr/local/bin/
	cp -f cfu.yaml /usr/local/etc/software/cfu.yaml

uninstall:
	rm -f /usr/local/bin/cfu
	rm -rf /usr/local/etc/cfu
	rm -f /usr/local/etc/software/cfu.yaml

