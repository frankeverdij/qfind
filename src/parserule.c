#include <stdio.h>
#include <stdlib.h>
#include "const.h"

/* =========================================== */
/*  Lookup tables to determine successor rows  */
/* =========================================== */

const char *rulekeys[] = {
   "0", "1c", "1e", "2a", "1c", "2c", "2a", "3i",
   "1e", "2k", "2e", "3j", "2a", "3n", "3a", "4a",
   "1c", "2n", "2k", "3q", "2c", "3c", "3n", "4n",
   "2a", "3q", "3j", "4w", "3i", "4n", "4a", "5a",
   "1e", "2k", "2i", "3r", "2k", "3y", "3r", "4t",
   "2e", "3k", "3e", "4j", "3j", "4k", "4r", "5n",
   "2a", "3q", "3r", "4z", "3n", "4y", "4i", "5r",
   "3a", "4q", "4r", "5q", "4a", "5j", "5i", "6a",
   "1c", "2c", "2k", "3n", "2n", "3c", "3q", "4n",
   "2k", "3y", "3k", "4k", "3q", "4y", "4q", "5j",
   "2c", "3c", "3y", "4y", "3c", "4c", "4y", "5e",
   "3n", "4y", "4k", "5k", "4n", "5e", "5j", "6e",
   "2a", "3n", "3r", "4i", "3q", "4y", "4z", "5r",
   "3j", "4k", "4j", "5y", "4w", "5k", "5q", "6k",
   "3i", "4n", "4t", "5r", "4n", "5e", "5r", "6i",
   "4a", "5j", "5n", "6k", "5a", "6e", "6a", "7e",
   "1e", "2a", "2e", "3a", "2k", "3n", "3j", "4a",
   "2i", "3r", "3e", "4r", "3r", "4i", "4r", "5i",
   "2k", "3q", "3k", "4q", "3y", "4y", "4k", "5j",
   "3r", "4z", "4j", "5q", "4t", "5r", "5n", "6a",
   "2e", "3j", "3e", "4r", "3k", "4k", "4j", "5n",
   "3e", "4j", "4e", "5c", "4j", "5y", "5c", "6c",
   "3j", "4w", "4j", "5q", "4k", "5k", "5y", "6k",
   "4r", "5q", "5c", "6n", "5n", "6k", "6c", "7c",
   "2a", "3i", "3j", "4a", "3q", "4n", "4w", "5a",
   "3r", "4t", "4j", "5n", "4z", "5r", "5q", "6a",
   "3n", "4n", "4k", "5j", "4y", "5e", "5k", "6e",
   "4i", "5r", "5y", "6k", "5r", "6i", "6k", "7e",
   "3a", "4a", "4r", "5i", "4q", "5j", "5q", "6a",
   "4r", "5n", "5c", "6c", "5q", "6k", "6n", "7c",
   "4a", "5a", "5n", "6a", "5j", "6e", "6k", "7e",
   "5i", "6a", "6c", "7c", "6a", "7e", "7c", "8" } ;
   
/*
 *   Parses the rule.  If there's an error, return a string describing the
 *   error.  Fills in the 512-element array pointed to by tab.
 */
const char *parseRule(const char *rule, int *tab) {
   const char *p = rule ;
   for (int i=0; i<512; i++)
      tab[i] = 0 ;
   for (int bs=0; bs<512; bs += 256) {
      if (bs == 0) {
         if (*p != 'B' && *p != 'b')
            return "Expected B at start of rule" ;
      } else {
         if (*p != 'S' && *p != 's')
            return "Expected S after slash" ;
      }
      p++ ;
      while (*p != '/' && *p != 0) {
         if (!('0' <= *p || *p <= '9'))
            return "Missing number in rule" ;
         char dig = *p++ ;
         int neg = 0 ;
         if (*p == '/' || *p == 0 || *p == '-' || ('0' <= *p && *p <= '8'))
            for (int i=0; i<256; i++)
               if (rulekeys[i][0] == dig)
                  tab[bs+i] = 1 ;
         for (; *p != '/' && *p != 0 && !('0' <= *p && *p <= '8'); p++) {
            if (*p == '-') {
               if (neg)
                  return "Can't have multiple negation signs" ;
               neg = 1 ;
            } else if ('a' <= *p && *p <= 'z') {
               int used = 0 ;
               for (int i=0; i<256; i++)
                  if (rulekeys[i][0] == dig && rulekeys[i][1] == *p) {
                     tab[bs+i] = 1-neg ;
                     used++ ;
                  }
               if (!used)
                  return "Unexpected character in rule" ;
            } else
               return "Unexpected character in rule" ;
         }
      }
      if (bs == 0) {
         if (*p++ != '/')
            return "Missing expected slash between b and s" ;
      } else {
         if (*p++ != 0)
            return "Extra unparsed junk at end of rule string" ;
      }
   }
   return 0 ;
}

void checkParams(const char *rule, int *tab, const int *params, const int pf, const int irf, const int ldf){
   int exitFlag = 0;
   const char *ruleError;
   
   /* Errors */
   ruleError = parseRule(rule, tab);
   if (ruleError != 0){
      fprintf(stderr, "Error: failed to parse rule %s\n", rule);
      fprintf(stderr, "       %s\n", ruleError);
      exitFlag = 1;
   }
#ifdef QSIMPLE
   if(gcd(PERIOD,OFFSET) > 1){
      fprintf(stderr, "Error: qfind-s does not support gcd(PERIOD,OFFSET) > 1. Use qfind instead.\n");
      exitFlag = 1;
   }
#else
   if(params[P_WIDTH] < 1 || params[P_PERIOD] < 1 || params[P_OFFSET] < 1){
      fprintf(stderr, "Error: period (-p), translation (-y), and width (-w) must be positive integers.\n");
      exitFlag = 1;
   }
   if(params[P_PERIOD] > MAXPERIOD){
      fprintf(stderr, "Error: maximum allowed period (%d) exceeded.\n", MAXPERIOD);
      exitFlag = 1;
   }
   if(params[P_OFFSET] > params[P_PERIOD] && params[P_PERIOD] > 0){
      fprintf(stderr, "Error: translation (-y) cannot exceed period (-p).\n");
      exitFlag = 1;
   }
   if(params[P_OFFSET] == params[P_PERIOD] && params[P_PERIOD] > 0){
      fprintf(stderr, "Error: photons are not supported.\n");
      exitFlag = 1;
   }
#endif
   if(params[P_SYMMETRY] == 0){
      fprintf(stderr, "Error: you must specify a symmetry type (-s).\n");
      exitFlag = 1;
   }
   if(pf && !ldf){
      fprintf(stderr, "Error: the search state must be loaded from a file to preview partial results.\n");
      exitFlag = 1;
   }
   if(irf && ldf){
      fprintf(stderr, "Error: Initial rows file cannot be used when the search state is loaded from a\n       saved state.\n");
      exitFlag = 1;
   }
   
   /* Warnings */
   if(2 * params[P_OFFSET] > params[P_PERIOD] && params[P_PERIOD] > 0){
      fprintf(stderr, "Warning: searches for speeds exceeding c/2 may not work correctly.\n");
   }
#ifdef NOCACHE
   if(5 * params[P_OFFSET] > params[P_PERIOD] && params[P_PERIOD] > 0 && params[P_CACHEMEM] == 0){
      fprintf(stderr, "Warning: Searches for speeds exceeding c/5 may be slower without caching.\n         It is recommended that you increase the cache memory (-c).\n");
   }
#else
   if(5 * params[P_OFFSET] <= params[P_PERIOD] && params[P_OFFSET] > 0 && params[P_CACHEMEM] > 0){
      fprintf(stderr, "Warning: Searches for speeds at or below c/5 may be slower with caching.\n         It is recommended that you disable caching (-c 0).\n");
   }
#endif
   
   /* exit if there are errors */
   if(exitFlag){
      fprintf(stderr, "\nUse --help for a list of available options.\n");
      exit(1);
   }
   fprintf(stderr, "\n");
}

