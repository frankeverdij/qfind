#ifndef ENUM_H
#define ENUM_H

typedef enum {
   asymmetric,          /* basic orthogonal pattern */
   odd, even,           /* orthogonal with bilateral symmetry */
   gutter,              /* orthogonal bilateral symmetry with empty column in middle */
} Mode;

typedef enum {
    unknown,
    pending,
    failure,
    success,
} Dump;

#endif /* ENUM_H */
