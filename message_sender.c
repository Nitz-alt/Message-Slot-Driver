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
        perror("Nubmber of arguments insufficient.\nPlease provide 3 arguments. Device path and channel id and message");
        exit(1);
    }
   /* Enough argumnets given */
    fd = open(argv[1], O_WRONLY);
    if (fd < 0){
        perror("Error opening file");
        exit(1);
    }
    /* File opened correctly */
    /* Parsing channel id */
    channelId = atoi(argv[2]);
    if (ioctl(fd, MSG_SLOT_CHANNEL, channelId) < 0){
        perror("Error setting channel id");
        exit(1);
    }
    /* Fiding length of the message given */
    messageLen = strlen(argv[3]);
    if (write(fd, argv[3], messageLen) < 0){
        perror("Error writing to device");
        exit(1);
    }
    if (close(fd) < 0){
        perror("Error closing file");
        exit(1);
    }
    return SUCCESS;
}