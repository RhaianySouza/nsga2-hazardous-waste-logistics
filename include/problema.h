#ifndef PROBLEMA_H
#define PROBLEMA_H

#include "modelo.h"

Problema criarProblema(char cenario);
int instalacaoAceita(const Instalacao *instalacao, int tipo);
int validarProblema(const Problema *problema, char *erro, size_t tamanhoErro);

#endif
