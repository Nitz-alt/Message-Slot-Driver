#ifndef MESSAGE_SLOT_H

#define MESSAGE_SLOT_H
#define SUCCESS 0
#define DEVICE_RANGE_NAME "message_slot"
#define MESSAGE_LEN 128
#define DEVICE_FILE_NAME "message_device"
#define MAJOR_NUM 235
#define MAX_CHANNELS 0x100000
#define MAX_DEVICES 256

/* IOCTL COMMAND MACRO */
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

#endif