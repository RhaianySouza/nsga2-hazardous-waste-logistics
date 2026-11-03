#ifndef ALEATORIO_H
#define ALEATORIO_H

#include <stdint.h>

#define TAMANHO_MT 624

typedef struct {
    uint32_t estado[TAMANHO_MT];
    int indice;
} GeradorAleatorio;

void iniciarGerador(GeradorAleatorio *gerador, uint32_t semente);
uint32_t proximoU32(GeradorAleatorio *gerador);
int sortearInteiro(GeradorAleatorio *gerador, int minimo, int maximo);
double sortearUnidade(GeradorAleatorio *gerador);

#endif
