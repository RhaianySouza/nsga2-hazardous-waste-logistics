#include "dominancia.h"
#include "ordenacao.h"

#include <math.h>
#include <stdlib.h>

int individuoDomina(const Individuo *primeiro, const Individuo *segundo) {
    int primeiroFactivel = primeiro->violacao < 1e-8;
    int segundoFactivel = segundo->violacao < 1e-8;
    int estrito = 0;
    int k;

    if (primeiroFactivel != segundoFactivel) {
        return primeiroFactivel;
    }
    if (!primeiroFactivel) {
        return primeiro->violacao + 1e-8 < segundo->violacao;
    }

    for (k = 0; k < 3; k++) {
        if (primeiro->objetivos[k] > segundo->objetivos[k] + 1e-9) {
            return 0;
        }
        if (primeiro->objetivos[k] + 1e-9 < segundo->objetivos[k]) {
            estrito = 1;
        }
    }
    return estrito;
}

typedef struct {
    const Populacao *populacao;
    int objetivo;
} ContextoObjetivo;

static int compararObjetivo(int primeiro, int segundo, void *contexto) {
    ContextoObjetivo *dados = (ContextoObjetivo *)contexto;
    return dados->populacao->individuos[primeiro].objetivos[dados->objetivo] <
           dados->populacao->individuos[segundo].objetivos[dados->objetivo];
}

static int calcularCrowdingIndices(Populacao *populacao, int *indices, int quantidade) {
    int *ordenados;
    int i;
    int k;

    for (i = 0; i < quantidade; i++) {
        populacao->individuos[indices[i]].crowding = 0.0;
    }
    if (quantidade < 3) {
        for (i = 0; i < quantidade; i++) {
            populacao->individuos[indices[i]].crowding = INFINITY;
        }
        return 1;
    }

    ordenados = (int *)malloc((size_t)quantidade * sizeof(int));
    if (ordenados == NULL) {
        return 0;
    }

    for (k = 0; k < 3; k++) {
        double menor;
        double maior;
        ContextoObjetivo contexto;

        for (i = 0; i < quantidade; i++) {
            ordenados[i] = indices[i];
        }
        contexto.populacao = populacao;
        contexto.objetivo = k;
        ordenarIndicesComoCpp(ordenados, quantidade, compararObjetivo, &contexto);
        populacao->individuos[ordenados[0]].crowding = INFINITY;
        populacao->individuos[ordenados[quantidade - 1]].crowding = INFINITY;
        menor = populacao->individuos[ordenados[0]].objetivos[k];
        maior = populacao->individuos[ordenados[quantidade - 1]].objetivos[k];

        if (maior - menor < 1e-12) {
            continue;
        }
        for (i = 1; i + 1 < quantidade; i++) {
            if (!isinf(populacao->individuos[ordenados[i]].crowding)) {
                double seguinte = populacao->individuos[ordenados[i + 1]].objetivos[k];
                double anterior = populacao->individuos[ordenados[i - 1]].objetivos[k];
                populacao->individuos[ordenados[i]].crowding += (seguinte - anterior) / (maior - menor);
            }
        }
    }
    free(ordenados);
    return 1;
}

int calcularCrowding(Populacao *populacao, int rank) {
    int *indices;
    int quantidade = 0;
    int i;

    indices = (int *)malloc((size_t)populacao->tamanho * sizeof(int));
    if (indices == NULL) {
        return 0;
    }
    for (i = 0; i < populacao->tamanho; i++) {
        if (populacao->individuos[i].rank == rank) {
            indices[quantidade++] = i;
        }
    }
    if (!calcularCrowdingIndices(populacao, indices, quantidade)) {
        free(indices);
        return 0;
    }
    free(indices);
    return 1;
}

int classificarPopulacao(Populacao *populacao) {
    int n = populacao->tamanho;
    unsigned char *matriz;
    int *dominadoPor;
    int *frenteAtual;
    int *proximaFrente;
    int quantidadeAtual = 0;
    int rank = 0;
    int i;
    int j;

    if (n <= 0) {
        return 1;
    }
    matriz = (unsigned char *)calloc((size_t)n * (size_t)n, sizeof(unsigned char));
    dominadoPor = (int *)calloc((size_t)n, sizeof(int));
    frenteAtual = (int *)malloc((size_t)n * sizeof(int));
    proximaFrente = (int *)malloc((size_t)n * sizeof(int));
    if (matriz == NULL || dominadoPor == NULL || frenteAtual == NULL || proximaFrente == NULL) {
        free(matriz);
        free(dominadoPor);
        free(frenteAtual);
        free(proximaFrente);
        return 0;
    }

    for (i = 0; i < n; i++) {
        populacao->individuos[i].rank = -1;
        populacao->individuos[i].crowding = 0.0;
        for (j = i + 1; j < n; j++) {
            if (individuoDomina(&populacao->individuos[i], &populacao->individuos[j])) {
                matriz[(size_t)i * (size_t)n + (size_t)j] = 1;
                dominadoPor[j]++;
            } else if (individuoDomina(&populacao->individuos[j], &populacao->individuos[i])) {
                matriz[(size_t)j * (size_t)n + (size_t)i] = 1;
                dominadoPor[i]++;
            }
        }
    }

    for (i = 0; i < n; i++) {
        if (dominadoPor[i] == 0) {
            frenteAtual[quantidadeAtual++] = i;
        }
    }

    while (quantidadeAtual > 0) {
        int quantidadeProxima = 0;

        for (i = 0; i < quantidadeAtual; i++) {
            int indice = frenteAtual[i];
            populacao->individuos[indice].rank = rank;

            for (j = 0; j < n; j++) {
                if (matriz[(size_t)indice * (size_t)n + (size_t)j]) {
                    dominadoPor[j]--;
                    if (dominadoPor[j] == 0) {
                        proximaFrente[quantidadeProxima++] = j;
                    }
                }
            }
        }
        if (!calcularCrowdingIndices(populacao, frenteAtual, quantidadeAtual)) {
            free(matriz);
            free(dominadoPor);
            free(frenteAtual);
            free(proximaFrente);
            return 0;
        }
        for (i = 0; i < quantidadeProxima; i++) {
            frenteAtual[i] = proximaFrente[i];
        }
        quantidadeAtual = quantidadeProxima;
        rank++;
    }

    free(matriz);
    free(dominadoPor);
    free(frenteAtual);
    free(proximaFrente);
    return 1;
}
