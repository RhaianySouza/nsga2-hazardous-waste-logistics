#ifndef EXPORTACAO_H
#define EXPORTACAO_H

#include "modelo.h"

int salvarResultados(const Problema *problema, Populacao *populacao,
                     const char *pasta, int *quantidadePareto,
                     char *erro, size_t tamanhoErro);

#endif
