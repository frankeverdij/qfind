#include <stdio.h>

#include "const.h"

void finalReport(const int numFound, const int longest, const int *params,
                 const int aborting, const char *patternBuf)
{
    printf("Search complete.\n\n");
   
    printf("%d spaceship%s found.\n", numFound,(numFound == 1) ? "" : "s");
    printf("Maximum depth reached: %d\n", longest);

    /* aborting == 3 means we reached ship limit */
    if(params[P_LONGEST] && aborting != 3)
    {
        if(patternBuf)
            printf("Longest partial result:\n\n%s", patternBuf);
        else
            printf("No partial results found.\n");
   }
}
