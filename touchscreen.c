#include <libudev.h>

#include <stdlib.h>
#include <stdio.h>

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
		dev = udev_device_new_from_syspath(udev, path);
		parent = udev_device_get_parent_with_subsystem_devtype(dev,"usb","usb_device");
		if (parent == NULL) {
			printf("INFO: Could not find USB Parent of: %s\n",path);
			continue;
		}
		const char* vendor = udev_device_get_sysattr_value(dev,"idVendor");
		const char* id = udev_device_get_sysattr_value(dev, "idProduct");
		printf("INFO: Found device: VID: %s DID: %s\n",vendor,id);
	    //Exit when device is found
		if (strcmp(vendor,"0EEF") && strcmp(id,"0005")) {
			udev_enumerate_unref(enumerate);
			return dev;
		}
	}
	udev_enumerate_unref(enumerate);
	return NULL;
}

/**
 * Main program
 */
int main(int argc,char** argv) {
	printf("INFO: Running touchscreen driver\n");
	struct udev* udev = udev_new();
	if (udev == NULL) {
		printf("%s","ERROR: Could not load UDEV system.\n");
		return -1;
	}
	struct udev_device *dev = detect(udev);
	if (dev == NULL) {
		fprintf(stderr,"ERROR: Could not load touchscreen device.\n");
		return -2;
	}
	udev_unref(udev);
	return -3;
}
