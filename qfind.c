/* qfind --- A spaceship search program by Matthias Merzenich.
** 
** This is an attempt at combining the functionality of gfind and zfind.
**
** Based on code by David Eppstein, zdr, Paul Tooke, and Tomas Rokicki.
** Thanks also to Frank Everdij, praosylen, and Adam P. Goucher for code
** and suggestions.
*/

#include "common.h"

int fwdOff[MAXPERIOD], backOff[MAXPERIOD], doubleOff[MAXPERIOD], tripleOff[MAXPERIOD];

void makePhases(){
   int i;
   for (i = 0; i < period; i++) backOff[i] = -1;
   i = 0;
   for (;;) {
      int j = offset;
      while (backOff[(i+j)%period] >= 0 && j < period) j++;
      if (j == period) {
         backOff[i] = period-i;
         break;
      }
      backOff[i] = j;
      i = (i+j)%period;
   }
   for (i = 0; i < period; i++)
      fwdOff[(i+backOff[i])%period] = backOff[i];
   for (i = 0; i < period; i++) {
      int j = (i - fwdOff[i]);
      if (j < 0) j += period;
      doubleOff[i] = fwdOff[i] + fwdOff[j];
   }
   for (i = 0; i <  period; i++){
      int j = (i - fwdOff[i]);
      if (j < 0) j += period;
      tripleOff[i] = fwdOff[i] + doubleOff[j];
   }
}

int lookAhead(row *pRows, int a, int pPhase){
   /* indices: first digit represents vertical offset,      */
   /*          second digit represents generational offset  */
   int ri11, ri12, ri13, ri22, ri23;
   uint16_t *riStart11, *riStart12, *riStart13, *riStart22, *riStart23;
   int numRows11, numRows12, numRows13, numRows22, numRows23;
   int row11, row12, row13, row22, row23;
   int k;
   
   numRows11 = getoffsetcount(pRows[a - params[P_PERIOD] - fwdOff[pPhase]],
                  pRows[a - fwdOff[pPhase]],
                  pRows[a], &riStart11);
   if (!numRows11)
      return 0;
   
   numRows12 = getoffsetcount(pRows[a - params[P_PERIOD] - doubleOff[pPhase]],
                  pRows[a - doubleOff[pPhase]],
                  pRows[a - fwdOff[pPhase]], &riStart12);
   
   if(tripleOff[pPhase] >= params[P_PERIOD]){
      riStart13 = pRows + (a + params[P_PERIOD] - tripleOff[pPhase]);
      numRows13 = 1;
#ifndef NOCACHE
      k = getkey(riStart11, riStart12, (uint16_t*)(gcount + riStart13[0]),
         (pRows[a-doubleOff[pPhase]] << width) + pRows[a-tripleOff[pPhase]]);
#endif
   }
   else{
      numRows13 = getoffsetcount(pRows[a - params[P_PERIOD] - tripleOff[pPhase]],
                     pRows[a - tripleOff[pPhase]],
                     pRows[a - doubleOff[pPhase]], &riStart13);
#ifndef NOCACHE
      k = getkey(riStart11, riStart12, riStart13,
         (pRows[a-doubleOff[pPhase]] << width) + pRows[a-tripleOff[pPhase]]);
#endif
   }
#ifndef NOCACHE
   if (k < 0)
      return k+2;
#endif
   
   for(ri11 = 0; ri11 < numRows11; ++ri11){
      row11 = riStart11[ri11];
      for(ri12 = 0; ri12 < numRows12; ++ri12){
         row12 = riStart12[ri12];
         numRows22 = getoffsetcount(pRows[a - doubleOff[pPhase]],
                        row12, row11, &riStart22);
         if(!numRows22) continue;
         
         for(ri13 = 0; ri13 < numRows13; ++ri13){
            row13 = riStart13[ri13];
            numRows23 = getoffsetcount(pRows[a - tripleOff[pPhase]],
                           row13, row12, &riStart23);
            if(!numRows23) continue;
            
            for(ri23 = 0; ri23 < numRows23; ++ri23){
               row23 = riStart23[ri23];
               uint16_t *p = getoffset(row13, row23);
               for(ri22 = 0; ri22 < numRows22; ++ri22){
                  row22 = riStart22[ri22];
                  if (p[row22+1]!=p[row22]) {
#ifndef NOCACHE
                     setkey(k, 1);
#endif
                     return 1;
                  }
               }
            }
         }
      }
   }
#ifndef NOCACHE
   setkey(k, 0);
#endif
   return 0;
}

/* Testing for subperiodic patterns
**
** For each possible phase of the ship, equivRow[0][phase] gives the row that 
** is equivalent if the pattern is subperiodic with a specified period.
** equivRow[1] is necessary if gcd(period,offset) has two distinct prime 
** divisors, as two subperiods need to be tested (e.g., if speed == 6c/12, we
** must test subperiods 4 and 6).
*/

int equivRow[2][MAXPERIOD];

int smallestDivisor(int b){
   int c = 2;
   while(b % c) ++c;
   return c;
}

void makeEqRows(int maxFactor, int divNum){
   int tempEquivRow[MAXPERIOD];
   int i,j;
   for(i = 0; i < period; ++i){
      tempEquivRow[i] = i;
      for(j = 0; j < maxFactor; ++j){
         tempEquivRow[i] += backOff[tempEquivRow[i] % period];
      }
      tempEquivRow[i] -= offset * maxFactor + i;
      equivRow[divNum][i] = tempEquivRow[i];
   }
   for(i = 0; i < period; ++i){     /* make equivRow[i] negative if possible */
      if(tempEquivRow[i] > 0){
         equivRow[divNum][i + tempEquivRow[i]] = -1 * tempEquivRow[i];
      }
   }
}

/* make phase tables for determining equivalent subperiodic rows */
void makeSubperiodTables(){
   if(gcd(period,offset) > 1){
      int div1 = smallestDivisor(gcd(period,offset));
      makeEqRows(period / div1,0);
      int div2 = gcd(period,offset);
      while(div2 % div1 == 0) div2 /= div1;
      if(div2 != 1)
         makeEqRows(period / smallestDivisor(div2),1);
      else                                /* If gcd(period,offset) has only one prime divisor, just  */
         makeEqRows(period / div1,1);     /* reuse it.  We don't run the subperiod check very often. */
   }
}

int subperiodTest(node x, int divNum, row *pRows, int nodeRow, uint32_t lastRow){
   int i,a;
   node y,z;
   
   int pPhase;
   
   pPhase = (peekPhase(x) + lastRow - nodeRow) % period + period;
   a = lastRow;   /* lastRow == 0 when calling from queue */
   while (equivRow[divNum][pPhase % period] >= 0){
      pPhase--;
      a--;
   }
   
   if (pRows != NULL){
      int b = a + equivRow[divNum][pPhase % period];
      
      while (b > nodeRow){
         if (pRows[a] != pRows[b])
            return 0;
         a -= period;
         b -= period;
      }
      z = x;
      for (i=0; i < (nodeRow - b); i++) z = PARENT(z);
      
      if (a > nodeRow){
         if (pRows[a] != ROW(z))
            return 0;
         a -= period;
         for (i=0; i < period; i++)
            z = PARENT(z);
      }
      y = x;
      for (i=0; i < (nodeRow - a); i++)
         y = PARENT(y);
   }
   else {
      y = x;
      for(i=0; i < -1 * a; i++)
         y = PARENT(y);
      z = y;
      for(i=0; i < -1 * equivRow[divNum][pPhase % period]; i++)
         z = PARENT(z);
   }
   
   while (z != 0){
      if (ROW(y) != ROW(z))
         return 0;
      
      for (i=0; i < period; i++){
         y = PARENT(y);
         z = PARENT(z);
      }
   }
   return 1;
}

int subperiodic(node x, row *pRows, int nodeRow, uint32_t lastRow){
   if (!params[P_FULLPERIOD] || gcd(period,offset) == 1)
      return 0;
   
   if (subperiodTest(x,0,pRows,nodeRow,lastRow) || subperiodTest(x,1,pRows,nodeRow,lastRow))
      return 1;
   
   return 0;
}


/* 
** process() dequeues the node at the head of the queue and enqueues any valid
** child nodes.  Spaceships are detected by a call to terminal().
*/

void process(node theNode)
{
   long long int i;
   int firstRow = 0;
   int numRows;
   int matchFlag = 1;
   node x = theNode;
   int pPhase = peekPhase(x);
   row *riStart;
   row pRows[2*MAXPERIOD + 2];
   int currRow = 2*period + 1;
   for(i = currRow - 1; i >= 0; --i){
      pRows[i] = ROW(x);
      x = PARENT(x);
   }
   
   ++pPhase;
   if(pPhase == period) pPhase = 0;
   
   numRows = getoffsetcount(pRows[currRow - 2 * period],
                     pRows[currRow - period],
                     pRows[currRow - period + backOff[pPhase]],
                     &riStart);
   
   /* we just ran dequeue() which changed DeepQHead so */
   /* we need to look at the previous head location    */
   uint32_t deepIndex = deepRowIndices[oldDeepQHead];
   
   if(theNode == 0){
      firstRow = 1;
   }
   else if(deepIndex > 1){  /* This means we found an extension previously */
      
      /* Sanity check: do the extension rows match the node rows? */
      node y = theNode;
      for(i = 0; i < 2*period; ++i){
         uint16_t startRow = deepRows[deepIndex][1] + 1;
         if(deepRows[deepIndex][startRow - i] != ROW(y)){
            fprintf(stderr, "Warning: non-matching rows detected at node %u in process()\n",theNode);
            matchFlag = 0;
            free(deepRows[deepIndex]);
            deepRows[deepIndex] = 0;
            break;
         }
         y = PARENT(y);
      }
      
      if(matchFlag){
         uint16_t deepStart = deepRows[deepIndex][1] + 2;
         ++deepRows[deepIndex][1];
         
         while(riStart[firstRow] != deepRows[deepIndex][deepStart]) ++firstRow;
         if (!isVisited(theNode, riStart[firstRow])){
            enqueue(theNode, riStart[firstRow]);
            deepRowIndices[deepQTail - 1] = deepIndex;
            if(currentDepth() > longest){
               if(params[P_LONGEST]) bufferPattern(qTail-1, NULL, 0, 0, 0);
               longest = currentDepth();
            }
            if (terminal(qTail-1) && !terminal(PARENT(qTail-1))) success(qTail-1, NULL, 0, 0);
            setVisited(qTail - 1);
            if(deepRows[deepIndex][1] > deepRows[deepIndex][0]){
               deepRowIndices[deepQTail - 1] = 0;
            }
         }
         
         /* eliminate extension if it gets too short */
         if(deepRows[deepIndex][1] > deepRows[deepIndex][0]){
            free(deepRows[deepIndex]);
            deepRows[deepIndex] = 0;
         }
         ++firstRow;
      }
   }
   
   /* This is no longer in the queue, so we can clear it */
   deepRowIndices[oldDeepQHead] = 0;
   
   for(i = firstRow; i < numRows; ++i){
      pRows[currRow] = riStart[i];
      if (!isVisited(theNode, pRows[currRow]) && lookAhead(pRows, currRow, pPhase)){
         enqueue(theNode, pRows[currRow]);
         if(currentDepth() > longest){
            if(params[P_LONGEST]) bufferPattern(qTail-1, NULL, 0, 0, 0);
            longest = currentDepth();
         }
         if (terminal(qTail-1) && !terminal(PARENT(qTail-1))) success(qTail-1, NULL, 0, 0);
         setVisited(qTail - 1);
      }
   }
}

int reloadDepthFirst(uint16_t startRow, int pPhase, uint16_t howDeep, row *pRows, uint16_t **pIndGen, int *pRemainGen, row *pRowsGen){
   uint16_t currRow = startRow;
   
   /* Return value if length of extension is greater than deepening amount */
   if(pRows[0] >= howDeep + pRows[1]) return 1;
   
   memcpy(pRowsGen + startRow, pRows + 2 + pRows[1], (pRows[0] - pRows[1] + 1) * (long long)sizeof(*pRowsGen));
   
   for(currRow = startRow; currRow <= startRow + (pRows[0] - pRows[1]); ++currRow){
      pRemainGen[currRow] = getoffsetcount(pRowsGen[currRow - 2 * period],
                  pRowsGen[currRow - period],
                  pRowsGen[currRow - period + backOff[pPhase]],
                  &(pIndGen[currRow]));
      
      pIndGen[currRow] += pRemainGen[currRow];
      
      while(*(pIndGen[currRow] - pRemainGen[currRow]) != pRowsGen[currRow]){
         --pRemainGen[currRow];
      }
      --pRemainGen[currRow];
      
      ++pPhase;
      if(pPhase == period) pPhase = 0;
   }
   return 0;
}

int depthFirst(node theNode, uint16_t howDeep, uint16_t **pInd, int *pRemain, row *pRows){
   int pPhase;
   pPhase = peekPhase(theNode);
   node x = theNode;
   uint32_t startRow = 2*period + 1;
   uint32_t currRow = startRow;
   uint32_t theDeepIndex = deepRowIndices[deepQHead + theNode - qHead];
   int matchFlag = 1;
   
   int i;
   for(i = currRow - 1; i >= 0; --i){
      pRows[i] = ROW(x);
      x = PARENT(x);
   }
   ++pPhase;
   if(pPhase == period) pPhase = 0;
   
   /* Reload state if we have a previous extension */
   if(theDeepIndex > 1){
      if(reloadDepthFirst( startRow,
                           pPhase,
                           howDeep,
                           deepRows[theDeepIndex],
                           pInd,
                           pRemain,
                           pRows ))
      {
         return 1;   /* return if howDeep is less than the length of the previous extension */
      }
      
      /* Sanity check: do the extension rows match the node rows? */
      node y = theNode;
      for(i = 0; i < 2*period; ++i){
         if(deepRows[theDeepIndex][deepRows[theDeepIndex][1] + 1 - i] != ROW(y)){
            fprintf(stderr, "Warning: non-matching rows detected at node %u in depthFirst()\n",theNode);
            matchFlag = 0;
            break;
         }
         y = PARENT(y);
      }
      
      if(matchFlag){
         currRow = startRow   + deepRows[theDeepIndex][0]
                              - deepRows[theDeepIndex][1]
                              + 1;
         pPhase = (pPhase + currRow - startRow) % period;
         
         #pragma omp critical(findDeepIndex)
         {
            free(deepRows[theDeepIndex]);
            deepRows[theDeepIndex] = 0;
         }
      }
   }
   
   deepRowIndices[deepQHead + theNode - qHead] = 0;
   
   pRemain[currRow] = getoffsetcount(pRows[currRow - 2 * period],
                  pRows[currRow - period],
                  pRows[currRow - period + backOff[pPhase]],
                  &(pInd[currRow]));
   pInd[currRow] += pRemain[currRow];
   
   for(;;){
      /* back up if there are no rows left to check at this depth */
      if(!pRemain[currRow]){
         --currRow;
         if(pPhase == 0) pPhase = period;
         --pPhase;
         if(currRow < startRow) return 0;
         
         continue;
      }
      pRows[currRow] = *(pInd[currRow] - pRemain[currRow]);
      --pRemain[currRow];
      if(!lookAhead(pRows, currRow, pPhase)) continue;

      ++currRow;
      ++pPhase;
      if(pPhase == period) pPhase = 0;
      /* Check if we reached the desired depth. If so,  
         check if the result is a complete spaceship */
      if(currRow > startRow + howDeep){
         /* Flag that an extension was found. This value will be changed by saveDepthFirst() */
         deepRowIndices[deepQHead + theNode - qHead] = 1;
         
         /* Save the extension if it is long enough */
         if(howDeep >= params[P_MINEXTENSION]){
            saveDepthFirst(theNode, startRow, howDeep, pRows);
         }
         
         /* Check if the extension represents a spaceship */
         if(params[P_PRINTDEEP] == 0) return 1;
         for(i = 1; i <= period; ++i){
            if(pRows[currRow - i]) return 1;
         }
         currRow -= period;
         for(i = 1; i <= period; ++i){
            if(causesBirth[pRows[currRow - i]]) return 1;
         }
         
         /* If we got here, then we found a spaceship! */
         #pragma omp critical(printWhileDeepening)
         {
            success(theNode, pRows, startRow - 1, currRow + period - 1);
         }
         return 1;
      }
      
      pRemain[currRow] = getoffsetcount(pRows[currRow - 2 * period],
                     pRows[currRow - period],
                     pRows[currRow - period + backOff[pPhase]],
                     &(pInd[currRow]));
      pInd[currRow] += pRemain[currRow];
   }
}

int main(int argc, char *argv[]){
   printf("%s\n",BANNER);
   printf("Input:");
   int i;
   for (i=0; i<argc; i++)
      printf(" %s", argv[i]);
   printf("\n\n");
   
   setDefaultParams();
   
   parseOptions(argc, argv);
   
   searchSetup();
   
   /* make phase tables for determining equivalent subperiodic rows */
   if(gcd(period,offset) > 1){
      int div1 = smallestDivisor(gcd(period,offset));
      makeEqRows(period / div1,0);
      int div2 = gcd(period,offset);
      while(div2 % div1 == 0) div2 /= div1;
      if(div2 != 1)
         makeEqRows(period / smallestDivisor(div2),1);
      else                                /* If gcd(period,offset) has only one prime divisor, just  */
         makeEqRows(period / div1,1);     /* reuse it.  We don't run the subperiod check very often. */
   }
   
   printf("Starting search\n");
   fflush(stdout);
   
   breadthFirst();
   
   finalReport();
   
   return 0;
}
