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


typedef struct {
    dev_t devno;
    struct cdev cdev;
    char *s;
    char *defaultSeparators;
} Device;            /* per-init() data */

typedef struct {
    char *s;
    char *separators;
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
    char *val = ",;";
    strcpy(file->separators, val);
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
 *
 * Returns 0 for end of token, -1 for end of file,
 * or length of data returned if token length > count
 *
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
    if (n == 0)
        return -1;
    n = (n < count ? n : count);
    int i;
    for (i = 0; i < n; i++) {
        printk("%s\n", file->s[i]);
        int k;
        for (k = 0; k < strlen(file->separators); k++) {
            printk("separator: %s\n", file->separators[k]);
            // if token found
            if (file->s[i] == file->separators[k]) {
                // get token
                char token[i];
                int j;
                for (j = 0; j < i; j++)
                    token[j] = file->s[j];
                // copy token to user space buffer
                copy_to_user(buf, token, n);
                buf[n] = 0;
                // remove token from file
                file->s = file->s[i + 1];
                // return 0 for end of token
                return 0;
            }
        }
    }
    // if this point reached, there is no separator within n characters
    if (copy_to_user(buf, file->s, n)) {
        printk(KERN_ERR
        "%s: copy_to_user() failed\n", DEVNAME);
        buf[n] = 0;
        return 0;
    }
    // n < strlen, then token is too long for count (0)
    if (n < strlen(file->s)) {
        memmove((void *) file->s, (void *) file->s[n + 1], strlen(file->s) - n);
        buf[n] = 0;
        return n;
    } else {
        // else end of file has been reached (-1)
        file->s = "";
        buf[n] = 0;
        return 0;
    }
}

/**
 * When separators == 0, initialize separators with buf. Otherwise, write file to buf from user to kernel space
 * @param filp
 * @param buf
 * @param count
 * @param f_pos
 * @return
 */
static ssize_t write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
    File *file = filp->private_data;
    if (file->separators == 0) {
        file->separators = (char *) kmalloc(sizeof(char *) * strlen(buf), GFP_KERNEL);
        if (!file->separators) {
            printk("%s: kmalloc() failed\n", DEVNAME);
            return -ENOMEM;
        }
        strcpy(file->separators, buf);
        printk("separators: %s\n", file->separators);
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
        .write=write,
        .unlocked_ioctl=ioctl,
        .owner=THIS_MODULE
};

static int __init

my_init(void) {
    const char *s = "";
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
