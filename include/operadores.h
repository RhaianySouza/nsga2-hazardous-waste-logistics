#ifndef OPERADORES_H
#define OPERADORES_H

#include "aleatorio.h"
#include "modelo.h"

Individuo criarIndividuoAleatorio(const Problema *problema, GeradorAleatorio *gerador);
int selecionarTorneio(const Populacao *populacao, GeradorAleatorio *gerador);
Individuo cruzarIndividuos(const Individuo *primeiro, const Individuo *segundo,
                           GeradorAleatorio *gerador, int quantidadeGenes);
void mutarIndividuo(Individuo *individuo, const Problema *problema,
                    GeradorAleatorio *gerador);

#endif
