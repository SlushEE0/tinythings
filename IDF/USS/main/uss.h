#ifndef _USS_H
#define _USS_H

void init_USS();
void uss_measure(int *nextPoll);
void tsk_ussRead(void *pvParameters);

#endif