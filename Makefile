touchscreen-driver:	touchscreen.c utils.h utils.c
	gcc -ludev -lc -I. touchscreen.c utils.c -o touchscreen-driver
debug:
	gcc -ludev -lc -g -I. touchscreen.c utils.c -o touchscreen-driver
clean:
	rm -f touchscreen-driver
