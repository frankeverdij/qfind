#ifndef DEFINES_H
#define DEFINES_H

#include "const.h"

inline int mindeep(const int * params, const int period)
{
    if (params[P_MINDEEP]>0)
        return params[P_MINDEEP];
    else
        return period;
}

#endif /* DEFINES_H */
