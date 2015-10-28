/*
 * touchscreen.h
 *
 *  Created on: Oct 27, 2015
 *      Author: lestarch
 */

#ifndef _TOUCHSCREEN_H_
#define _TOUCHSCREEN_H_

/**
 * TouchEvent struct
 */
typedef struct {
	char header;
	unsigned char click;
	short x;
	short y;
} TouchEvent;

/**
 * Detects the correct hidraw device that matches the touchscreen's
 * vendor id of 0eef and device id of 0005.
 * struct udev* udev - udev context used to enumerate
 * return - /dev/?? path to touchscreen's device handle
 */
char* detect(struct udev* udev);

/**
 * Sets up the uinput device
 * int ui - uinput file pointer
 */
int setup(int ui);

/**
 * Read events from a file pointer and write to another pointer
 * int fp - file pointer to read from
 * int ui - uinput file pointer
 */
void events(int fp,int ui)
;
/**
 * Parse an event record and emit touch event struct
 * char* record - record byte array to parse
 * return - touch event struct pointer (caller must free it)
 */
TouchEvent* parseEvent(char* record);

/**
 * Create a list of input events based on the current event and the last event
 * TouchEvent* latest - most recent event
 * TouchEvent* previous - previously detected event
 */
struct input_event* currentEvents(TouchEvent* latest,TouchEvent* previous);

#endif /* _TOUCHSCREEN_H_ */
