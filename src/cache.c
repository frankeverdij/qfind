/* ================= */
/*  Lookahead cache  */
/* ================= */

#include <stdio.h>
#include <stdlib.h>
#include "cache.h"

long long cachesize ;
typedef struct {
    uint16_t *p1, *p2, *p3;
    int abn, r;
} cacheentry;

cacheentry *totalCache;

cacheentry **cache;

int getkey(uint16_t *p1, uint16_t *p2, uint16_t *p3, int abn, const int * const pm) {
#ifndef QSIMPLE
    if(pm[P_CACHEMEM] == 0)
        return 0;
#endif
    unsigned long long h = (unsigned long long)p1 + 17 * (unsigned long long)p2
    + 257 * (unsigned long long)p3 + 513 * abn;
    h = h + (h >> 15);
    h &= (cachesize-1);
    cacheentry *ce = &(cache[omp_get_thread_num()][h]);
    if (ce->p1 == p1 && ce->p2 == p2 && ce->p3 == p3 && ce->abn == abn)
        return -2 + ce->r;
    ce->p1 = p1;
    ce->p2 = p2;
    ce->p3 = p3;
    ce->abn = abn;
    return h;
}

void setkey(int h, int v, const int * const pm) {
#ifndef QSIMPLE
    if(pm[P_CACHEMEM])
#endif
        cache[omp_get_thread_num()][h].r = v ;
}

int cacheallocate(const int memlimit, const int * const pm) {
    int memrequested;
    cachesize = 32768 ;
    while (cachesize * sizeof(cacheentry) < 550000 * pm[P_CACHEMEM])
        cachesize <<= 1 ;
    memrequested = sizeof(cacheentry) * (cachesize + 5) * pm[P_NUMTHREADS];
    if(pm[P_MEMLIMIT] >= 0 && memrequested > memlimit){
        printf("Not enough memory to allocate lookahead cache\n");
        exit(0);
    } else
        printf("Allocating lookahead cache %d\n",memrequested);
    totalCache = (cacheentry *)calloc((cachesize + 5) * pm[P_NUMTHREADS], sizeof(cacheentry)) ;
    cache = (cacheentry **)calloc(pm[P_NUMTHREADS], sizeof(**cache));
   
    for (int i = 0; i < pm[P_NUMTHREADS]; i++)
        cache[i] = totalCache + (cachesize + 5) * i;

    return memrequested;
}
