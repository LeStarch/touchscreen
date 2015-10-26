#include <libudev.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/**
 * Detects the correct hidraw device
 */
struct udev_device* detect(struct udev* udev) {
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
			printf("INFO: Detected touchscreen at path: %s\n",path);
			udev_enumerate_unref(enumerate);
			return dev;
		}
	}
	udev_enumerate_unref(enumerate);
	return NULL;
}
/**
 * Read events from a file
 */
void events(FILE* fp) {
	char buffer[50];
	while (1) {
		char* ret = fgets(buffer,25,fp);
		if (NULL != ret) {
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
 */
int main(int argc,char** argv) {
	printf("INFO: Running touchscreen driver\n");
	struct udev* udev = udev_new();
	if (NULL == udev) {
		printf("%s","ERROR: Could not load UDEV system.\n");
		return -1;
	}
	struct udev_device *dev = detect(udev);
	if (NULL == dev) {
		fprintf(stderr,"ERROR: Could not load touchscreen device.\n");
		return -2;
	}
	const char* path = udev_device_get_syspath(dev);
	FILE* fp = fopen(path,"rb");
	if (NULL == fp) {
		fprintf(stderr,"ERROR: Failed to open file: %s with error %s\n",path,strerror(errno));
		return -1;
	}
	events(fp);
	close(fp);
	udev_unref(udev);
	return -3;
}
