#include <stdint.h>
# include <stddef.h>

void insortRows(uint16_t *r, const uint32_t * const g, const uint32_t n) {
   size_t i,j;
   uint16_t t;
   
   for(i = 1; i < n; ++i){
      t = r[i];
      j = i - 1;
      while(j >= 0 && g[r[j]] < g[t]){
         r[j+1] = r[j];
         --j;
      }
      r[j+1] = t;
   }
}

