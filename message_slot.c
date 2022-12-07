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




MODULE_LICENSE("GPL");
#define SUCCESS 0
#define DEVICE_RANGE_NAME "message_slot"
#define MESSAGE_LEN 128
#define DEVICE_FILE_NAME "message_device"
#define MAJOR_NUM 235
#define MAX_CHANNELS 0x100000
#define MAX_DEVICES 256

#ifndef MSG_SLOT_CHANNEL
#define MSG_SLOT_CHANNEL 10
#endif

struct channel{
    int id;
    int messageSize;
    char MESSAGE[128];
};

struct arrayBoundries{
    int iNum;
    int aSize;
};


struct channel *DEVICES[MAX_DEVICES];
int USER_CHANNELS[MAX_DEVICES];
struct arrayBoundries *BOUNDRIES[MAX_DEVICES];

static int device_open(struct inode *inode, struct file *fp){
    /* Check if the minor memory is allocated already. Otherwise allocate space for it*/
    int minor = iminor(inode);
    if (DEVICES[minor] == NULL){
        /* Starting size will be 10 and we will allocate more if we need more */
        DEVICES[minor] = (struct channel *) kmalloc(sizeof(struct channel) * 10, GFP_KERNEL);
        if (DEVICES[minor] == NULL) return -1;
        memset(DEVICES[minor], 0, sizeof(struct channel) * 10);
        BOUNDRIES[minor] = (struct arrayBoundries *) kmalloc(sizeof(struct arrayBoundries), GFP_KERNEL);
        if(BOUNDRIES[minor] == NULL) return -1;
        BOUNDRIES[minor] -> aSize = 10;
        BOUNDRIES[minor] -> iNum = 0;
    }
    /* END MEMORY CHECKING */
    /* Setting the minor number in the fp private data */
    fp->private_data = (void *) minor;
    return SUCCESS;
}

static long int device_ioctl(struct file *fp, unsigned int channelId, unsigned long command){
    int minor;
    if (command != MSG_SLOT_CHANNEL || channelId == 0){
        return -EINVAL;
    }
    minor = (int) fp->private_data;
    /* Checking if the channel even exists will happen in the write/read functions */
    USER_CHANNELS[minor] = channelId;
    return SUCCESS;
}

static ssize_t device_read(struct file *fp, char* userbuffer, size_t length, loff_t *offset){
    int minor, channelId, i;
    struct arrayBoundries *chBound;
    struct channel *channel_array, *ch;
    char *msg;
    minor = (int) fp->private_data;
    if (minor == -1){
        return -EINVAL;
    }
    channelId = USER_CHANNELS[minor];
    if (channelId == -1){
        /* Not channel has been set*/
        return -EINVAL;
    }
    chBound = BOUNDRIES[minor];
    channel_array = DEVICES[minor];
    /* char *msg = DEVICES[minor][channel].MESSAGE; */
    /* We need to search for the channel strcuture with the specific id*/
    for (i = 0; i < chBound->iNum; i++){
        ch = channel_array + i;
        if (ch->id == channelId) break;
    }
    if (i == chBound->iNum){
        /* Channel wasn't found */
        return -EWOULDBLOCK;
    }
    if (length < ch->messageSize){
        /* The buffer size is smaller than the message */
        return -ENOSPC;
    }
    msg = ch->MESSAGE;
    for (i = 0; i < length && i < MESSAGE_LEN; i++){
        put_user(msg[i], userbuffer+i);
    }
    return i;
}

static ssize_t device_write(struct file *fp, const char *userBuffer, size_t length, loff_t *offset){
    int minor, channelId, i;
    struct arrayBoundries *bounds;
    struct channel *ch, *channel_Array;
    char *msg;
    ssize_t j;
    if (length > MESSAGE_LEN){
        return -EMSGSIZE;
    }
    minor = (int) fp -> private_data;
    channelId = USER_CHANNELS[minor];
    if (channelId == -1){
        /* No channel has been set */
        return -EINVAL;
    }
    bounds = BOUNDRIES[minor];
    /* We need to look for the channel in the table */
    channel_Array = DEVICES[minor];
    for (i = 0; i < bounds->iNum; i++){
        ch = channel_Array + i;
        if (ch->id == channelId) break;
    }
    if (i == bounds->iNum){
        /* There is no channel with the requested id ==> We need to create one */
        /* No space in the array we need to realloc (by a factor of 2)*/
        if (bounds->iNum == bounds->aSize){
            DEVICES[minor] = (struct channel * ) krealloc(DEVICES[minor], bounds->aSize * 2 * sizeof(struct channel), GFP_KERNEL);
            if (DEVICES[minor] == NULL) return -1;
            bounds -> aSize = bounds->aSize * 2;
        }
        /*DEVICES[minor][bounds->iNum] = (struct channel) kmalloc(sizeof(struct channel), GFP_KERNEL);
        if (DEVICES[minor][bounds->iNum] == NULL) return -1;*/
        ch = &(DEVICES[minor][bounds->iNum]);
        bounds->iNum++;
    }
    /* After channel lookup or creation we need to copy the data */
    msg = ch->MESSAGE;
    for (j = 0; j < length; j++){
        get_user(*msg, userBuffer);
        msg++;
        userBuffer++;
    }
    ch->messageSize = length;
    return i;
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
    /* Initializing memory */
    memset(DEVICES, 0, sizeof(struct channel * ) * MAX_DEVICES);
    memset(USER_CHANNELS, -1, sizeof(int) * MAX_DEVICES);
    memset(BOUNDRIES, -1, sizeof(struct arrayBoundries) * MAX_DEVICES);
    return SUCCESS;
}

static void __exit message_slot_cleanup(void){
    /* Cleaning data. We need to release every channels */
    int i,j, numOfItems;
    struct arrayBoundries *bounds;
    struct channel *channels;
    /* Freeing channels data */
    for (i = 0; i < 256; i++){
        channels = DEVICES[i];
        bounds = BOUNDRIES[i];
        numOfItems = bounds->iNum;
        for (j = 0; j < numOfItems; j++){
            kfree(channels + j);
        }
        kfree(channels);
        kfree(bounds);
    }
    /* End of memory deallocation */
    /* Unregistering the device */
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

module_init(init_message_slot);
module_exit(message_slot_cleanup);





