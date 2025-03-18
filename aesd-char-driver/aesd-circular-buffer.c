/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#include <linux/slab.h>  // For kmalloc and kfree
#include <linux/mutex.h>
#include <linux/printk.h>
#else
#include <string.h>
#include <stdio.h>
#endif

#include "aesd-circular-buffer.h"




/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */
  if (!buffer || !entry_offset_byte_rtn) 
  {
      return NULL;
  }
    
	int count;
	if (!buffer->full) 
	{
	    
	    count = buffer->in_offs;
	} else 
	{
	    count = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
	}

    size_t OffSet_Pos = 0;
    
       uint8_t i = 0;
	while (i < count) {
	    uint8_t Pos = (buffer->out_offs+i) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
	    struct aesd_buffer_entry *Buffer_entry = &buffer->entry[Pos];

	      if (char_offset >= OffSet_Pos) 
	      {
	      
		    if (char_offset < (OffSet_Pos + Buffer_entry->size)) 
		    {
			*entry_offset_byte_rtn = char_offset - OffSet_Pos;
			return Buffer_entry;
		    }
		    
		}

	    OffSet_Pos += Buffer_entry->size;
	    i++;
	}

    
    return NULL;
   
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
const char* aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description
    */
    const char *Value = NULL;
   if (!buffer || !add_entry) 
   {
      return Value;
    
   }
    if (!buffer->full) 
    {
    
        
        
        //printk("No overwrite needed.\n");
    }
    else 
	{
	   struct aesd_buffer_entry *overwrite = &buffer->entry[buffer->in_offs];
        
        overwrite->buffptr = 0;
        overwrite->size = 0;
        Value = buffer->entry[buffer->out_offs].buffptr;
        buffer->out_offs = (buffer->out_offs+1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;  
	}
   
    
    memcpy(&buffer->entry[buffer->in_offs], add_entry, sizeof(struct aesd_buffer_entry));
    
    buffer->in_offs = (buffer->in_offs+1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    
    if (buffer->in_offs == buffer->out_offs && buffer->full == false) 
	{
	    buffer->full = true;
	    //printk("Buffer is full. \n");
	} 
	else 
	{
	    //printk("Buffer is not full yet. \n");
	}
	
	 return Value;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
      memset(buffer,0,sizeof(struct aesd_circular_buffer));
}


