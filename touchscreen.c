#include <libudev.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h> 

#include "utils.h"

/**
 * Detects the correct hidraw device that matches the touchscreen's
 * vendor id of oeef and device id of 0005.
 * udev - udev context used to enumerate
 * returns - /dev/?? path to touchscreen's device handle
 */
char* detect(struct udev* udev) {
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *entry;
	struct udev_device *dev,*parent;
	printf("INFO: Detecting hidraw udev devices\n");
	//Enumerate the hidraw systems
	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate,"hidraw");
	udev_enumerate_scan_devices(enumerate);
	//Get device array
	devices = udev_enumerate_get_list_entry(enumerate);
	//Iterate the array
	udev_list_entry_foreach(entry,devices) {
		const char* path;
		//Get device handle
		path = udev_list_entry_get_name(entry);
		printf("INFO: Inspecting path: %s\n",path);
		dev = udev_device_new_from_syspath(udev, path);
		//Get the parent (USB Device)
		parent = udev_device_get_parent_with_subsystem_devtype(dev,"usb","usb_device");
		if (parent == NULL) {
			printf("INFO: Could not find USB Parent of: %s\n",path);
			continue;
		}
		const char* vendor = udev_device_get_sysattr_value(parent,"idVendor");
		const char* id = udev_device_get_sysattr_value(parent, "idProduct");
		printf("INFO: Found device: VID: %s DID: %s\n",vendor,id);
		//Exit when device is found
		if (0 == strcmp(vendor,"0eef") && 0 == strcmp(id,"0005")) {
			//Construct path from sysname
                        const char* name = udev_device_get_sysname(dev);
                        int len = strlen(name)+6;
                        char* dpath = (char*)safe_malloc(len*sizeof(char));
			if (NULL == dpath) {
				return NULL;
			}
			strcpy(dpath,"/dev/");
			strcpy(dpath+5,udev_device_get_sysname(dev));
			printf("INFO: Detected touchscreen at path: %s\n",dpath);
			//Free resources and eject
			udev_enumerate_unref(enumerate);
			//udev_device_unref(parent);
			//udev_device_unref(dev);
			return dpath;
		}
	}
	//Free resources and eject
	udev_enumerate_unref(enumerate);
	//udev_device_unref(parent);
	//udev_device_unref(dev);
	return NULL;
}
/**
 * Clear out the file
 */
void clear(int fp) {
	char buffer[2048];
	size_t ret = 2048;
	while (0 != ret) {
		ret = read(fp,buffer,2048);
		printf("Reading %d:%s\n",ret,strerror(errno));
	}
}

/**
 * Read events from a file pointer
 * fp - file pointer to read from
 */
void events(int fp) {
	char buffer[25];
	//clear(fp);
	while (1) {
		size_t s = read(fp,buffer,25);
		if (s <= 0) {
			continue;
		}
		int i = 0;
		for (i = 0; i < 50; i++) {
			unsigned char u = buffer[i];
			printf("%x ",0xff & u);
		}
		printf("\n");
	}
}
/**
 * Main program
 * argc - number of arguments
 * argv - argument values
 * return - system exit code
 */
int main(int argc,char** argv) {
	printf("INFO: Running touchscreen driver\n");
	struct udev* udev = udev_new();
	if (NULL == udev) {
		fprintf(stderr,"%s","ERROR: Could not load UDEV system.\n");
		return -1;
	}
	char* path = detect(udev);
	if (NULL == path) {
		fprintf(stderr,"%s","ERROR: Could not detect touchscreen device.\n");
		return -2;
	}
	printf("INFO: Opening hidraw device at: %s\n",path);
	int fp = open(path,O_RDONLY);
	if (-1 == fp) {
		fprintf(stderr,"ERROR: Failed to open file: %s with error %s\n",path,strerror(errno));
		return -3;
	}
	events(fp);
	close(fp);
        free(path);
	udev_unref(udev);
	return 0;
}
