#ifndef COMM_H_
#define COMM_H_

#define COMM_BUFFER_LEN 16

typedef struct{
  uint8_t key;
  uint16_t val;
}
comm_t;

// Buffer messages to sent to the monitor
comm_t comm_buffer[COMM_BUFFER_LEN];
unsigned char comm_head;
unsigned char comm_tail;

/**
 * Push an element onto the end of the  buffer.
 */
void comm_push(comm_t d)
{
   // Add to the buffer
   comm_buffer[comm_head] = d;

   // Increment the index
   comm_head++;
   if (comm_head >= COMM_BUFFER_LEN) {
       comm_head = 0;
   }
   
}

/**
 * Shift an element off the beginning of the buffer.
 */
comm_t comm_shift()
{
    comm_t d = comm_buffer[comm_tail];
    // Increase the processed index
    comm_tail++;
    if (comm_tail >= COMM_BUFFER_LEN) {
        comm_tail = 0;
    }

    return d;
}

/**
 * Determine whether the buffer is empty.
 */
uint8_t comm_empty()
{
    if (comm_head == comm_tail) {
        return 1;
    }
    return 0;
}

#endif

