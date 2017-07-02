
install:
	if [ ! -d "/usr/local/etc/rfu" ]; then mkdir "/usr/local/etc/rfu"; fi
	cp -f etc/* /usr/local/etc/rfu/
	if [ ! -d "/usr/local/etc/software" ]; then mkdir "/usr/local/etc/software"; fi
	cp -f rfu.yaml /usr/local/etc/software/rfu.yaml
	chmod +x bin/*
	cp -f bin/* /usr/local/bin/
	
update:
	chmod +x bin/*
	cp -f bin/* /usr/local/bin/
	cp -f rfu.yaml /usr/local/etc/software/rfu.yaml

uninstall:
	rm -f /usr/local/bin/rfu
	rm -rf /usr/local/etc/rfu/
	rm -f /usr/local/etc/software/rfu.yaml



