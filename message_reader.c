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
        perror("Nubmber of arguments insufficient.\nPlease provide 2 arguments. Device path and channel id");
        exit(1);
    }
   /* Enough argumnets given */
    fd = open(argv[1], O_RDONLY);
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
    if ((bytesRead = read(fd, msg, MESSAGE_LEN)) < 0){
        perror("Error reading from device");
        exit(1);
    }
    if (close(fd) < 0){
        perror("Error closing file");
        exit(1);
    }
    if (write(STDOUT_FILENO, msg, bytesRead) < 0){
        perror("Error printing message");
        exit(1);
    }
    return SUCCESS;
}