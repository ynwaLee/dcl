
install:
	if [ ! -d "/usr/local/etc/mfu" ]; then mkdir "/usr/local/etc/mfu"; fi
	cp -f etc/* /usr/local/etc/mfu/
	cp -f mfu.yaml /usr/local/etc/software/mfu.yaml
	cp -f bin/* /usr/local/bin/

update:
	cp -f bin/* /usr/local/bin/
	cp -f mfu.yaml /usr/local/etc/software/mfu.yaml

uninstall:
	rm -f /usr/local/bin/mfu
	rm -rf /usr/local/etc/mfu
	rm -f /usr/local/etc/software/mfu.yaml

