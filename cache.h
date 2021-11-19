#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>

#include "const.h"
#include "openmp.h"

int getkey(uint16_t *p1, uint16_t *p2, uint16_t *p3, int abn, const int * const pm);
void setkey(int h, int v, const int * const pm);
int cacheallocate(const int memlimit, const int * const pm);

#endif /* CACHE_H */
