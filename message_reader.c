#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "message_slot.h"




int main(int argc, char *argv[]){
    char msg[MESSAGE_LEN];
    int fd, channelId, bytesRead;
    if (argc != 3){
        perror("3 Arugmenst need to be enter");
        return -1;
    }
   /* Enough argumnets given */
    fd = open(argv[1], O_RDONLY);
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
    if ((bytesRead = read(fd, msg, MESSAGE_LEN)) < 0){
        perror("Error reading from device");
        return -1;
    }
    if (close(fd) < 0){
        perror("Error closing file");
        return -1;
    }
    if (write(STDOUT_FILENO, msg, bytesRead) < bytesRead){
        perror("Error writing message");
        return -1;
    }
    return 0;
}