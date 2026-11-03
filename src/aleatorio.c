#include "aleatorio.h"

#include <stdint.h>

static void atualizarEstado(GeradorAleatorio *gerador) {
    int i;

    for (i = 0; i < TAMANHO_MT; i++) {
        uint32_t alto = gerador->estado[i] & 0x80000000u;
        uint32_t baixo = gerador->estado[(i + 1) % TAMANHO_MT] & 0x7fffffffu;
        uint32_t combinado = alto | baixo;
        uint32_t valor = gerador->estado[(i + 397) % TAMANHO_MT] ^ (combinado >> 1);

        if ((combinado & 1u) != 0u) {
            valor ^= 0x9908b0dfu;
        }
        gerador->estado[i] = valor;
    }
    gerador->indice = 0;
}

void iniciarGerador(GeradorAleatorio *gerador, uint32_t semente) {
    int i;

    gerador->estado[0] = semente;
    for (i = 1; i < TAMANHO_MT; i++) {
        uint32_t anterior = gerador->estado[i - 1];
        gerador->estado[i] = 1812433253u * (anterior ^ (anterior >> 30)) + (uint32_t)i;
    }
    gerador->indice = TAMANHO_MT;
}

uint32_t proximoU32(GeradorAleatorio *gerador) {
    uint32_t valor;

    if (gerador->indice >= TAMANHO_MT) {
        atualizarEstado(gerador);
    }

    valor = gerador->estado[gerador->indice++];
    valor ^= valor >> 11;
    valor ^= (valor << 7) & 0x9d2c5680u;
    valor ^= (valor << 15) & 0xefc60000u;
    valor ^= valor >> 18;
    return valor;
}

int sortearInteiro(GeradorAleatorio *gerador, int minimo, int maximo) {
    uint32_t intervalo = (uint32_t)(maximo - minimo) + 1u;
    uint32_t limite = (uint32_t)(-intervalo) % intervalo;
    uint64_t produto;
    uint32_t parteBaixa;

    do {
        produto = (uint64_t)proximoU32(gerador) * (uint64_t)intervalo;
        parteBaixa = (uint32_t)produto;
    } while (parteBaixa < limite);

    return minimo + (int)(produto >> 32);
}

double sortearUnidade(GeradorAleatorio *gerador) {
    const double base = 4294967296.0;
    double primeiro = (double)proximoU32(gerador);
    double segundo = (double)proximoU32(gerador);

    return (primeiro + segundo * base) / (base * base);
}
