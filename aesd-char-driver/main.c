/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */
#include "aesd_ioctl.h"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h>
#include "aesdchar.h"


//#include <linux/stdbool.h>
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Darshan Salian"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    struct aesd_dev *dev;
     dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
     filp->private_data = dev;
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    filp->private_data = NULL;
    return 0;
}

long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct aesd_dev *dev = filp->private_data;
    struct aesd_seekto seekto;

    if (_IOC_TYPE(cmd) != AESD_IOC_MAGIC || _IOC_NR(cmd) > AESDCHAR_IOC_MAXNR)
        return -ENOTTY;

    if (cmd == AESDCHAR_IOCSEEKTO)
    {
        if (copy_from_user(&seekto, (const void __user *)arg, sizeof(seekto)))
            return -EFAULT;

        if (mutex_lock_interruptible(&dev->Mutexlock))
            return -ERESTARTSYS;

        // Validate write_cmd index
        if (seekto.write_cmd >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
        {
            mutex_unlock(&dev->Mutexlock);
            return -EINVAL;
        }

        uint8_t cmd_index = (dev->buffer.out_offs + seekto.write_cmd) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        struct aesd_buffer_entry *entry = &dev->buffer.entry[cmd_index];

        if (!entry->buffptr || seekto.write_cmd_offset > entry->size)
        {
            mutex_unlock(&dev->Mutexlock);
            return -EINVAL;
        }

        // Calculate f_pos based on all previous entries
        loff_t pos = 0;
        for (uint8_t i = 0; i < seekto.write_cmd; ++i)
        {
            uint8_t real_index = (dev->buffer.out_offs + i) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
            pos += dev->buffer.entry[real_index].size;
        }

        pos += seekto.write_cmd_offset;
        filp->f_pos = pos;

        mutex_unlock(&dev->Mutexlock);
        return 0;
    }

    return -ENOTTY;
}


loff_t aesd_llseek(struct file *filp, loff_t offset, int whence)
{
  struct aesd_dev *dev = filp->private_data;
  loff_t new_pos = 0;
  
  if(dev==NULL)
  {
    return -EINVAL;
  }
  if (mutex_lock_interruptible(&dev->Mutexlock) !=0 ) 
  {
        
        PDEBUG("Fail to acquire mutex\n");
        return -ERESTART;
    
  }
  
  size_t total_size = 0;
  struct aesd_buffer_entry *entry;
  uint8_t Count;
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &dev->buffer, Count) 
    {
        total_size += entry->size;
    }
    
    switch (whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = filp->f_pos + offset;
            break;
        case SEEK_END:
            new_pos = total_size + offset;
            break;
        default:
            mutex_unlock(&dev->Mutexlock);
            return -EINVAL;
    }

    if (new_pos < 0 || new_pos > total_size) {
        mutex_unlock(&dev->Mutexlock);
        return -EINVAL;
    }

    filp->f_pos = new_pos;

    mutex_unlock(&dev->Mutexlock);
    return new_pos;
    
}


ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                  loff_t *f_pos)
{
    ssize_t Value = 0;
    PDEBUG("read %zu bytes with offset %lld", count, *f_pos);

    struct aesd_dev *dev = filp->private_data;
    size_t bytes_Left = 0;
    size_t Pos = 0;

    
    if (mutex_lock_interruptible(&dev->Mutexlock)) 
    {
        PDEBUG("Fail to acquire mutex\n");
        return -ERESTART;
    }

    struct aesd_buffer_entry *Buffer_temp = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer, *f_pos, &Pos);

    if (Buffer_temp == NULL) 
    {
        PDEBUG("At position no entry found\n");
        Value = 0; 
    } 
    else 
     {
        bytes_Left = Buffer_temp->size - Pos;
        if (bytes_Left > count) 
        {
          bytes_Left = count;
        }

        if (copy_to_user(buf, Buffer_temp->buffptr + Pos, bytes_Left) != 0) 
        {
            PDEBUG("Copy failed to user space\n");
            Value = -EFAULT;
        } else 
        {
            *f_pos += bytes_Left; 
            Value = bytes_Left;  
        }
    }

    
    mutex_unlock(&dev->Mutexlock);
    PDEBUG("unlock Mutex Success\n");

    return Value;
}




ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
   ssize_t Value = -ENOMEM;
    char *UserSpacePtr = NULL;
    const char *PtrNewLine = NULL;
    size_t NewlineCharBytes = 0;
    struct aesd_dev *dev = NULL;
    void *temp_ptr = NULL;

    dev = filp->private_data;
    if (dev==NULL) {
        PDEBUG("Memory Failed to allocate\n");
        return -ENOMEM;
    }
    
    UserSpacePtr = kmalloc(count, GFP_KERNEL);
    if (UserSpacePtr==NULL) {
        PDEBUG("Memory Failed to allocate\n");
        return -ENOMEM;
    }

    
    if (copy_from_user(UserSpacePtr, buf, count)) {
        PDEBUG("Copy failed to user space\n");
        kfree(UserSpacePtr);
        return -EFAULT;
    }

   
    PtrNewLine = memchr(UserSpacePtr, '\n', count);
    if (PtrNewLine) 
    {
      NewlineCharBytes = (size_t)(PtrNewLine - UserSpacePtr) + 1;
    } 
    else
     {
        NewlineCharBytes = 0;
     }


    
    if (mutex_lock_interruptible(&dev->Mutexlock) != 0) {
        PDEBUG("Fail to acquire mutex\n");
        kfree(UserSpacePtr);
        return -ERESTART;
    }

  if (NewlineCharBytes > 0) {

    temp_ptr = krealloc(dev->entry.buffptr, dev->entry.size + NewlineCharBytes, GFP_KERNEL);

    if (temp_ptr != NULL) {
        dev->entry.buffptr = temp_ptr;
        memcpy(dev->entry.buffptr + dev->entry.size, UserSpacePtr, NewlineCharBytes);
        dev->entry.size += NewlineCharBytes;

        const char *old_entry = aesd_circular_buffer_add_entry(&dev->buffer, &dev->entry);
        
        if (old_entry != NULL) {
            kfree(old_entry);
        }

        dev->entry.buffptr = NULL;
        dev->entry.size = 0;
        Value = count;
    } 
    else {
        PDEBUG("Memory Failed to allocate\n");
        Value = -ENOMEM;
    }

} 
else {

    temp_ptr = krealloc(dev->entry.buffptr, dev->entry.size + count, GFP_KERNEL);

    if (temp_ptr != NULL) {
        dev->entry.buffptr = temp_ptr;
        memcpy(dev->entry.buffptr + dev->entry.size, UserSpacePtr, count);
        dev->entry.size += count;
        Value = count;
    } 
    else {
        PDEBUG("Memory Failed to allocate\n");
        Value = -ENOMEM;
    }
}


    mutex_unlock(&dev->Mutexlock);
    kfree(UserSpacePtr);
    return Value;
}


struct file_operations aesd_fops = {
    .owner =           THIS_MODULE,
    .read =            aesd_read,
    .write =           aesd_write,
    .open =            aesd_open,
    .release =         aesd_release,
    .llseek =          aesd_llseek,
    .unlocked_ioctl =  aesd_ioctl,
};


static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Adding as a Char dev failed with error %d\n", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));
	aesd_circular_buffer_init(&aesd_device.buffer);
	mutex_init(&aesd_device.Mutexlock);


    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);
uint8_t index = 0;
struct aesd_buffer_entry *entry = NULL;

for (index = 0; index < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; index++) {
    entry = &aesd_device.buffer.entry[index];
    if (entry->buffptr) {
        kfree(entry->buffptr);
        entry->buffptr = NULL;
    }
}

mutex_destroy(&aesd_device.Mutexlock);


    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);


