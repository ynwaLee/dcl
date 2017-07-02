
install:
	if [ ! -d "/usr/local/etc/mfa" ]; then mkdir "/usr/local/etc/mfa"; fi
	cp -f etc/* /usr/local/etc/mfa/
	cp -f mfa.yaml /usr/local/etc/software/mfa.yaml
	cp -f bin/* /usr/local/bin/

update:
	cp -f bin/* /usr/local/bin/
	cp -f mfa.yaml /usr/local/etc/software/mfa.yaml

uninstall:
	rm -f /usr/local/bin/mfa
	rm -rf /usr/local/etc/mfa
	rm -f /usr/local/etc/software/mfa.yaml

