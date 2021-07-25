#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "const.h"
#include "enum.h"
#include "load.h"

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


/* ========================== */
/*  Print usage instructions  */
/* ========================== */

/* Note: currently reserving -v for potentially editing an array of extra variables */
void usage(){
#ifndef QSIMPLE
   printf("Usage: \"qfind options\"\n");
   printf("  e.g. \"qfind -r B3/S23 -p 3 -y 1 -w 6 -s even\" searches Life (rule B3/S23)\n");
   printf("  for c/3 orthogonal spaceships with even bilateral symmetry and a\n");
   printf("  logical width of 6 (full width 12).\n");
#else
   printf("Three required parameters, the period, offset, and width, must be\n");
   printf("set within the code before it is compiled. You have compiled with\n");
   printf("\n");
   printf("Period: %d\n",PERIOD);
   printf("Offset: %d\n",OFFSET);
   printf("Width:  %d\n",WIDTH);
#endif
   printf("\n");
   printf("Available options:\n");
   printf("  -r bNN/sNN  searches for spaceships in the specified rule (default: b3/s23)\n");
   printf("              Non-totalistic rules can be entered using Hensel notation.\n");
   printf("\n");
#ifndef QSIMPLE
   printf("  -p NN  searches for spaceships with period NN\n");
   printf("  -y NN  searches for spaceships that travel NN cells every period\n");
   printf("  -w NN  searches for spaceships with logical width NN\n");
   printf("         (full width depends on symmetry type)\n");
#endif
   printf("  -s FF  searches for spaceships with symmetry type FF\n");
   printf("         Valid symmetry types are asymmetric, odd, even, and gutter.\n");
   printf("\n");
#ifdef _OPENMP
   printf("  -t NN  runs search using NN threads during deepening step (default: 1)\n");
#endif
   printf("  -i NN  sets minimum deepening increment to NN (default: period)\n");
   printf("  -n NN  deepens to total depth at least NN during first deepening step\n");
   printf("         (total depth includes depth of BFS queue)\n");
   printf("  -g NN  stores depth-first extensions of length at least NN (default: 0)\n");
   printf("  -f NN  stops search if NN spaceships are found (default: no limit)\n");
   printf("  -q NN  sets the BFS queue size to 2^NN (default: %d)\n",QBITS);
   printf("  -h NN  sets the hash table size to 2^NN (default: %d)\n",HASHBITS);
   printf("         Use -h 0 to disable duplicate elimination.\n");

   printf("  -b NN  groups 2^NN queue entries to an index node (default: 4)\n");
   printf("  -m NN  limits memory usage to NN megabytes (default: no limit)\n");
#ifndef NOCACHE
   printf("  -c NN  allocates NN megabytes per thread for lookahead cache\n");
   printf("         (default: %d if speed is greater than c/5 and disabled otherwise)\n",DEFAULT_CACHEMEM);
   printf("         Use -c 0 to disable lookahead caching.\n");
#endif
   printf("  -z     toggles whether to output spaceships found during deepening step\n");
   printf("         (default: output enabled for spaceships found during deepening step)\n");
   printf("         Disabling is useful for searches that find many spaceships.\n");
   printf("  -a     toggles whether to output longest partial result at end of search\n");
   printf("         (default: output enabled for longest partial result)\n");
   printf("\n");
   printf("  -e FF  uses rows in the file FF as the initial rows for the search\n");
   printf("         (use the companion Golly Lua script to easily generate the\n");
   printf("         initial row file)\n");
   printf("  -d FF  dumps the search state after each queue compaction using\n");
   printf("         file name prefix FF\n");
   printf("  -l FF  loads the search state from the file FF\n");
   printf("  -j NN  splits the search state into at most NN files\n");
   printf("         (uses the file name prefix defined by the -d option)\n");
   printf("  -u     previews partial results from the loaded state\n");
   printf("\n");
   printf("  --help  prints usage instructions and exits\n");
}

/* ================================= */
/*  Parse options and set up search  */
/* ================================= */

void setDefaultParams(int * params){
#ifdef QSIMPLE
   params[P_WIDTH] = WIDTH;
   params[P_PERIOD] = PERIOD;
   params[P_OFFSET] = OFFSET;
#else
   params[P_WIDTH] = 0;
   params[P_PERIOD] = 0;
   params[P_OFFSET] = 0;
#endif
   params[P_SYMMETRY] = 0;
   params[P_REORDER] = 1;
   params[P_CHECKPOINT] = 0;
   params[P_BASEBITS] = 4;
   params[P_QBITS] = QBITS;
   params[P_HASHBITS] = HASHBITS;
   params[P_NUMTHREADS] = 1;
   params[P_MINDEEP] = 0;
   params[P_CACHEMEM] = -1*DEFAULT_CACHEMEM;
   params[P_MEMLIMIT] = -1;
   params[P_PRINTDEEP] = 1;
   params[P_LONGEST] = 1;
   params[P_LASTDEEP] = 0;
   params[P_NUMSHIPS] = 0;
   params[P_MINEXTENSION] = 0;
}

/* Note: currently reserving -v for potentially editing an array of extra variables */
void parseOptions(int argc, char *argv[], char *rule, Mode *mode, int *params, int *newLastDeep, int *previewFlag, char *dumpRoot, int *splitNum, char *initRows, int *initRowsFlag, char *loadFile, int *loadDumpFlag)
{
   while(--argc > 0){               /* read input parameters */
      if ((*++argv)[0] == '-'){
         switch ((*argv)[1]){
            case 'r': case 'R':
               --argc;
               ++argv;
               strncpy(rule, *argv, 255);
               break;
#ifndef QSIMPLE
            case 'p': case 'P':
               --argc;
               sscanf(*++argv, "%d", &params[P_PERIOD]);
               break;
            case 'y': case 'Y':
               --argc;
               sscanf(*++argv, "%d", &params[P_OFFSET]);
              break;
            case 'w': case 'W':
               --argc;
               sscanf(*++argv, "%d", &params[P_WIDTH]);
               break;
#endif
            case 's': case 'S':
               --argc;
               switch((*++argv)[0]) {
                  case 'a': case 'A':
                     params[P_SYMMETRY] = SYM_ASYM; *mode = asymmetric; break;
                  case 'o': case 'O':
                     params[P_SYMMETRY] = SYM_ODD; *mode = odd; break;
                  case 'e': case 'E':
                     params[P_SYMMETRY] = SYM_EVEN; *mode = even; break;
                  case 'g': case 'G':
                     params[P_SYMMETRY] = SYM_GUTTER; *mode = gutter; break;
                  default:
                     fprintf(stderr, "Error: unrecognized symmetry type %s\n", *argv);
                     fprintf(stderr, "\nUse --help for a list of available options.\n");
                     exit(1);
                     break;
               }
               break;
            case 'm': case 'M':
               --argc;
               sscanf(*++argv, "%d", &params[P_MEMLIMIT]);
               break;
            case 'n': case 'N':
               --argc;
               sscanf(*++argv, "%d", &params[P_LASTDEEP]);
               *newLastDeep = 1;
               break;
            case 'c': case 'C':
               --argc;
               sscanf(*++argv, "%d", &params[P_CACHEMEM]);
               break;
            case 'i': case 'I':
               --argc;
               sscanf(*++argv, "%d", &params[P_MINDEEP]);
               break;
            case 'q': case 'Q':
               --argc;
               sscanf(*++argv, "%d", &params[P_QBITS]);
               break;
            case 'h': case 'H':
               --argc;
               sscanf(*++argv, "%d", &params[P_HASHBITS]);
               break;
            case 'b': case 'B':
               --argc;
               sscanf(*++argv, "%d", &params[P_BASEBITS]);
               break;
#ifdef _OPENMP
            case 't': case 'T':
               --argc;
               sscanf(*++argv, "%d", &params[P_NUMTHREADS]);
               break;
#endif
            case 'f': case 'F':
               --argc;
               sscanf(*++argv, "%d", &params[P_NUMSHIPS]);
               break;
            case 'g': case 'G':
               --argc;
               sscanf(*++argv, "%d", &params[P_MINEXTENSION]);
               break;
            case 'z': case 'Z':
               params[P_PRINTDEEP] ^= 1;
               break;
            case 'a': case 'A':
               params[P_LONGEST] ^= 1;
               break;
            case 'u': case 'U':
               *previewFlag = 1;
               break;
            case 'd': case 'D':
               --argc;
               ++argv;
               strncpy(dumpRoot, *argv, 255);
               params[P_CHECKPOINT] = 1;
               break;
            case 'j': case 'J':
               --argc;
               sscanf(*++argv, "%d", splitNum);
               if(*splitNum < 0) *splitNum = 0;
               break;
            case 'e': case 'E':
               --argc;
               initRows = *++argv;
               *initRowsFlag = 1;
               break;
            case 'l': case 'L':
               --argc;
               loadFile = *++argv;
               *loadDumpFlag = 1;
               loadParams(params, loadFile, newLastDeep, rule, dumpRoot);
               break;
            case '-':
               if(!strcmp(*argv,"--help") || !strcmp(*argv,"--Help")){
                  usage();
                  exit(0);
               }
               else{
                  fprintf(stderr, "Error: unrecognized option %s\n", *argv);
                  fprintf(stderr, "\nUse --help for a list of available options.\n");
                  exit(1);
               }
               break;
           default:
              fprintf(stderr, "Error: unrecognized option %s\n", *argv);
              fprintf(stderr, "\nUse --help for a list of available options.\n");
              exit(1);
              break;
         }
      }
      else{
         fprintf(stderr, "Error: unrecognized option %s\n", *argv);
         fprintf(stderr, "\nUse --help for a list of available options.\n");
         exit(1);
      }
   }
}

