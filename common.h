/* This header file contains functions common to both qfind and qfind-s.
** Some functions contained in this file work slightly differently depending
** on whether qfind or or qfind-s is being compiled.  Such differences are
** determined by the presence of the macro QSIMPLE defined in qfind-s.cpp.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _OPENMP
#include <omp.h>
#else
#define omp_get_thread_num() 0
#define omp_set_num_threads(x)
#endif

#include <time.h>

#include "const.h"
#include "enum.h"
#include "load.h"

#ifdef QSIMPLE
   #define WHICHPROGRAM qfind-simple
#else
   #define WHICHPROGRAM qfind
#endif

#define STR(x) #x
#define XSTR(x) STR(x)

#define BANNER XSTR(WHICHPROGRAM)" v2.1 by Matthias Merzenich, 3 May 2021"

#define DEFAULT_DEPTHLIMIT (qBits-3)

char *initRows;

int params[NUM_PARAMS];
int width;
int deepeningAmount;
int nRowsInState;
int phase;

int period;
int offset;

int aborting;
int numFound = 0;       /* number of spaceships found so far */
int longest = 0;        /* length of current longest partial result */

Mode mode;
Dump dump;

/* the big data structures */
#define qBits params[P_QBITS]
#define QSIZE (1<<qBits)

#define hashBits params[P_HASHBITS]
#define HASHSIZE (1<<hashBits)
#define HASHMASK (HASHSIZE - 1)

typedef uint32_t node;
typedef uint16_t row;

row * rows;
node * base;
node * hash;

int nttable[512] ;
uint16_t **gInd3 ;
uint32_t *gcount ;
uint16_t *gRows;
long long memusage = 0;
long long memlimit = 0;

row **deepRows;
uint32_t *deepRowIndices;
uint32_t deepQHead, deepQTail, oldDeepQHead;

#ifndef NOCACHE
long long cachesize ;
typedef struct {
   uint16_t *p1, *p2, *p3 ;
   int abn, r ;
} cacheentry;

cacheentry *totalCache;

cacheentry **cache;
#endif

/*
** Representation of vertices.
**
** Each vertex is represented by an entry in the rows[] array.
** That entry consists of the bits of the last row of the vertex's pattern
** concatenated with a number, the "offset" of the vertex.
** The parent of vertex i is formed by adding the offset of i
** with the number in base[i/BASEFACTOR].
**
** If node i has offset -1, the entry is considered to be empty and is skipped.
** This is to deal with the case that base[i/BASEFACTOR] is already set to
** something too far away to deal with via the offset.
**
** qIsEmpty() checks the size of the queue
** enqueue(n,r) adds a new node with last row r and parent n
** dequeue() returns the index of the next unprocessed node
** pop() undoes the previous enqueue operation
** resetQ() clears the queue
**
** ROW(b) returns the last row of b
** PARENT(b) returns the index of the parent of b
*/

#define ROWBITS ((1<<width)-1)
#define BASEBITS (params[P_BASEBITS])
#define BASEFACTOR (1<<BASEBITS)
#define MAXOFFSET ((((row) -1) >> width) - 1)

#define ROW(i) (rows[i] & ROWBITS)
#define ROFFSET(i) (rows[i] >> width)
#define EMPTY(i) (rows[i] == (row)-1)
#define MAKEEMPTY(i) rows[i] = (row)-1
#define PARENT(i) (base[(i)>>BASEBITS]+ROFFSET(i))
#define FIRSTBASE(i) (((i) & ((1<<BASEBITS) - 1)) == 0)

#define MINDEEP ((params[P_MINDEEP]>0) ? params[P_MINDEEP] : period)

void insortRows(uint16_t *r, const uint32_t * const g, const uint32_t n);

void fasterTable(const int *table1, char *table2);
int evolveBit(const int row1, const int row2, const int row3, const char *table);
int evolveRow(const int row1, const int row2, const int row3, const char *table, const int wi, const int s);
int evolveRowHigh(const int row1, const int row2, const int row3, const int bits, const char *table, const int wi);
int evolveRowLow(const int row1, const int row2, const int row3, const int bits, const char *table, const int s);
void checkParams(const char *rule, int *tab, const int *params, const int pf, const int irf, const int ldf);
int gcd(int a, int b);
void echoParams(const int * params, const char *rule, const int period);

int fwdOff[MAXPERIOD], backOff[MAXPERIOD], doubleOff[MAXPERIOD], tripleOff[MAXPERIOD];
void makePhases(const int period, const int offset, int *fwdOff, int *backOff, int *doubleOff, int *tripleOff);

unsigned char *causesBirth;

char rule[256] = "B3/S23";
char dumpRoot[256] = "dump";
char nttable2[512] ;

uint16_t *makeRow(int row1, int row2) ;

/*                                  */
/*      function: getoffset         */
/*         input: int row1, row2    */
/* global  input: uint16_t **gInd3  */
/* global  input: int width         */
/*        output: uint16_t* r       */
/*                                  */
uint16_t *getoffset(int row1, int row2) {
   uint16_t *r = gInd3[(row1 << width) + row2] ;
   if (r == 0)
      r = makeRow(row1, row2) ;
   return r ;
}

/*                                      */
/*      function: getoffsetcount        */
/*         input: int row1, row2, row3  */
/*         input: uint16_t **p          */
/*         input: int *n                */
/*                                      */
void getoffsetcount(int row1, int row2, int row3, uint16_t **p, int *n) {
   uint16_t *theRow = getoffset(row1, row2) ;
   *p = theRow + theRow[row3] ;
   *n = theRow[row3+1] - theRow[row3] ;
}

int *gWorkConcat ;      /* gWorkConcat to be parceled out between threads */
int *rowHash ;
uint16_t *valorder ;
void genStatCounts(const int symmetry, const int wi, const char *table2, uint32_t *gc);

/*                                              */
/*      function: makeTables                    */
/* global    i/o: unsigned char *causesBirth    */
/* global  input: int width                     */
/* global    i/o: uint16_t **gInd3              */
/* global    i/o: int *rowhash                  */
/* global    i/o: uint32_t *gcount              */
/* global    i/o: long long memusage            */
/* global  input: char *nttable2                */
/* global  input: int *params                   */
/* global    i/o: int *gWorkConcat              */
/* global    i/o: uint16_t* valorder            */
/*                                              */
void makeTables() {
   causesBirth = (unsigned char*)malloc((long long)sizeof(*causesBirth)<<width);
   gInd3 = (uint16_t **)calloc(sizeof(*gInd3),(1LL<<(width*2))) ;
   rowHash = (int *)calloc(sizeof(int),(2LL<<(width*2))) ;
   memset(rowHash, -1, sizeof(int)*(2LL<<(width*2)));

   gcount = (uint32_t *)calloc(sizeof(*gcount), (1LL << width));
   memusage += (sizeof(*gInd3)+2*sizeof(int)) << (width*2) ;
   uint32_t i;
   for(i = 0; i < 1 << width; ++i)
       causesBirth[i] = (evolveRow(i, 0, 0, nttable2, width, params[P_SYMMETRY]) ? 1 : 0);

   gWorkConcat = (int *)calloc(sizeof(int), (3LL*params[P_NUMTHREADS])<<width);
   if (params[P_REORDER] == 1)
      genStatCounts(params[P_SYMMETRY], width, nttable2, gcount) ;
   if (params[P_REORDER] == 2)
      for (int i=1; i<1<<width; i++)
         gcount[i] = 1 + gcount[i & (i - 1)] ;
   gcount[0] = 0xffffffff;  /* Maximum value so empty row is chosen first */
   valorder = (uint16_t *)calloc(sizeof(uint16_t), 1LL << width) ;
   for (int i=0; i<1<<width; i++)
      valorder[i] = (1<<width)-1-i ;
   if (params[P_REORDER] != 0)
      insortRows(valorder, gcount, 1<<width) ;
   for (int row2=0; row2<1<<width; row2++)
      makeRow(0, row2) ;
}

/*                                  */
/*      function: hasRow            */
/*         input: uint16_t *theRow  */
/*         input: int siz           */
/*        output: unsigned int h    */
/*                                  */
unsigned int hashRow(uint16_t *theRow, int siz) {
   unsigned int h = 0 ;
   for (int i=0; i<siz; i++)
      h = h * 3 + theRow[i] ;
   return h ;
}

/*                                  */
/*      function: makeRow           */
/*         input: int row1, row2    */
/* global  input: int width         */
/* global  input: char *nttable2    */
/* global  input: int *params       */
/* global    i/o: uint16_t **gInd3  */
/*        output: uint16_t *theRow  */
/*                                  */
uint16_t *makeRow(int row1, int row2) {
   int good = 0 ;
   /* Set up gWork for this particular thread */
   int *gWork = gWorkConcat + ((3LL * omp_get_thread_num()) << width);
   int *gWork2 = gWork + (1 << width) ;
   int *gWork3 = gWork2 + (1 << width) ;
   if (width < 4) {
      for (int row3=0; row3<1<<width; row3++)
         gWork3[row3] = evolveRow(row1, row2, row3, nttable2, width, params[P_SYMMETRY]) ;
   } else {
      int lowbitcount = (width >> 1) + 1 ;
      int hibitcount = ((width + 1) >> 1) + 1 ;
      int hishift = lowbitcount - 2 ;
      int lowcount = 1 << lowbitcount ;
      for (int row3=0; row3<1<<lowbitcount; row3++)
         gWork2[row3] = evolveRowLow(row1, row2, row3, lowbitcount-1, nttable2, params[P_SYMMETRY]);
      for (int row3=0; row3<1<<width; row3 += 1<<hishift)
         gWork2[lowcount+(row3>>hishift)] =
                        evolveRowHigh(row1, row2, row3, hibitcount-1, nttable2, width);
      for (int row3=0; row3<1<<width; row3++)
         gWork3[row3] = gWork2[row3 & ((1<<lowbitcount) - 1)] |
                        gWork2[lowcount+(row3 >> hishift)] ;
   }
   for (int row3i = 0; row3i < 1<<width; row3i++) {
      int row3 = valorder[row3i] ;
      int row4 = gWork3[row3] ;
      if (row4 < 0)
         continue ;
      gWork2[good] = row3 ;
      gWork[good++] = row4 ;
   }
   
   uint16_t *theRow = (uint16_t*)calloc((1+(1<<width)+good), sizeof(uint16_t)) ;
   theRow[0] = 1 + (1 << width) ;
   
   for (int row3=0; row3 < good; row3++)
      theRow[gWork[row3]]++ ;
   theRow[1<<width] = 0 ;
   for (int row3=0; row3 < (1<<width); row3++)
      theRow[row3+1] += theRow[row3] ;
   for (int row3=good-1; row3>=0; row3--) {
      int row4 = gWork[row3] ;
      theRow[--theRow[row4]] = gWork2[row3] ;
   }

   unsigned int h = hashRow(theRow, 1+(1<<width)+good) ;
   h &= (2 << (2 * width)) - 1 ;

   /* All operations that read or write to row,                 */
   /* rowHash, and gInd3 must be included in a critical region. */
   #pragma omp critical(updateTable)
   {
      while (1) {
         if (rowHash[h] == -1) {
            rowHash[h] = (row1 << width) + row2 ;
            break ;
         }
         /* Maybe two different row12s result in the exact same rows for the */
         /* lookup table. This prevents two different threads from trying to */
         /* build the same part of the lookup table.                         */
         if (memcmp(theRow, gInd3[rowHash[h]], 2*(1+(1<<width)+good)) == 0) {
            free(theRow);
            theRow = gInd3[rowHash[h]] ;
            /* unbmalloc(1+(1<<width)+good) ; */
            break ;
         }
         h = (h + 1) & ((2 << (2 * width)) - 1) ;
      }
      
      gInd3[(row1<<width)+row2] = theRow ;
   }
   
/*
 *   For debugging:
 *
   printf("R") ;
   for (int i=0; i<1+(1<<width)+good; i++)
      printf(" %d", theRow[i]) ;
   printf("\n") ;
   fflush(stdout) ;
 */
 
   return theRow ;
}

/* ====================================================== */
/*  Hash table for detecting equivalent partial patterns  */
/* ====================================================== */
/*                              */
/*      function: resetHash     */
/* global    i/o: node *hash    */
/* global  input: int *params   */
/*                              */
void resetHash() { if (hash != 0) memset(hash,0,4*HASHSIZE); }

int hashPhase = 0;

/*                                  */
/*      function: hashFunction      */
/*         input: node b            */
/*         input: row r             */
/* global  input: int nRowsInState  */
/* global  input: int *params       */
/* global  input: row *rows         */
/* global  input: int width         */
/* global  input: node *base        */
/*        output: long h            */
/*                                  */
static inline long hashFunction(node b, row r) {
   long h = r;
   int i;
   for (i = 0; i < nRowsInState; i++) {
      h = (h * 269) + ROW(b);
      b = PARENT(b);
   }
   h += (h>>16)*269;
   h += (h>>8)*269;
   return h & HASHMASK;
}

/*                                  */
/*      function: same              */
/*         input: node p, q         */
/*         input: row r             */
/* global  input: int nRowsInState  */
/* global  input: int *params       */
/* global  input: row *rows         */
/* global  input: int width         */
/* global  input: node *base        */
/*                                  */
/* test if q+r is same as p */
static inline int same(node p, node q, row r) {
   int i;
   for (i = 0; i < nRowsInState; i++) {
      if (p >= QSIZE || q >= QSIZE || EMPTY(p) || EMPTY(q)) return 0;   /* sanity check */
      if (ROW(p) != r) return 0;
      p = PARENT(p);
      r = ROW(q);
      q = PARENT(q);
   }
   return 1;
}

/*                                  */
/*      function: isVisited         */
/*         input: node b            */
/*         input: row r             */
/* global  input: int nRowsInState  */
/* global  input: int *params       */
/* global  input: row *rows         */
/* global  input: int width         */
/* global  input: node *base        */
/* global  input: node *hash        */
/*        output: int               */
/*                                  */
/* test if we've seen this child before */
static inline int isVisited(node b, row r) {
   if (same(0,b,r)) return 1;
   if (hash != 0) {
      int hashVal = hashFunction(b,r);
      node hashNode = hash[hashVal];
      if (hashNode == 0) return 0;
      if (same(hashNode,b,r)){
         return 1;
      }
   }
   return 0;
}

/*                                  */
/*      function: setVisited        */
/*         input: node b            */
/* global    i/o: node *hash        */
/* global  input: int nRowsInState  */
/* global  input: int *params       */
/* global  input: row *rows         */
/* global  input: int width         */
/* global  input: node *base        */
/*                                  */
/* set node (NOT child) to be visited */
static inline void setVisited(node b) {
   if (hash != 0) hash[ hashFunction(PARENT(b),ROW(b)) ] = b;
}

/* ============================================== */
/*  Output patterns found by successful searches  */
/* ============================================== */

#define MAXRLELINEWIDTH 63
int RLEcount = 0;
int RLElineWidth = 0;
char RLEchar;
char * patternBuf;

/*                                  */
/*      function: bufRLE            */
/*         input: char c            */
/* global    i/o: int RLEcount      */
/* global    i/o: int RLElineWidth  */
/* global    i/o: char RLEchar      */
/* global    i/o: char *patternBuf  */
/*                                  */
void bufRLE(char c) {
  if (RLEcount > 0 && c != RLEchar) {
      if (RLElineWidth++ >= MAXRLELINEWIDTH) {
         if (RLEchar != '\n') sprintf(patternBuf+strlen(patternBuf),"\n");
         RLElineWidth = 0;
      }
    if (RLEcount == 1) strncat(patternBuf,&RLEchar,1);
    else {
       sprintf(patternBuf+strlen(patternBuf),"%d%c", RLEcount, RLEchar);
       RLElineWidth ++;
       if (RLEcount > 9) RLElineWidth++;
    }
    RLEcount = 0;
    if (RLEchar == '\n') RLElineWidth = 0;
  }
  if (c != '\0') {
    RLEcount++;
    RLEchar = c;
  } else RLElineWidth = 0;
}

/*                                      */
/*      function: bufRow                */
/*         input: unsigned long rr, r   */
/*         input: int shift             */
/* inherited from bufRLE                */
/* global    i/o: int RLEcount          */
/* global    i/o: int RLElineWidth      */
/* global    i/o: char RLEchar          */
/* global    i/o: char *patternBuf      */
/*                                      */
void bufRow(unsigned long rr, unsigned long r, int shift) {
   while (r | rr) {
      if (shift == 0)
         bufRLE(r & 1 ? 'o' : 'b');
      else shift--;
      r >>= 1;
      if (rr & 1) r |= (1<<31);
      rr >>= 1;
   }
   bufRLE('$');
}

/*                                  */
/*      function: safeShift         */
/*         input: unsigned long r   */
/*         input: int i             */
/*        output: unsigned long rr  */
/*                                  */
/* Avoid Intel shift bug */
static inline unsigned long
safeShift(unsigned long r, int i)
{
    unsigned long rr = r;
    while (i>16) { rr >>= 16; i-=16;}
    return (rr>>i);
}

int sxsAllocRows =  0;
unsigned long * sxsAllocData;
unsigned long * sxsAllocData2;
int oldnrows = 0;
unsigned long * oldsrows;
unsigned long * oldssrows;

/* Buffers RLE into patternBuf; returns 1 if pattern successfully buffered */ 
int bufferPattern(node b, row *pRows, int nodeRow, uint32_t lastRow, int printExpected){
   node c;
   int nrows = 0;
   int skewAmount = 0;
   int swidth;
   int sxsNeeded;
   int p, i, j, margin;
   unsigned long *srows, *ssrows;

   uint32_t currRow = lastRow;
   int nDeepRows = 0;
   int nodeDiff;

   if(pRows != NULL){
      while(pRows[currRow] == 0){
         if(currRow == 0){
            if(!printExpected) return 0;
            printf("Success called on search root!\n");
            aborting = 1;
            return 0;
         }
         currRow--;
      }
      nDeepRows = (currRow / period) - 1;
      nodeDiff = nodeRow - period - (currRow%period);
      nodeRow -= nodeDiff;

      for(j = 0; j < nodeDiff; j++){
         b = PARENT(b);
      }
      currRow = currRow - period + 1;
      nrows = nDeepRows;
      
   }
   else{
      /* shift until we find nonzero row.
         then shift PERIOD-1 more times to get leading edge of glider */
      while (ROW(b) == 0) {
         b = PARENT(b);
         if (b == 0) {
            if(!printExpected) return 0;
            printf("Success called on search root!\n");
            aborting = 1;
            return 0;
         }
      }
   }
   if(nrows < 0) nrows = 0;
   
   for (p = 0; p < period-1; p++) b = PARENT(b);
   if (b == 0) {
      if(!printExpected) return 0;
      printf("Success called on search root!\n");
      aborting = 1;
      return 0;
   }
   
   /* count rows */
   c = b;
   while (c != 0) {
      for (p = 0; p < period; p++)
         c = PARENT(c);
      nrows++;
   }
   
   /* build data structure of rows so we can reduce width etc */
   sxsNeeded = nrows+MAXWIDTH+1;
   if (!sxsAllocRows)
   {
      sxsAllocRows = sxsNeeded;
      sxsAllocData = (unsigned long*)malloc(sxsAllocRows * sizeof(unsigned long));
      sxsAllocData2 = (unsigned long*)malloc(sxsAllocRows * sizeof(unsigned long));
      oldsrows = (unsigned long*)calloc(sxsAllocRows, sizeof(unsigned long));
      oldssrows = (unsigned long*)calloc(sxsAllocRows, sizeof(unsigned long));
      patternBuf = (char*)malloc(((2 * MAXWIDTH + 4) * sxsAllocRows + 300) * sizeof(char));
   }
   else if (sxsAllocRows < sxsNeeded)
   {
      sxsAllocRows = sxsNeeded;
      sxsAllocData = (unsigned long*)realloc(sxsAllocData, sxsAllocRows * sizeof(unsigned long));
      sxsAllocData2 = (unsigned long*)realloc(sxsAllocData2, sxsAllocRows * sizeof(unsigned long));
   }
   srows  = sxsAllocData;
   ssrows = sxsAllocData2;
   
   for (i = 0; i <= nrows+MAXWIDTH; i++) srows[i]=ssrows[i]=0;
   for (i = nrows - 1; i >= 0; i--) {
      row r;
      if(nDeepRows > 0){
         r = pRows[currRow];
         currRow -= period;
         nDeepRows--;
      }
      else{
         r = ROW(b);
         for (p = 0; p < period; p++) {
            b = PARENT(b);
         }
      }
      switch(mode) {
         case asymmetric:
            srows[i] = r;
            break;

         case odd:
            srows[i] = r << (MAXWIDTH - 1);
            ssrows[i] = r >> (32 - (MAXWIDTH - 1));
            for (j = 1; j < MAXWIDTH; j++)
               if (r & (1<<j))
                  srows[i] |= 1 << (MAXWIDTH - 1 - j);
            break;

         case even:
            srows[i] = r << MAXWIDTH;
            ssrows[i] = r >> (32 - MAXWIDTH);
            for (j = 0; j < MAXWIDTH; j++)
               if (r & (1<<j))
                  srows[i] |= 1 << (MAXWIDTH - 1 - j);
            break;

         case gutter:
            srows[i] = r << (MAXWIDTH + 1);
            ssrows[i] = r >> (32 - (MAXWIDTH + 1));
            for (j = 0; j < MAXWIDTH; j++)
               if (r & (1<<j))
                  srows[i+skewAmount] |= 1 << (MAXWIDTH - 1 - j);
            break;

         default:
            printf("Unexpected mode in success!\n");
            aborting = 1;
            return 0;
      }
   }
   
   /* normalize nrows to only include blank rows */
   nrows += MAXWIDTH;
   while (nrows>0) {
       if (srows[nrows-1] == 0 && ssrows[nrows-1] == 0)
           nrows--;
       else
           break;
   }
   while (srows[0] == 0 && ssrows[0] == 0 && nrows>0) {
      srows++;
      ssrows++;
      nrows--;
   }
   
   /* sanity check: are all rows empty? */
   int allEmpty = 1;
   for(i = 0; i < nrows; i++){
      if(srows[i]){
         allEmpty = 0;
         break;
      }
   }
   if(allEmpty) return 0;
   
   /* make at least one row have nonzero first bit */
   i = 0;
   while ((srows[i] & 1) == 0) {
      for (i = 0; (srows[i] & 1) == 0 && i < nrows; i++) { }
      if (i == nrows) {
         for (i = 0; i < nrows; i++) {
            srows[i] >>= 1;
            if (ssrows[i] & 1) srows[i] |= (1<<31);
            ssrows[i] >>= 1;
         }
         i = 0;
      }
   }
   
   swidth = 0;
   for (i = 0; i < nrows; i++)
      while (safeShift(ssrows[i],swidth))
         swidth++;
   if (swidth) swidth += 32;
   for (i = 0; i < nrows; i++)
      while (safeShift(srows[i],swidth))
         swidth++;
   
      
   /* compute margin on other side of width */
   margin = 0;

   /* make sure we didn't just output the exact same pattern (happens a lot for puffer) */
   if(printExpected){
      if (nrows == oldnrows) {
         int different = 0;
         for (i = 0; i < nrows && !different; i++)
            different = (srows[i] != oldsrows[i] || ssrows[i] != oldssrows[i]);
         if (!different) return 0;
      }
      
      /* replace previous saved rows with new rows */
      oldnrows = nrows;
      oldsrows = (unsigned long*)realloc(oldsrows, sxsAllocRows * sizeof(unsigned long));
      oldssrows = (unsigned long*)realloc(oldssrows, sxsAllocRows * sizeof(unsigned long));
      memcpy(oldsrows, srows, nrows * sizeof(unsigned long));
      memcpy(oldssrows, ssrows, nrows * sizeof(unsigned long));
   }
   
   /* Buffer output */
   patternBuf = (char*)realloc(patternBuf, ((2 * MAXWIDTH + 4) * sxsAllocRows + 300) * sizeof(char));
   
   sprintf(patternBuf,"x = %d, y = %d, rule = %s\n", swidth - margin, nrows, rule);
      
   while (nrows-- > 0) {
      if (margin > nrows) bufRow(ssrows[nrows], srows[nrows], margin - nrows);
      else bufRow(ssrows[nrows], srows[nrows], 0);
   }
   RLEchar = '!';
   bufRLE('\0');
   sprintf(patternBuf+strlen(patternBuf),"\n");
   
   if(printExpected){
      numFound++;
      if(params[P_NUMSHIPS] > 0){
         if(--params[P_NUMSHIPS] == 0) aborting = 3;  /* use 3 to flag that we reached ship limit */
      }
   }
   
   return 1;
}

void successfulPattern(node b, row *pRows, int nodeRow, uint32_t lastRow){
   if(bufferPattern(b, pRows, nodeRow, lastRow, 1))
      printf("\n%s\n",patternBuf);
   fflush(stdout);
}

/* Is this a node at which we can stop? */
int terminal(node n){
   int p;

   for (p = 0; p < period; p++) {   /* last row in each phase must be zero */
      if (ROW(n) != 0) return 0;
      n = PARENT(n);
   }

   for (p = 0; p < period; p++) {
      if(causesBirth[ROW(n)]) return 0;
      n = PARENT(n);
   }
   return 1;
}


/* ================================================ */
/*  Queue of partial patterns still to be examined  */
/* ================================================ */

/* SU patch */
node qHead,qTail;

/* PATCH queue dimensions required during save/restore */
node qStart; /* index of first node in queue */
node qEnd;   /* index of first unused node after end of queue */

/* Maintain phase of queue nodes.  After dequeue(), the global variable phase
   gives the phase of the dequeued item.  If the queue is compacted, this information
   needs to be reinitialized by a call to rephase(), after which phase will not be
   valid until the next call to dequeue().  Variable nextRephase points to the next
   node for which dequeue will need to increment the phase. Phase is not maintained
   when treating queue as a stack (using pop()) -- caller must do it in that case.
   It's ok to change phase since we maintain a separate copy in queuePhase. */

int queuePhase = 0;
node nextRephase = 0;
void rephase() {
   node x, y;
   while (qHead < qTail && EMPTY(qHead)) qHead++;   /* skip past empty queue cells */
   x = qHead;   /* find next item in queue */
   queuePhase = period - 1;
   while (x != 0) {
      x = PARENT(x);
      queuePhase++;
   }
   queuePhase %= period;

   /* now walk forward through queue finding breakpoints between each generation
      invariants: y is always the first in its generation */
   x = 0; y = 0;
   while (y <= qHead) {
      ++x;
      if (x >= qTail || (!EMPTY(x) && PARENT(x) >= y)) y = x;
   }
   nextRephase = y;
}

/* phase of an item on the queue */
int peekPhase(node i) {
   return (i < nextRephase? queuePhase : (queuePhase+1)%period);
}

/* Test queue status */
static inline int qIsEmpty() {
   while (qHead < qTail && EMPTY(qHead)){
      ++qHead;
      ++deepQHead;
   }
   return (qTail == qHead);
}

void qFull() {
    if (aborting != 2) {
      printf("Exceeded %d node limit, search aborted\n", QSIZE);
      fflush(stdout);
      aborting = 2;
   }
}

static inline void enqueue(node b, row r) {
   node tempQTail = qTail;
   node i = qTail++;
   if (i >= QSIZE) qFull();
   else if (FIRSTBASE(i)) {
      base[i>>BASEBITS] = b;
      rows[i] = r;
   } else {
      long o = b - base[i>>BASEBITS];
      if (o < 0 || o >(long) MAXOFFSET) {   /* offset out of range */
         while (!FIRSTBASE(i)) {
            rows[i] = -1;
            i = qTail++;
            if (i >= QSIZE) qFull();
         }
         base[i>>BASEBITS] = b;
         rows[i] = r;
      } else rows[i] = (o << width) + r;
   }
   
   /* update tail of parallel queue, but don't set value */
   deepQTail += qTail - tempQTail;
   deepRowIndices[deepQTail] = 0;
}

static inline node dequeue() {
   oldDeepQHead = deepQHead;  /* Save old parallel queue head for use in process() */
   while (qHead < qTail && EMPTY(qHead)){
      ++qHead;
      ++deepQHead;
   }
   if (qHead >= nextRephase) {
      queuePhase = (queuePhase+1)%period;
      nextRephase = qTail;
   }
   phase = queuePhase;
   ++deepQHead;
   return qHead++;
}

static inline void pop() {
   qTail--;
   while (qTail > qHead && EMPTY(qTail-1)) qTail--;
}

void resetQ() { qHead = qTail = 0; deepQHead = deepQTail = 0; }

static inline int qTop() { return qTail - 1; }

/* =================== */
/*  Dump search state  */
/* =================== */

int dumpNum = 1;
char dumpFile[512];

Dump dumpFlag = unknown;    /* Dump status flags, possible values follow */

FILE * openDumpFile()
{
   FILE * fp;

   while (dumpNum < 100000)
   {
      sprintf(dumpFile,"%s%05d",dumpRoot,dumpNum++);
      if ((fp=fopen(dumpFile,"r")))
         fclose(fp);
      else
         return fopen(dumpFile,"w");
   }
   return (FILE *) 0;
}

void dumpState()
{
   FILE * fp;
   uint32_t i,j;
   dumpFlag = failure;
   if (!(fp = openDumpFile())) return;
   fprintf(fp,"%lu\n",FILEVERSION);
   fprintf(fp,"%s\n",rule);
   fprintf(fp,"%s\n",dumpRoot);
   for (i = 0; i < NUM_PARAMS; ++i)
      fprintf(fp,"%d\n",params[i]);
   fprintf(fp,"%d\n",width);
   fprintf(fp,"%d\n",period);
   fprintf(fp,"%d\n",offset);
   fprintf(fp,"%d\n",mode);
   fprintf(fp,"%u\n",qHead-qStart);
   fprintf(fp,"%u\n",qEnd-qStart);
   for (i = qStart; i < qEnd; ++i)
      fprintf(fp,"%u\n",rows[i]);
   for (i = 0; i < QSIZE; ++i){
      if (deepRowIndices[i]){
         if (deepRowIndices[i] > 1){
            for (j = 0; j < deepRows[deepRowIndices[i]][0] + 1 + 2; ++j){
               fprintf(fp,"%u\n",deepRows[deepRowIndices[i]][j]);
            }
         }
         else {
            fprintf(fp,"0\n");
            j = 0;
            while (deepRowIndices[i] <= 1 && i < QSIZE){
               if (deepRowIndices[i] == 1) ++j;
               ++i;
            }
            fprintf(fp,"%u\n",j);
            if (i == QSIZE) break;
            --i;
         }
      }
   }
   fclose(fp);
   dumpFlag = success;
}

/* ================================= */
/*  Compaction of nearly full queue  */
/* ================================= */

void putnum(long n) {
   char suffix;
   if (n >= 1000000) {
      n /= 100000;
      suffix = 'M';
   } else if (n >= 1000) {
      n /= 100;
      suffix = 'k';
   } else {
      printf("%ld", n);
      return;
   }

   if (n >= 100) printf("%ld", n/10);
   else printf("%ld.%ld", n/10, n%10);
   putchar(suffix);
}

long currentDepth() {
   long i;
   node x;
   x = qTail - 1;
   i = 1;
   while (x != 0) {
      x = PARENT(x);
      i++;
   }
   return i;
}

/*
** doCompact() now has two parts.  The first part compresses
** the queue.  The second part consists of the last loop which
** converts parent bits to back parent pointers.  The search
** state may be saved in between.  The queue dimensions, which
** were previously saved in local variables are saved in globals.
*/

void doCompactPart1()
{
   node x,y;
   qEnd = qTail;
   
   /* make a pass backwards from the end finding unused nodes at or before qHead */
   x = qTail - 1;
   y = qHead - 1;
   while (y > 0) {
      /* invariants: everything after y is still active.
                     everything after x points to something after y.
                     x is nonempty and points to y or something before y.
                     so, if x doesn't point to y, y must be unused and can be removed. */
      if (!EMPTY(y)) {
         if (y > PARENT(x)) rows[y] = -1;
         else while (EMPTY(x) || PARENT(x) == y) x--;
      }
      y--;
   }
   
   /* make a pass forwards converting parent pointers to offset from prev parent ptr.
      note that after unused nodes are eliminated, all these offsets are zero or one. */
   y = 0;
   for (x = 0; x < qTail; x++) if (!EMPTY(x)) {
      if (PARENT(x) == y) rows[x] = ROW(x);
      else {
         y = PARENT(x);
         rows[x] = (1<<width) + ROW(x);
      }
   }
   
   /*
      Make a pass backwards compacting gaps.
   
      For most times we run this, it could be combined with the next phase, but
      every once in a while the process of repacking the remaining items causes them
      to use *MORE* space than they did before they were repacked (because of the need
      to leave empty space when OFFSET gets too big) and without this phase the repacked
      stuff overlaps the not-yet-repacked stuff causing major badness.
      
      For this phase, y points to the current item to be repacked, and x points
      to the next free place to pack an item.
   */
   x = y = qTail-1;
   for (;;) {
      if (qHead == y) qHead = x;
      if (!EMPTY(y)) {
         rows[x] = rows[y];
         x--;
      }
      if (y-- == 0) break;    /* circumlocution for while (y >= 0) because x is unsigned */
   }
   qStart = ++x;     /* mark start of queue */
}

void doCompactPart2()
{
   node x,y;
   uint32_t i, j, k;
   
   /*
      Make a pass forwards converting parent bits back to parent pointers.
      
      For this phase, x points to the current item to be repacked, and y points
      to the parent of the previously repacked item.
      After the previous pass, x is initialized to first nonempty item,
      and all items after x are nonempty. 
    */
   qTail = 0; y = 0;
   resetHash();
   for (x = qStart; x < qEnd; x++) {
      if (ROFFSET(x)) {   /* skip forward to next parent */
         y++;
         while (EMPTY(y)) y++;
      }
      enqueue(y,ROW(x));
      //if (aborting) return;    /* why is this here? value of aborting is not changed by enqueue(). */
      if (qHead == x) qHead = qTail - 1;
      setVisited(qTail - 1);
   }
   rephase();
   
   /* Repack and respace depth-first extension queue to match node queue */
   j = QSIZE - 1;
   for(i = QSIZE; i > 0; --i){
      if(deepRowIndices[i - 1]){
         deepRowIndices[j] = deepRowIndices[i - 1];
         deepRowIndices[i - 1] = 0;
         --j;
      }
   }
   
   /* Sanity check: extension queue should not take up all available space */
   if(deepRowIndices[0]){
      fprintf(stderr,"Error: extension queue has too many elements.\n");
      exit(1);
   }
   
   i = 0;
   j = 0;
   while(!deepRowIndices[j] && j < QSIZE) ++j;
   for(x = qHead; x < qTail && j < QSIZE; ++x){
      if(EMPTY(x)){
         ++i;
         continue;
      }
      
      deepRowIndices[i] = deepRowIndices[j];
      
      /* Sanity check: do the extension rows match the node rows? */
      if(deepRowIndices[j] > 1){
         y = x;
         for(k = 0; k < 2*period; ++k){
            uint16_t startRow = deepRows[deepRowIndices[j]][1] + 1;
            if(deepRows[deepRowIndices[j]][startRow - k] != ROW(y)){
               fprintf(stderr, "Warning: non-matching rows detected at node %u in doCompactPart2()\n",x);
               free(deepRows[deepRowIndices[j]]);
               deepRows[deepRowIndices[j]] = 0;
               deepRowIndices[i] = 0;
               break;
            }
            y = PARENT(y);
         }
      }
      if(j > i) deepRowIndices[j] = 0;
      ++i;
      ++j;
   }
   for(j = qTail - qHead; j < QSIZE; ++j){
      deepRowIndices[j] = 0;
   }
   deepQHead = 0;
   deepQTail = qTail - qHead;
}

void doCompact()
{
   /* make sure we still have something left in the queue */
   if (qIsEmpty()) {
      qTail = qHead = 0;   /* nothing left, make an extremely compact queue */
      return;
   }
   /* First loop of part 1 requires qTail-1 to be non-empty.  Make it so */
   while(EMPTY(qTail-1))
      qTail--;
   
   doCompactPart1();
   if (dumpFlag == pending) dumpState();
   doCompactPart2();
}

/* ================= */
/*  Lookahead cache  */
/* ================= */

#ifndef NOCACHE
int getkey(uint16_t *p1, uint16_t *p2, uint16_t *p3, int abn) {
#ifndef QSIMPLE
   if(params[P_CACHEMEM] == 0) return 0;
#endif
   unsigned long long h = (unsigned long long)p1 +
      17 * (unsigned long long)p2 + 257 * (unsigned long long)p3 +
      513 * abn ;
   h = h + (h >> 15) ;
   h &= (cachesize-1) ;
   cacheentry *ce = &(cache[omp_get_thread_num()][h]) ;
   if (ce->p1 == p1 && ce->p2 == p2 && ce->p3 == p3 && ce->abn == abn)
      return -2 + ce->r ;
   ce->p1 = p1 ;
   ce->p2 = p2 ;
   ce->p3 = p3 ;
   ce->abn = abn ;
   return h ;
}
#endif

#ifndef NOCACHE
void setkey(int h, int v) {
#ifndef QSIMPLE
   if(params[P_CACHEMEM])
#endif
      cache[omp_get_thread_num()][h].r = v ;
}
#endif

/* ========================== */
/*  Primary search functions  */
/* ========================== */

void process(node theNode);
int depthFirst(node theNode, uint16_t howDeep, uint16_t **pInd, int *pRemain, row *pRows);

static void deepen(){
   node i;

   /* compute amount to deepen, apply reduction if too deep */
#ifdef PATCH07
   timeStamp();
#endif
   printf("Queue full");
   i = currentDepth();
   if (i >= params[P_LASTDEEP]) deepeningAmount = MINDEEP;
   else deepeningAmount = params[P_LASTDEEP] + MINDEEP - i;   /* go at least MINDEEP deeper */

   params[P_LASTDEEP] = i + deepeningAmount;

   /* start report of what's happening */
   printf(", depth %ld, deepening %d, ", (long int) i, deepeningAmount);
   putnum(qTail - qHead);
   printf("/");
   putnum(qTail);
   fflush(stdout);


   /* go through queue, deepening each one */
   
   #pragma omp parallel
   {
      node j;
      uint16_t **pInd;
      int *pRemain;
      row *pRows;
      
      pInd = (uint16_t**)calloc((deepeningAmount + 4 * params[P_PERIOD]), (long long)sizeof(*pInd));
      pRemain = (int*)calloc((deepeningAmount + 4 * params[P_PERIOD]), (long long)sizeof(*pRemain));
      pRows = (row*)calloc((deepeningAmount + 4 * params[P_PERIOD]), (long long)sizeof(*pRows));
      
      #pragma omp for schedule(dynamic, CHUNK_SIZE)
      for (j = qHead; j < qTail; ++j) {
         if (!EMPTY(j) && !depthFirst(j, (uint16_t)deepeningAmount, pInd, pRemain, pRows))
            MAKEEMPTY(j);
      }
      free(pInd);
      free(pRemain);
      free(pRows);
   }
   
   if (deepeningAmount > period) --deepeningAmount; /* allow for gradual depth reduction */
   
   /* before reporting new queue size, shrink tree back down */
   printf(" -> ");
   fflush(stdout);
   
   /* signal time for dump */
   if (params[P_CHECKPOINT]) dumpFlag = pending;
   
   doCompact();
   
   /* now finish report */
   putnum(qTail - qHead);
   printf("/");
   putnum(qTail);
   printf("\n");
   
   /* Report successful/unsuccessful dump */
   if (dumpFlag == success)
   {
#ifdef PATCH07
      timeStamp();
#endif
       printf("State dumped to %s\n",dumpFile);
       /*analyse();
       if (chainWidth)
           printf("[%d/%d]\n",chainDepth,chainWidth+1);
       else
           printf("[%d/-]\n",chainDepth);*/
   }
   else if (dumpFlag == failure)
   {
#ifdef PATCH07
      timeStamp();
#endif
      printf("State dump unsuccessful\n");
   }
   
   fflush(stdout);
}

static void breadthFirst()
{
   while (!aborting && !qIsEmpty()) {
      if (qTail - qHead >= (1<<params[P_DEPTHLIMIT]) || qTail >= QSIZE - QSIZE/16 ||
          qTail >= QSIZE - (deepeningAmount << 2)) deepen();
      else process(dequeue());
   }
}

void saveDepthFirst(node theNode, uint16_t startRow, uint16_t howDeep, row *pRows){
   uint32_t theDeepIndex;
   #pragma omp critical(findDeepIndex)
   {
      theDeepIndex = 2;
      while(deepRows[theDeepIndex] && theDeepIndex < 1 << (params[P_DEPTHLIMIT] + 1)) ++theDeepIndex;
      if(theDeepIndex == 1 << (params[P_DEPTHLIMIT] + 1)){
         fprintf(stderr,"Error: no available extension indices.\n");
         aborting = 1;
      }
      if(!aborting){
         deepRows[theDeepIndex] = (row*)calloc( startRow + howDeep + 1 + 2,
                                                sizeof(**deepRows) );
      }
   }
   if(aborting) return;
   
   memcpy( deepRows[theDeepIndex] + 2,
           pRows,
           (startRow + howDeep + 1) * (long long)sizeof(**deepRows) );
   
   deepRows[theDeepIndex][0] = startRow + howDeep;
   deepRows[theDeepIndex][1] = startRow;
   
   deepRowIndices[deepQHead + theNode - qHead] = theDeepIndex;
}


/* ========================= */
/*  Preview partial results  */
/* ========================= */

static void preview(int allPhases) {
   node i,j,k;
   int ph;

   for (i = qHead; (i<qTail) && EMPTY(i); i++);
   for (j = qTail-1; (j>i) && EMPTY(j); j--);
   if (j<i) return;
   
   while (j>=i && !aborting) {
      if (!EMPTY(j)) {
         successfulPattern(j, NULL, 0, 0);
         if (allPhases == 0) {
            k=j;
            for (ph = 1; ph < period; ph++) {
               k=PARENT(k);
               successfulPattern(k, NULL, 0, 0);
            }
         }
      }
      j--;
   }
}

/* ============================================================ */
/*  Check parameters for validity and exit if there are errors  */
/* ============================================================ */

int splitNum = 0;
int loadDumpFlag = 0;
int previewFlag = 0;
int initRowsFlag = 0;
int newLastDeep = 0;


/* ============================ */
/*  Load saved state from file  */
/* ============================ */

char * loadFile;

void loadState(char * loadFile){
   FILE * fp;
   uint32_t i, j;
   
   fp = fopen(loadFile, "r");
   if (!fp) loadFail(loadFile);
   
   loadUInt(fp, loadFile);                                 /* skip file version */
   fscanf(fp, "%*[^\n]\n");                                /* skip rule */
   fscanf(fp, "%*[^\n]\n");                                /* skip dump root */
   for (i = 0; i < NUM_PARAMS; ++i) loadInt(fp, loadFile); /* skip parameters */
   
   /* Load / initialise globals */
   width          = loadInt(fp, loadFile);
   period         = loadInt(fp, loadFile);
   offset         = loadInt(fp, loadFile);
   mode           = (Mode)(loadInt(fp, loadFile));
   
   deepeningAmount = period; /* Currently redundant, since it's recalculated */
   aborting        = 0;
   nRowsInState    = period+period;   /* how many rows needed to compute successor graph? */
   
   params[P_DEPTHLIMIT] = DEFAULT_DEPTHLIMIT;
   
    /* Allocate space for the data structures */
   base = (node*)malloc((QSIZE>>BASEBITS)*sizeof(node));
   rows = (row*)malloc(QSIZE*sizeof(row));
   if (base == 0 || rows == 0) {
      printf("Unable to allocate BFS queue!\n");
      exit(0);
   }
   
   if (hashBits == 0) hash = 0;
   else {
      hash = (node*)malloc(HASHSIZE*sizeof(node));
      if (hash == 0) printf("Unable to allocate hash table, duplicate elimination disabled\n");
   }
   
   /* Load up BFS queue and complete compaction */
   qHead  = loadUInt(fp, loadFile);
   qEnd   = loadUInt(fp, loadFile);
   qStart = QSIZE - qEnd;
   qEnd   = QSIZE;
   qHead += qStart;
   if (qStart > QSIZE || qStart < QSIZE/16) {
      printf("BFS queue is too small for saved state\n");
      exit(0);
   }
   for (i = qStart; i < qEnd; ++i)
      rows[i] = (row) loadUInt(fp, loadFile);
   
   /*
   printf("qHead:  %d qStart: %d qEnd: %d\n",qHead,qStart,qEnd);
   printf("rows[0]: %d\n",rows[qStart]);
   printf("rows[1]: %d\n",rows[qStart+1]);
   printf("rows[2]: %d\n",rows[qStart+2]);
   fflush(stdout);
   exit(0);
   */
   
   deepRows = (row**)calloc(1 << (params[P_DEPTHLIMIT] + 1),sizeof(*deepRows));
   deepRowIndices = (uint32_t*)calloc(QSIZE,sizeof(deepRowIndices));
   
   uint32_t theDeepIndex = 2;
   deepQTail = 0;
   
   while (fscanf(fp,"%u\n",&j) != EOF){
      if (j == 0){
         fscanf(fp,"%u\n",&j);
         for (i = 0; i < j; ++i){
            deepRowIndices[deepQTail] = 1;
            ++deepQTail;
         }
         continue;
      }
      deepRows[theDeepIndex] = (row*)calloc( j + 1 + 2, sizeof(**deepRows));
      deepRows[theDeepIndex][0] = j;
      for (i = 1; i < j + 1 + 2; ++i){
         deepRows[theDeepIndex][i] = loadUInt(fp, loadFile);
      }
      deepRowIndices[deepQTail] = theDeepIndex;
      ++theDeepIndex;
      ++deepQTail;
   }
   
   fclose(fp);
   
   doCompactPart2();
   
   /* Let the user know that we got this far (suppress if splitting) */
   if(!splitNum) printf("State successfully loaded from file %s\n",loadFile);
   
   fflush(stdout);
}

/* ================================================= */
/*  Load initial rows for extending partial results  */
/* ================================================= */

void printRow(row theRow){
   int i;
   for(i = width - 1; i >= 0; --i) printf("%c",(theRow & 1 << i ? 'o' : '.'));
   printf("\n");
}

void loadInitRows(char * loadFile, char * file){
   FILE * fp;
   int i,j;
   char rowStr[MAXWIDTH];
   row theRow = 0;
   
   loadFile = file;
   fp = fopen(loadFile, "r");
   if (!fp) loadFail(loadFile);
   
   printf("Starting search from rows in %s:\n",loadFile);
   
   for(i = 0; i < 2 * period; i++){
      fscanf(fp,"%s",rowStr);
      for(j = 0; j < width; j++){
         theRow |= ((rowStr[width - j - 1] == '.') ? 0:1) << j;
      }
      printRow(theRow);
      enqueue(dequeue(),theRow);
      theRow = 0;
   }
   fclose(fp);
}



void searchSetup(char * loadFile){
   if (params[P_CACHEMEM] < 0){
      if (5 * params[P_OFFSET] > params[P_PERIOD]) params[P_CACHEMEM] *= -1;
      else params[P_CACHEMEM] = 0;
   }
   
   checkParams(rule, nttable, params, previewFlag, initRowsFlag, loadDumpFlag);  /* Exit if parameters are invalid */
   
   if(loadDumpFlag) loadState(loadFile);
   else {
      width = params[P_WIDTH];
      period = params[P_PERIOD];
      offset = params[P_OFFSET];
      deepeningAmount = period;
      hashPhase = (gcd(period,offset)>1);
      
      nRowsInState = period+period;
      
      params[P_DEPTHLIMIT] = DEFAULT_DEPTHLIMIT;
      
      base = (node*)malloc((QSIZE>>BASEBITS)*sizeof(node));
      rows = (row*)malloc(QSIZE*sizeof(row));
      if (base == 0 || rows == 0) {
         printf("Unable to allocate BFS queue!\n");
         exit(0);
      }
      
      if (hashBits == 0) hash = 0;
      else {
         hash = (node*)malloc(HASHSIZE*sizeof(node));
         if (hash == 0) printf("Unable to allocate hash table, duplicate elimination disabled\n");
      }
      
      deepRows = (row**)calloc(1 << (params[P_DEPTHLIMIT] + 1),sizeof(*deepRows));
      deepRowIndices = (uint32_t*)calloc(QSIZE,sizeof(deepRowIndices));
      
      resetQ();
      resetHash();
      
      enqueue(0,0);
      
      if(initRowsFlag) loadInitRows(loadFile, initRows);
   }
   
   if(previewFlag){
      preview(1);
      exit(0);
   }
   
   if(params[P_MINEXTENSION] < 0) params[P_MINEXTENSION] = 0;
   
   /* correction of params[P_LASTDEEP] after modification */
   if(newLastDeep){
      params[P_LASTDEEP] -= MINDEEP;
      if(params[P_LASTDEEP] < 0) params[P_LASTDEEP] = 0;
   }
   
   /* split queue across multiple files */
   if(splitNum > 0){
      node x;
      uint32_t i,j;
      uint32_t deepIndex;
      int firstDumpNum = 0;
      int totalNodes = 0;
      
      echoParams(params, rule, period);
      printf("\n");
      
      if(!loadDumpFlag || qHead == 0 || splitNum == 1){
         dumpFlag = pending;
         if(qHead == 0){      /* can't use doCompact() here, because it tries to access rows[-1] */
            qStart = qHead;
            qEnd = qTail;
            dumpState();
         }
         else doCompact();
         if(dumpFlag == success){
            printf("State dumped to %s\n",dumpFile);
            exit(0);
         }
         else{
            fprintf(stderr, "Error: dump failed.\n");
            exit(1);
         }
      }
      
      if(splitNum >= 100000){
         fprintf(stderr, "Warning: queue cannot be split into more than 99999 files.\n");
         splitNum = 99999;
      }
      
      /* count nodes in queue */
      for(x = qHead; x < qTail; x++){
         if(!EMPTY(x)) totalNodes++;
      }
      
      /* nodes per file is rounded up */
      int nodesPerFile = (totalNodes - 1) / splitNum + 1;
      
      printf("Splitting search state with %d queue nodes per file\n",nodesPerFile);
      
      /* save qHead and qTail, as creating the pieces will change their values */
      node fixedQHead = qHead;
      node fixedQTail = qTail;
      
      /* delete the queue; we will reload it as needed */
      free(base);
      free(rows);
      free(hash);
      
      for (deepIndex = 0; deepIndex < 1 << (params[P_DEPTHLIMIT] + 1); ++deepIndex){
         if (deepRows[deepIndex]) free(deepRows[deepIndex]);
         deepRows[deepIndex] = 0;
      }
      free(deepRows);
      free(deepRowIndices);
      
      node currNode = fixedQHead;
      
      while(currNode < fixedQTail){
         
         /* load the queue */
         loadState(loadFile);
         
         /* empty everything before currNode */
         j = deepQHead;
         for(x = fixedQHead; x < currNode; ++x){
            MAKEEMPTY(x);
            deepRowIndices[j] = 0;
            ++j;
         }
         
         /* skip the specified number of nonempty nodes */
         i = 0;
         while(i < nodesPerFile && x < fixedQTail){
            if(!EMPTY(x))
               ++i;
            ++x;
            ++j;
         }
         
         /* update currNode */
         currNode = x;
         
         /* empty everything after specified nonempty nodes */
         while(x < fixedQTail){
            MAKEEMPTY(x);
            deepRowIndices[j] = 0;
            ++x;
            ++j;
         }
         
         /* save the piece */
         dumpFlag = pending;
         doCompact();
         
         if(!firstDumpNum) firstDumpNum = dumpNum - 1;
         
         if (dumpFlag != success){
            printf("Failed to save %s\n",dumpFile);
            exit(1);
         }
         
         if(dumpNum >= 100000){
            fprintf(stderr, "Error: dump file number limit reached.\n");
            fprintf(stderr, "       Try splitting the queue in a new directory.\n");
            exit(1);
         }
         
         /* free memory allocated in loadState() */
         for (deepIndex = 0; deepIndex < 1 << (params[P_DEPTHLIMIT] + 1); ++deepIndex){
            if (deepRows[deepIndex]) free(deepRows[deepIndex]);
            deepRows[deepIndex] = 0;
         }
         
         free(base);
         free(rows);
         free(hash);
         free(deepRows);
         free(deepRowIndices);
         
      }
      
      printf("Saved pieces in files %s%05d to %s\n",dumpRoot,firstDumpNum,dumpFile);
      exit(0);
   }
   
   omp_set_num_threads(params[P_NUMTHREADS]);
   
   memlimit = ((long long)params[P_MEMLIMIT]) << 20;
   
   /* Allocate lookahead cache */
#ifndef NOCACHE
   cachesize = 32768 ;
   while (cachesize * sizeof(cacheentry) < 550000 * params[P_CACHEMEM])
      cachesize <<= 1 ;
   memusage += sizeof(cacheentry) * (cachesize + 5) * params[P_NUMTHREADS];
   if(params[P_MEMLIMIT] >= 0 && memusage > memlimit){
      printf("Not enough memory to allocate lookahead cache\n");
      exit(0);
   }
   totalCache = (cacheentry *)calloc(sizeof(cacheentry),
         (cachesize + 5) * params[P_NUMTHREADS]) ;
   cache = (cacheentry **)calloc(sizeof(**cache), params[P_NUMTHREADS]);
   
   for(int i = 0; i < params[P_NUMTHREADS]; i++)
      cache[i] = totalCache + (cachesize + 5) * i;
#endif
   
   echoParams(params, rule, period);
   
#ifndef QSIMPLE
   makePhases(period, offset, fwdOff, backOff, doubleOff, tripleOff);
#endif
   fasterTable(nttable, nttable2);
   makeTables();
   
   rephase();
}
