#define SYM_ASYM 1
#define SYM_ODD 2
#define SYM_EVEN 3
#define SYM_GUTTER 4

int slowEvolveBit(const int row1, const int row2, const int row3, const int bshift, const int *table){
    return table[(((row2>>bshift) & 2)<<7) | (((row1>>bshift) & 2)<<6)
               | (((row1>>bshift) & 4)<<4) | (((row2>>bshift) & 4)<<3)
               | (((row3>>bshift) & 7)<<2) | (((row2>>bshift) & 1)<<1)
               |  ((row1>>bshift) & 1)<<0];
}

void fasterTable(const int *table1, char *table2) {
    int p = 0 ;
    for (int row1=0; row1<8; row1++)
        for (int row2=0; row2<8; row2++)
            for (int row3=0; row3<8; row3++)
                table2[p++] = slowEvolveBit(row1, row2, row3, 0, table1) ;
}

int evolveBitShift(const int row1, const int row2, const int row3, const int bshift, const char *table) {
    return table[
        (((row1 << 6) >> bshift) & 0700) +
        (((row2 << 3) >> bshift) &  070) +
        (( row3       >> bshift) &   07)] ;
}

int evolveBit(const int row1, const int row2, const int row3, const char *table) {
    return table[
        ((row1 << 6) & 0700) +
        ((row2 << 3) &  070) +
        ( row3       &   07)] ;
}

int evolveRow(const int row1, const int row2, const int row3, const char *table, const int wi, const int symmetry){
    int row4;
    int row1_s, row2_s, row3_s;
    int s = 0;
    
    if (symmetry == SYM_ODD) s = 1;
    if (evolveBitShift(row1, row2, row3, wi - 1, table))
        return -1;
    if (symmetry == SYM_ASYM && evolveBit(row1 << 2, row2 << 2, row3 << 2, table))
        return -1;
        
    row1_s = (row1 << 1);
    row2_s = (row2 << 1);
    row3_s = (row3 << 1);
    if (symmetry == SYM_ODD || symmetry == SYM_EVEN){
        row1_s += ((row1 >> s) & 1);
        row2_s += ((row2 >> s) & 1);
        row3_s += ((row3 >> s) & 1);
    }
    row4 = evolveBit(row1_s, row2_s, row3_s, table);
    for (int j = 1; j < wi; j++)
        row4 += evolveBitShift(row1, row2, row3, j - 1, table) << j;
    return row4;
}

int evolveRowHigh(const int row1, const int row2, const int row3, const int bits, const char *table, const int wi){
    int row4 = 0;
    
    if (evolveBitShift(row1, row2, row3, wi - 1, table))
        return -1;
    for (int j = wi-bits; j < wi; j++)
        row4 += evolveBitShift(row1, row2, row3, j - 1, table) << j;
    return row4;
}

int evolveRowLow(const int row1, const int row2, const int row3, const int bits, const char *table, const int symmetry){
    int row4;
    int row1_s,row2_s,row3_s;
    int s = 0;
    
    if (symmetry == SYM_ODD) s = 1;
    if (symmetry == SYM_ASYM && evolveBit(row1 << 2, row2 << 2, row3 << 2, table))
        return -1;
   
    row1_s = (row1 << 1);
    row2_s = (row2 << 1);
    row3_s = (row3 << 1);
    if (symmetry == SYM_ODD || symmetry == SYM_EVEN){
        row1_s += ((row1 >> s) & 1);
        row2_s += ((row2 >> s) & 1);
        row3_s += ((row3 >> s) & 1);
    }
    row4 = evolveBit(row1_s, row2_s, row3_s, table);
    for (int j = 1; j < bits; j++)
        row4 += evolveBitShift(row1, row2, row3, j - 1, table) << j;
    return row4;
}

