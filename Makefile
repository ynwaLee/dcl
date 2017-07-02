com:
	cd lib/ && make unzip
	cd common/ && make
rfu:
	cd dcl-rfu && make build

cfu:
	cd dcl-cfu && make build

mfa:
	cd dcl-mfa && make build

mfu:
	cd dcl-mfu && make build

clean:
	cd ./common && make clean && cd ..
	cd ./dcl-rfu && make clean && cd ..
	cd ./dcl-cfu && make clean && cd ..
	cd ./dcl-mfa && make clean && cd ..
	cd ./dcl-mfu && make clean && cd ..
	cd ./lib && make clean
