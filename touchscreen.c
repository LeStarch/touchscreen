#include <libudev.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h> 
#include <linux/input.h>
#include <linux/uinput.h>

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
 * Sets up the uinput device
 * ui - uinput file pointer
 */
int setup(int ui) {
	//int ret = ioctl(ui, UI_SET_EVBIT,EV_KEY);
        //if (ret == -1) {
	//	return ret;
	//} 
	int ret = ioctl(ui, UI_SET_EVBIT,EV_SYN);
        if (ret == -1) {
		return ret;
	}
	ret = ioctl(ui, UI_SET_EVBIT,EV_ABS);
        if (ret == -1) {
		return ret;
	}
	ret = ioctl(ui, UI_SET_ABSBIT, ABS_X);
        if (ret == -1) {
		return ret;
	}
	ret = ioctl(ui, UI_SET_ABSBIT, ABS_Y);
        if (ret == -1) {
		return ret;
	}
	struct uinput_user_dev uidev;
	memset(&uidev, 0, sizeof(uidev));
	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "touchscreen-driver");
	uidev.id.bustype = BUS_USB;
	uidev.id.vendor  = 0xdead;
	uidev.id.product = 0xbeef;
	uidev.id.version = 1;
	uidev.absmin[ABS_X] = 0;
	uidev.absmax[ABS_X] = 1023;
	uidev.absmin[ABS_Y] = 0;
	uidev.absmax[ABS_Y] = 600;
	ret = write(ui, &uidev, sizeof(uidev));
        if (ret == -1) {
		return ret;
	}
	ret = ioctl(ui, UI_DEV_CREATE);
        if (ret == -1) {
		return ret;
	}
 
}
/**
 * Read events from a file pointer
 * fp - file pointer to read from
 * ui - uinput file pointer
 */
void events(int fp,int ui) {
	char buffer[25];
	while (1) {
		size_t s = read(fp,buffer,25);
		if (s <= 0 || buffer[0] != 0xaa) {
			continue;
		}
		struct input_event ev[2];
		memset(&ev, 0, sizeof(ev));
		ev[0].type = EV_ABS;
		ev[0].code = ABS_X;
		ev[0].value = *((short*)(buffer+2));
		ev[1].type = EV_ABS;
		ev[1].code = ABS_Y;
		ev[1].value = *((short*)(buffer+4));
		write(ui, &ev, sizeof(ev));
		
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
	if (0 > fp) {
		fprintf(stderr,"ERROR: Failed to open file: %s with error %s\n",path,strerror(errno));
		return -3;
	}
	int ui = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if (0 > ui) {
		fprintf(stderr,"ERROR: Failed to open file: /dev/uinput with error %s\n",strerror(errno));
		return -4;
	}
	int ret = setup(ui);
	if (ret < 0) {
		fprintf(stderr,"ERROR: Failed to setup uinput with error: %s\n",strerror(errno));
		return -5;
	}
	events(fp,ui);
	close(fp);
        free(path);
	udev_unref(udev);
	return 0;
}
