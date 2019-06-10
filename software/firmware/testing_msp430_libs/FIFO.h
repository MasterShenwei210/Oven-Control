/*
 * queue.h
 *
 *  Created on: Jun 7, 2019
 *      Author: Sherwin
 */

#ifndef FIFO_H_
#define FIFO_H_
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct FIFOS {
    uint8_t size;           // Can read these, BUT DONT WRITE!!!
    uint8_t count;
    bool full;
    bool empty;

    uint8_t read_ptr;       // Don't bother looking at these
    uint8_t write_ptr;
    uint8_t* buffer;
} FIFO;

FIFO* FIFO_construct(int size);
bool FIFO_enqueue(FIFO *f, uint8_t elem);
uint8_t FIFO_dequeue(FIFO *f);

#endif /* FIFO_H_ */
