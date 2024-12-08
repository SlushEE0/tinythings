#ifndef _USS_H
#define _USS_H

void init_USS();
void uss_measure(bool* isOpen);
void tsk_ussRead(void *pvParameters);

typedef struct t_GarageObj {
  bool readState;
  bool isOpen;
} t_GarageObj;

#endif