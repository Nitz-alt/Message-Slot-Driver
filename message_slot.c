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
#define BUF_LEN 128
#define DEVICE_FILE_NAME "message_device"
#define MAJOR_NUM 235

struct channel{
    int id;
    char MESSAGE[128];
}

struct channel *DEVICES[256];
int USER_CHANNELS[256];

int device_open(struct inode *inode, struct file *fp){
    /* Check if the minor memory is allocated already. Otherwise allocate space for it*/
    int minor = iminor(inode);
    if (DEVICES[minor] == NULL){
        /* Starting size will be 10 and we will allocate more if we need more */
        DEVICES[minor] = kmalloc(sizeof(struct channel) * 10, GFP_KERNEL);
        memset(DEVICES[minor], 0, sizeof(struct channel) * 10);
    }
    /* END MEMORY CHECKING */
    /* Setting the minor number in the fp private data */
    fp->private_data = minor;
}

static int __init init_message_slot(void){
    if(register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops) < -1){
        printk(KERN_ALERT "%s Registration failed for %d\n", DEVICE_FILE_NAME, MAJOR_NUM);
        return -1;
    }
    return 0;
}

int device_ioctl(struct file *fp, unsigned int channelId, unsigned long command){
    int minor;
    if (command != MSG_SLOT_CHANNEL || channelId == 0){
        errno = EINVAL;
        return -1;
    }
    minor = fp->private_data;
    /* Checking if the channel even exists will happen in the write/read functions */
    USER_CHANNELS[minor] = channelId;
}

struct file_operations Fops{
    .owner = THIS_MODULE;
    .read = NULL;
    .write = NULL;
    .open = device_open;
    .release = NULL;
    .ioctl = device_ioctl;
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





