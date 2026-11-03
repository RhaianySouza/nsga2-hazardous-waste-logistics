#ifndef APLICACAO_H
#define APLICACAO_H

#include "modelo.h"

int lerArgumentos(int argc, char **argv, Configuracao *configuracao,
                  char *erro, size_t tamanhoErro);
void mostrarAjuda(void);
int executarAplicacao(int argc, char **argv);

#endif
