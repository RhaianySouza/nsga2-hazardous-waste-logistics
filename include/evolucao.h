#ifndef EVOLUCAO_H
#define EVOLUCAO_H

#include "aleatorio.h"
#include "modelo.h"

struct Registro;

int alocarPopulacao(Populacao *populacao, int capacidade);
void liberarPopulacao(Populacao *populacao);
int inicializarPopulacao(const Problema *problema, int quantidade,
                         GeradorAleatorio *gerador, Populacao *populacao);
int criarDescendentes(const Populacao *pais, const Problema *problema,
                      GeradorAleatorio *gerador, Populacao *filhos);
int selecionarAmbiente(Populacao *combinada, int tamanhoDesejado,
                       Populacao *proxima);
int executarNSGA2(const Problema *problema, const Configuracao *configuracao,
                  GeradorAleatorio *gerador, Populacao *resultado);
int executarNSGA2ComRegistro(const Problema *problema, const Configuracao *configuracao,
                             GeradorAleatorio *gerador, Populacao *resultado,
                             struct Registro *registro);

#endif
