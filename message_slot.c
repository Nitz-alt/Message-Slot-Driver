#undef __KERNAL__
#define __KERNAL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/errno.h>


/* Our header file */
#include "message_slot.h"



MODULE_LICENSE("GPL");

struct channel{
    unsigned long id;
    int messageSize;
    char MESSAGE[MESSAGE_LEN];
};

struct arrayBoundries{
    int iNum;
    int aSize;
};


static struct channel *DEVICES[MAX_DEVICES];
static struct arrayBoundries BOUNDRIES[MAX_DEVICES];

static int device_open(struct inode *inode, struct file *fp){
    /* Check if the minor memory is allocated already. Otherwise allocate space for it*/
    int minor = iminor(inode);
    int *data;
    if (DEVICES[minor] == NULL){
        /* Starting size will be 10 and we will allocate more if we need more */
        DEVICES[minor] = (struct channel *) kmalloc(sizeof(struct channel) * 10, GFP_KERNEL);
        if (DEVICES[minor] == NULL){
            printk(KERN_ERR "Error allocating memory\n");
            return -1;
        }
        memset(DEVICES[minor], -1, sizeof(struct channel) * 10);
        BOUNDRIES[minor].aSize = 10;
        BOUNDRIES[minor].iNum = 0;
    }
    /* END MEMORY CHECKING */
    /* Setting the minor number in the fp private data */
    data = (int *) kmalloc(sizeof(int) * 2, GFP_KERNEL);
    memset(data, -1, sizeof(int) * 2);
    data[0] = minor;
    fp->private_data = (void *) data;
    return SUCCESS;
}

static long int device_ioctl(struct file *fp, unsigned int command, unsigned long channelId){
    int *data;
    if (command != MSG_SLOT_CHANNEL || channelId == 0){
        printk(KERN_ERR "IOCTL command not good\n");
        return -EINVAL;
    }
    data = (int *) fp->private_data;
    data[1] = channelId;
    return SUCCESS;
}

static ssize_t device_read(struct file *fp, char* userbuffer, size_t length, loff_t *offset){
    int minor, i, msgSize, channelId, *data, j;
    struct arrayBoundries *chBound;
    struct channel *channel_array, *ch;
    char *msg, backupMsg[MESSAGE_LEN];
    data = (unsigned int *) fp->private_data;
    minor = data[0];
    if (minor == -1){
        printk(KERN_ERR "Error with minor\n");
        return -EINVAL;
    }
    channelId = data[1];
    if (channelId == -1){
        /* Not channel has been set*/
        printk(KERN_ERR "Channel id not set\n");
        return -EINVAL;
    }
    chBound = &(BOUNDRIES[minor]);
    channel_array = DEVICES[minor];
    /* We need to search for the channel strcuture with the specific id*/
    for (i = 0; i < chBound->iNum; i++){
        ch = channel_array + i;
        if (ch->id == channelId) break;
    }
    if (i == chBound->iNum){
        /* Channel wasn't found */
        printk(KERN_ERR "Channel was not found\n");
        return -EWOULDBLOCK;
    }
    if (length < ch->messageSize){
        /* The buffer size is smaller than the message */
        printk(KERN_ERR "Not enough space in buffer\n");
        return -ENOSPC;
    }
    msg = ch->MESSAGE;
    msgSize = ch->messageSize;
    /* Copying user buffer to use in case of failure */
    for (j = 0 ; j < length && j < msgSize; j++){
        if (get_user(backupMsg[i], userbuffer+j) < 0){
            return -1;
        }
    }

    for (i = 0; i < length && i < msgSize; i++){
        if(put_user(msg[i], userbuffer+i) < 0){
            /* Failure ==> Rollback all writes */
            for (j = 0 ; j < i; j++){
                put_user(backupMsg[j], userbuffer + j);
            }
            return -1;
        }
    }
    return i;
}

static ssize_t device_write(struct file *fp, const char *userBuffer, size_t length, loff_t *offset){
    int minor, i, boundSize, *data, channelId;
    struct arrayBoundries *bounds;
    struct channel *ch, *channel_array;
    char *msg, backupMsg[MESSAGE_LEN];
    ssize_t j;
    if ((length > MESSAGE_LEN) | (length <= 0)){
        return -EMSGSIZE;
    }
    data = (int *) fp->private_data;
    minor = data[0];
    channelId = data[1];
    if (channelId == -1){
        /* No channel has been set */
        printk(KERN_ERR "Channel id was not set\n");
        return -EINVAL;
    }
    bounds = &(BOUNDRIES[minor]);
    /* We need to look for the channel in the table */
    channel_array = DEVICES[minor];
    for (i = 0; i < bounds->iNum; i++){
        ch = channel_array + i;
        if (ch->id == channelId) break;
    }
    if (i == bounds->iNum){
        /* There is no channel with the requested id ==> We need to create one */
        /* No space in the array we need to realloc (by a factor of 2)*/
        if (bounds->iNum == bounds->aSize){
            boundSize = bounds->aSize;
            DEVICES[minor] = (struct channel * ) krealloc(DEVICES[minor], boundSize * 2 * sizeof(struct channel), GFP_KERNEL);
            if (DEVICES[minor] == NULL){
                printk(KERN_ERR "Error allocating memory\n");
                return -1;
            }
            memset(DEVICES[minor] + boundSize, -1, sizeof(struct channel) * boundSize);
            bounds -> aSize = boundSize * 2;
        }
        ch = &(DEVICES[minor][bounds->iNum]);
        ch->id = channelId;
        bounds->iNum++;
    }
    /* After channel lookup or creation we need to copy the data */
    msg = ch->MESSAGE;
    /* Reading message without changing channel message. This makes the write atomic */
    for (j = 0; j < length; j++){
        /* Returning error if message retrieval went wrong for some reason */
        if (get_user(backupMsg[j], userBuffer + j)){
            return - 1;
        }
    }
    /* After successful retrieval from user */
    for (j = 0; j < length; j++){
        msg[j] = backupMsg[j];
    }
    ch->messageSize = length;
    return j;
}

struct file_operations Fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .unlocked_ioctl = device_ioctl,
};


static int __init init_message_slot(void){
    if(register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops) < -1){
        printk(KERN_ALERT "%s Registration failed for %d\n", DEVICE_FILE_NAME, MAJOR_NUM);
        return -1;
    }
    printk("Loaded Module %s with major %d\n", DEVICE_FILE_NAME, MAJOR_NUM);
    /* Initializing memory */
    memset(DEVICES, 0, sizeof(struct channel * ) * MAX_DEVICES);
    memset(BOUNDRIES, -1, sizeof(struct arrayBoundries) * MAX_DEVICES);
    return SUCCESS;
}

static void __exit message_slot_cleanup(void){
    /* Cleaning data. We need to release every channels */
    int i;
    /* Freeing channels data */
    for (i = 0; i < MAX_DEVICES; i++){
        kfree(DEVICES[i]);
    }
    /* End of memory deallocation */
    /* Unregistering the device */
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
    printk("Unloaded moudel %s with major %d\n", DEVICE_FILE_NAME, MAJOR_NUM);
}

module_init(init_message_slot);
module_exit(message_slot_cleanup);





