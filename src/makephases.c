void makePhases(const int period, const int offset, int * forwardOffset, int * backOffset, int * doubleOffset, int * tripleOffset)
{
    int i;
    for (i = 0; i < period; i++)
        backOffset[i] = -1;
    i = 0;
    for (;;) {
        int j = offset;
        while (backOffset[(i + j) % period] >= 0 && j < period) j++;
            if (j == period) {
                backOffset[i] = period - i;
                break;
            }
        backOffset[i] = j;
        i = (i+j) % period;
    }
    for (i = 0; i < period; i++)
        forwardOffset[(i + backOffset[i]) % period] = backOffset[i];
        
    for (i = 0; i < period; i++) {
        int j = (i - forwardOffset[i]);
        if (j < 0)
            j += period;
        doubleOffset[i] = forwardOffset[i] + forwardOffset[j];
    }
    for (i = 0; i <  period; i++){
        int j = (i - forwardOffset[i]);
        if (j < 0)
            j += period;
        tripleOffset[i] = forwardOffset[i] + doubleOffset[j];
    }
}
