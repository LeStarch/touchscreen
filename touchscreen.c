#include <libudev.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h> 
#include <linux/input.h>
#include <linux/uinput.h>
#include <arpa/inet.h>

#include "touchscreen.h"
#include "utils.h"

/**
 * Detects the correct hidraw device that matches the touchscreen's
 * vendor id of 0eef and device id of 0005.
 * struct udev* udev - udev context used to enumerate
 * return - /dev/?? path to touchscreen's device handle
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
 * int ui - uinput file pointer
 */
int setup(int ui) {
    //Setup left button
    int ret = ioctl(ui, UI_SET_EVBIT,EV_KEY);
    if (ret == -1) {
        return ret;
    }
    ret = ioctl(ui, UI_SET_KEYBIT,BTN_LEFT);
    if (ret == -1) {
        return ret;
    }
    //Setup touch events
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
    //Set synch
    ret = ioctl(ui, UI_SET_EVBIT,EV_SYN);
    if (ret == -1) {
        return ret;
    }
    //Setup uinput user device
    struct uinput_user_dev uidev;
    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "touchscreen-lestarch");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor  = 0xdead;
    uidev.id.product = 0xbeef;
    uidev.id.version = 1;
    //Setup the ABS bounds
    uidev.absmin[ABS_X] = 0;
    uidev.absmax[ABS_X] = WIDTH-1;
    uidev.absmin[ABS_Y] = 0;
    uidev.absmax[ABS_Y] = HEIGHT-1;
    //Write out the file
    ret = write(ui, &uidev, sizeof(uidev));
    if (ret == -1) {
        return ret;
    }
    //Create device
    ret = ioctl(ui, UI_DEV_CREATE);
    if (ret == -1) {
        return ret;
    }
    return 0;
}
/**
 * Read events from a file pointer and write to another pointer
 * int fp - file pointer to read from
 * int ui - uinput file pointer
 */
void events(int fp,int ui) {
    char buffer[25];
    TouchEvent* current = NULL;
    TouchEvent* previous = safe_malloc(sizeof(TouchEvent));
    memset(previous,0,sizeof(TouchEvent));
    while (1) {
        printf("INFO: Attempting to read hidraw.\n");
        //Read and process or ignore
        if (read(fp,buffer,25) <= 0 || (current = parseEvent(buffer)) == NULL) {
            printf("INFO: Failed to read or parse event\n");
            continue;
        }
        printf("DEBUG: Read event at (%d,%d) clicked: %d\n",current->x,current->y,current->click);
        int count = eventCount(current,previous);
        printf("INFO: Attepting to write %d events to uinput stream\n",count);
        //Get events and write them out
        struct input_event* events = currentEvents(current,previous);
        write(ui, events, sizeof(struct input_event)*count);
        //Update pointers
        free(previous);
        previous = current;
    }
}
/**
 * Get the count of the events
 * TouchEvent* latest - latest event
 * TouchEvent* previous - previous event
 * return - count of the number of events
 */
int eventCount(TouchEvent* latest, TouchEvent* previous) {
    return (latest->click << 1) + (latest->click != previous->click);
}
/**
 * Parse an event record and emit touch event struct
 * char* record - record byte array to parse
 * return - touch event struct pointer (caller must free it)
 */
TouchEvent* parseEvent(char* record) {
    TouchEvent* te = (TouchEvent*) safe_malloc(sizeof(TouchEvent));
    if (te == NULL) {
        return te;
    }
    memcpy(te,record,sizeof(TouchEvent));
    if (te->header != 0xaa) {
        free(te);
        return NULL;
    }
    te->x = ntohs(te->x);
    te->y = ntohs(te->y);
    return te;
}
/**
 * Create a list of input events based on the current event and the last event
 * TouchEvent* latest - most recent event
 * TouchEvent* previous - previously detected event
 */
struct input_event* currentEvents(TouchEvent* latest,TouchEvent* previous) {
    int count = eventCount(latest,previous);
    if (count == 0) {
        return NULL;
    }
    struct input_event* events = safe_malloc(count*sizeof(struct input_event));
    memset(events, 0, sizeof(events));
    //Construct the latest event
    events[count-1].type = EV_KEY;
    events[count-1].code = BTN_LEFT;
    events[count-1].value = latest->click;
    if (count < 2) {
        return events;
    } 
    events[0].type = EV_ABS;
    events[0].code = ABS_X;
    events[0].value = latest->x;
    events[1].type = EV_ABS;
    events[1].code = ABS_Y;
    events[1].value = latest->y;
    return events;
}

/**
 * Main program
 * int argc - number of arguments
 * char** argv - argument values
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
    printf("INFO: Opening uinput device: %s\n","/dev/uinput");
    int ui = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (0 > ui) {
        fprintf(stderr,"ERROR: Failed to open file: /dev/uinput with error %s\n",strerror(errno));
        return -4;
    }
    printf("INFO: Setting up uinput device at: %s\n","/dev/uinput");
    int ret = setup(ui);
    if (ret < 0) {
        fprintf(stderr,"ERROR: Failed to setup uinput with error: %s\n",strerror(errno));
        return -5;
    }
    printf("%s","INFO: Detecting events.\n");
    events(fp,ui);
    close(fp);
    free(path);
    udev_unref(udev);
    return 0;
}
