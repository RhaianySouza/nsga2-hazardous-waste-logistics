#ifndef ORDENACAO_H
#define ORDENACAO_H

typedef int (*ComparadorIndice)(int primeiro, int segundo, void *contexto);

void ordenarIndicesComoCpp(int *valores, int quantidade,
                           ComparadorIndice comparar, void *contexto);

#endif
