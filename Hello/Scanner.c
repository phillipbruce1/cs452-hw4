#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("BSU CS 452 HW4");
MODULE_AUTHOR("<phillipbruce@u.boisestate.edu>");

// TODO: figure out why seg fault on onyx and pkill on local vm - somewhere in ioctl?

typedef struct {
    dev_t devno;
    struct cdev cdev;
    char *s;
    char *defaultSeparators;
} Device;            /* per-init() data */

typedef struct {
    char *s;
    char *separators;
    int *newSeparators;
} File;                /* per-open() data */

static Device device;

/**
 * Opens a scanner
 * @param inode
 * @param filp
 * @return
 */
static int open(struct inode *inode, struct file *filp) {
    File *file = (File *) kmalloc(sizeof(*file), GFP_KERNEL);
    if (!file) {
        printk(KERN_ERR
        "%s: kmalloc() failed\n", DEVNAME);
        return -ENOMEM;
    }
    file->s = (char *) kmalloc(strlen(device.s) + 1, GFP_KERNEL);
    if (!file->s) {
        printk(KERN_ERR
        "%s: kmalloc() failed\n", DEVNAME);
        return -ENOMEM;
    }
    strcpy(file->s, device.s);
    file->separators = (char *) kmalloc(sizeof(char *) * 2, GFP_KERNEL);
    if (!file->separators) {
        printk(KERN_ERR
        "%s: kmalloc() failed\n", DEVNAME);
        return -ENOMEM;
    }
    file->separators = ",;";
    filp->private_data = file;
    return 0;
}

static int release(struct inode *inode, struct file *filp) {
    File *file = filp->private_data;
    kfree(file->s);
    kfree(file);
    return 0;
}

/**
 * Copy from buf to file from kernel to user space
 * @param filp
 * @param buf
 * @param count
 * @param f_pos
 * @return
 */
static ssize_t read(struct file *filp,
                    char *buf,
                    size_t count,
                    loff_t *f_pos) {
    File *file = filp->private_data;
    int n = strlen(file->s);
    n = (n < count ? n : count);
    if (copy_to_user(buf, file->s, n)) {
        printk(KERN_ERR
        "%s: copy_to_user() failed\n", DEVNAME);
        return 0;
    }
    return n;
}

/**
 * When separators == 0, initialize separators with buf. Otherwise, write file to buf from user to kernel space
 * @param filp
 * @param buf
 * @param count
 * @param f_pos
 * @return
 */
static ssize_t write(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
    File *file = filp->private_data;
    if (file->separators == 0) {
        file->separators = (char *) kmalloc(sizeof(char *) * strlen(buf), GFP_KERNEL);
        if (!file->separators) {
            printk("%s: kmalloc() failed\n", DEVNAME);
            return -ENOMEM;
        }
        // TODO: set separators = buf
        return strlen(buf);
    } else {
        int n = strlen(buf);
        n = (n < count ? n : count);
        if (copy_from_user(file->s, buf, n)) {
            printk(KERN_ERR
            "%s: copy_to_user() failed\n", DEVNAME);
            return 0;
        }
        return n;
    }
}

/**
 * Resets separators when cmd == 0
 * @param filp
 * @param cmd
 * @param arg
 * @return
 */
static long ioctl(struct file *filp,
                  unsigned int cmd,
                  unsigned long arg) {
    File *file = filp->private_data;
    if (cmd == 0) {
        if (file->separators)
            kfree(file->separators);
        file->separators = 0;
    }
    return 0;
}

static struct file_operations ops = {
        .open=open,
        .release=release,
        .read=read,
        .unlocked_ioctl=ioctl,
        .owner=THIS_MODULE
};

static int __init

my_init(void) {
    // set default separators
//    device.separators = (char *) kmalloc(sizeof(char *) * 2, GFP_KERNEL);
//    if (!device.separators) {
//        printk(KERN_ERR
//        "%s: kmalloc() failed\n", DEVNAME);
//        return -ENOMEM;
//    }
//    device.separators = ",:";
    // other stuff
    const char *s = "Hello world!\n";
    int err;
    device.s = (char *) kmalloc(strlen(s) + 1, GFP_KERNEL);
    if (!device.s) {
        printk(KERN_ERR
        "%s: kmalloc() failed\n", DEVNAME);
        return -ENOMEM;
    }
    strcpy(device.s, s);
    err = alloc_chrdev_region(&device.devno, 0, 1, DEVNAME);
    if (err < 0) {
        printk(KERN_ERR
        "%s: alloc_chrdev_region() failed\n", DEVNAME);
        return err;
    }
    cdev_init(&device.cdev, &ops);
    device.cdev.owner = THIS_MODULE;
    err = cdev_add(&device.cdev, device.devno, 1);
    if (err) {
        printk(KERN_ERR
        "%s: cdev_add() failed\n", DEVNAME);
        return err;
    }
    printk(KERN_INFO
    "%s: init\n", DEVNAME);
    return 0;
}

static void __exit

my_exit(void) {
    cdev_del(&device.cdev);
    unregister_chrdev_region(device.devno, 1);
    kfree(device.s);
    printk(KERN_INFO
    "%s: exit\n", DEVNAME);
}

module_init(my_init);
module_exit(my_exit);
