#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "message_slot.h"




int main(int argc, char *argv[]){
    int fd, channelId, messageLen;
    if (argc != 4){
        perror("3 Arugmenst need to be enter");
        return -1;
    }
   /* Enough argumnets given */
    fd = open(argv[1], O_WRONLY);
    if (fd < 0){
        perror("Error opening file");
        return -1;
    }
    /* File opened correctly */
    /* TODO: How do we check if its acutally a device */
    /* Parsing channel id */
    channelId = atoi(argv[2]);
    if (ioctl(fd, MSG_SLOT_CHANNEL, channelId) < 0){
        perror("Error setting channel id");
        return -1;
    }
    /* Fiding length of the message given */
    messageLen = strlen(argv[3]);
    if (write(fd, argv[3], messageLen) < 0){
        perror("Error writing to device");
        return -1;
    }
    if (close(fd) < 0){
        perror("Error closing file");
        return -1;
    }
    return 0;
}