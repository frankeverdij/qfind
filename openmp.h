#ifndef OPENMP_H
#define OPENMP_H

#ifdef _OPENMP
#include <omp.h>
#else
#define omp_get_thread_num() 0
#define omp_set_num_threads(x)
#endif

#endif /* OPENMP_H */
