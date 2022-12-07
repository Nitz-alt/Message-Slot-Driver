#undef __KERNAL__
#define __KERNAL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>




MODULE_LICENSE("GPL");
#define SUCCESS 0
#define DEVICE_RANGE_NAME "message_slot"
#define MESSAGE_LEN 128
#define DEVICE_FILE_NAME "message_device"
#define MAJOR_NUM 235
#define MAX_CHANNELS 0x100000

struct channel{
    int id;
    int messageSize;
    char MESSAGE[128];
};

struct arrayBoundries{
    int iNum;
    int aSize;
};


struct channel *DEVICES[256];
int USER_CHANNELS[256];
struct arrayBoundries BOUNDRIES[256];

static int device_open(struct inode *inode, struct file *fp){
    /* Check if the minor memory is allocated already. Otherwise allocate space for it*/
    int minor = iminor(inode);
    if (DEVICES[minor] == NULL){
        /* Starting size will be 10 and we will allocate more if we need more */
        DEVICES[minor] = kmalloc(sizeof(struct channel) * 10, GFP_KERNEL);
        memset(DEVICES[minor], 0, sizeof(struct channel) * 10);
        BOUNDRIES[minor] = {0, 10};
    }
    /* END MEMORY CHECKING */
    /* Setting the minor number in the fp private data */
    fp->private_data = minor;
    return SUCCESS;
}

static int device_ioctl(struct file *fp, unsigned int channelId, unsigned long command){
    int minor;
    if (command != MSG_SLOT_CHANNEL || channelId == 0){
        errno = EINVAL;
        return -1;
    }
    minor = fp->private_data;
    /* Checking if the channel even exists will happen in the write/read functions */
    USER_CHANNELS[minor] = channelId;
    return SUCCESS;
}

static ssize_t device_read(struct *file fp, char* userbuffer, size_t length, loff_t *offset){
    int minor, channelId, bytesRead = 0, steps = 1;
    struct arrayBoundries chBound;
    struct channel *chAr;
    minor = fp->private_data;;
    char *msg;
    if (minor == -1){
        errno = EINVAL;
        return -1;
    }
    channelId = USER_CHANNELS[minor];
    chBound = BOUNDRIES[minor];
    chAr = DEVICES[minor];
    /* char *msg = DEVICES[minor][channel].MESSAGE; */
    /* We need to search for the channel strcuture with the specific id*/
    while (steps++ <= chBound.iNum && chAr != NULL && chAr -> id != channelId) chAr++;
    if (chAr == NULL || steps == chBound.iNum + 1){
        errno = EWOULDBLOCK;
        return -1;
    }
    if (length < chAr->messageSize){
        errno = ENOSPC;
        return -1;
    }
    msg = chAr->MESSAGE;
    while (length && *msg){
        put_user(*(msg++), userbuffer++);
        bytesRead++;
        length--;
    }
    return bytesRead;
}

static ssize_t device_write(strcut file *fp, const char *userBuffer, ssize_t length, loff_t *offset){
    int minor, channelId, i;
    struct arrayBoundries bounds;
    strcut channel *ch;
    ssize_t i;
    if (length > MESSAGE_LEN){
        errno = EMSGSIZE;
        return -1;
    }
    minor = fp -> private_data;
    channelId = USER_CHANNELS[minor];
    bounds = BOUNDRIES[minor];
    /* We need to look for the channel in the table */
    ch = DEVICES[minor];
    for (i = 0; i < bounds.iNum; i++){
        if (ch->id == channelId) break;
    }
    if (i == bounds.iNum){
        /* There is no channel with the requested id ==> We need to create one */
    }
    


    

}






struct file_operations Fops{
    .owner = THIS_MODULE;
    .read = device_read;
    .write = NULL;
    .open = device_open;
    .ioctl = device_ioctl;
}


static int __init init_message_slot(void){
    if(register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops) < -1){
        printk(KERN_ALERT "%s Registration failed for %d\n", DEVICE_FILE_NAME, MAJOR_NUM);
        return -1;
    }
    return SUCCESS;
}

static void __exit message_slot_cleanup(void){
    /* Cleaning data. We need to release every channels */
    int i;
    for (i = 0; i < 256; i++){
        kfree(DEVICES[i]);
    }
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

module_init(init_message_slot);
module_exit(message_slot_cleanup);





