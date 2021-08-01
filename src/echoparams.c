#include <stdio.h>
#include "const.h"
#include "defines.h"

/* ======================== */
/*  Echo loaded parameters  */
/* ======================== */

void echoParams(const int * params, const char *rule, const int period){
   printf("Rule: %s\n",rule);
   printf("Period: %d\n",params[P_PERIOD]);
   printf("Offset: %d\n",params[P_OFFSET]);
   printf("Width:  %d\n",params[P_WIDTH]);
   if (params[P_SYMMETRY] == SYM_ASYM) printf("Symmetry: asymmetric\n");
   else if (params[P_SYMMETRY] == SYM_ODD) printf("Symmetry: odd\n");
   else if (params[P_SYMMETRY] == SYM_EVEN) printf("Symmetry: even\n");
   else if (params[P_SYMMETRY] == SYM_GUTTER) printf("Symmetry: gutter\n");
   if (params[P_CHECKPOINT]) printf("Dump state after queue compaction\n");
   printf("Queue size: 2^%d\n",params[P_QBITS]);
   printf("Hash table size: 2^%d\n",params[P_HASHBITS]);
   printf("Minimum deepening increment: %d\n", mindeep(params, period));
   if (params[P_PRINTDEEP] == 0)printf("Output disabled while deepening\n");
#ifndef NOCACHE
   if (params[P_CACHEMEM])
      printf("Cache memory per thread: %d megabytes\n", params[P_CACHEMEM]);
   else
      printf("Lookahead caching disabled\n");
#endif
   if (params[P_MEMLIMIT] >= 0) printf("Memory limit: %d megabytes\n",params[P_MEMLIMIT]);
   printf("Number of threads: %d\n",params[P_NUMTHREADS]);
   if (params[P_MINEXTENSION]) printf("Save depth-first extensions of length at least %d\n",params[P_MINEXTENSION]);
   if (params[P_LONGEST] == 0) printf("Printing of longest partial result disabled\n");
}

