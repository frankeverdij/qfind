#include <stdio.h>
#include <stdlib.h>
#include <const.h>

/* ============================ */
/*  Load saved state from file  */
/* ============================ */

void loadFail(const char * file)
{
    printf("Load from file %s failed\n",file);
    exit(1);
}

signed int loadInt(FILE *fp, const char * file)
{
    signed int v;
    if (fscanf(fp,"%d\n",&v) != 1) loadFail(file);
    return v;
}

unsigned int loadUInt(FILE *fp, const char * file)
{
    unsigned int v;
    if (fscanf(fp,"%u\n",&v) != 1) loadFail(file);
    return v;
}

void loadParams(int *params, const char *file, int *newLastDeep, char *rule, char *dumpRoot) {
   FILE * fp;

   /* reset flag to prevent modification of params[P_LASTDEEP] at start of search */
   *newLastDeep = 0;
   
   fp = fopen(file, "r");
   if (!fp) loadFail(file);
   if (loadUInt(fp, file) != FILEVERSION)
   {
      printf("Incompatible file version\n");
      exit(1);
   }
   
   /* Load rule */
   if (fscanf(fp,"%255s\n",rule) != 1) loadFail(file);
   
   /* Load dump root */
   if (fscanf(fp,"%255s\n",dumpRoot) != 1) loadFail(file);
   
   /* Load parameters */
   for (int i = 0; i < NUM_PARAMS; ++i)
      params[i] = loadInt(fp, file);
}

