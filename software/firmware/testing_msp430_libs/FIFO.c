/*
 * Queue.c
 *
 *  Created on: Jun 7, 2019
 *      Author: Sherwin
 */

#include <FIFO.h>

FIFO* FIFO_construct(int size) {
    FIFO* out = (FIFO*) malloc(sizeof(FIFO));
    out->buffer = (uint8_t*) malloc(size);
    out->size = size;
    out->count = 0;
    out->read_ptr = 0;
    out->write_ptr = 0;
    out->full = false;
    out->empty = true;
    return out;
}

bool FIFO_enqueue(FIFO *f, uint8_t elem) {
    if (f->full) return false;
    f->empty = false;
    f->buffer[f->write_ptr] = elem;

    if (f->write_ptr == f->size-1) f->write_ptr = 0;
    else f->write_ptr += 1;

    if (f->write_ptr == f->read_ptr) f->full = true;
    f->count += 1;
    return true;
}


uint8_t FIFO_dequeue(FIFO* f) {
    if (f->empty) return 0;
    f->full = false;
    uint8_t out = f->buffer[f->read_ptr];
    if (f->read_ptr == f->size-1) f->read_ptr = 0;
        else f->read_ptr += 1;
    if (f->read_ptr == f->write_ptr) f->empty = true;
    f->count -= 1;
    return out;
}


