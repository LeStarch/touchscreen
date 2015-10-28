touchscreen-driver:	touchscreen.h touchscreen.c utils.h utils.c
	gcc -I. touchscreen.c utils.c -ludev -lc -o touchscreen-driver
debug:
	gcc -I. touchscreen.c utils.c -ludev -lc -g -o touchscreen-driver-debug
clean:
	rm -f touchscreen-driver touchscreen-driver-debug
