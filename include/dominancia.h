#ifndef DOMINANCIA_H
#define DOMINANCIA_H

#include "modelo.h"

int individuoDomina(const Individuo *primeiro, const Individuo *segundo);
int classificarPopulacao(Populacao *populacao);
int calcularCrowding(Populacao *populacao, int rank);

#endif
