#ifndef LOAD_H
#define LOAD_H

void loadFail(const char * file);
signed int loadInt(FILE *fp, const char * file);
unsigned int loadUInt(FILE *fp, const char * file);

void loadParams(int *params, const char *file, int *newLastDeep, char *rule, char *dumpRoot);

#endif /* LOAD_H */
