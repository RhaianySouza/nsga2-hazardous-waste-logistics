#include "evolucao.h"

#include "avaliacao.h"
#include "dominancia.h"
#include "operadores.h"
#include "ordenacao.h"
#include "registro.h"

#include <stdlib.h>
#include <string.h>

int alocarPopulacao(Populacao *populacao, int capacidade) {
    if (populacao == NULL || capacidade <= 0) {
        return 0;
    }
    populacao->individuos = (Individuo *)calloc((size_t)capacidade, sizeof(Individuo));
    if (populacao->individuos == NULL) {
        populacao->tamanho = 0;
        populacao->capacidade = 0;
        return 0;
    }
    populacao->tamanho = 0;
    populacao->capacidade = capacidade;
    return 1;
}

void liberarPopulacao(Populacao *populacao) {
    if (populacao == NULL) {
        return;
    }
    free(populacao->individuos);
    populacao->individuos = NULL;
    populacao->tamanho = 0;
    populacao->capacidade = 0;
}

int inicializarPopulacao(const Problema *problema, int quantidade,
                         GeradorAleatorio *gerador, Populacao *populacao) {
    int i;

    if (!alocarPopulacao(populacao, quantidade)) {
        return 0;
    }
    for (i = 0; i < quantidade; i++) {
        populacao->individuos[i] = criarIndividuoAleatorio(problema, gerador);
    }
    populacao->tamanho = quantidade;
    return 1;
}

int criarDescendentes(const Populacao *pais, const Problema *problema,
                      GeradorAleatorio *gerador, Populacao *filhos) {
    int i;

    if (!alocarPopulacao(filhos, pais->tamanho)) {
        return 0;
    }
    for (i = 0; i < pais->tamanho; i++) {
        int b = selecionarTorneio(pais, gerador);
        int a = selecionarTorneio(pais, gerador);
        Individuo filho = cruzarIndividuos(&pais->individuos[a], &pais->individuos[b],
                                           gerador, problema->quantidadeResiduos);

        mutarIndividuo(&filho, problema, gerador);
        avaliarIndividuo(&filho, problema);
        filhos->individuos[i] = filho;
    }
    filhos->tamanho = pais->tamanho;
    return 1;
}

static int compararCrowding(int primeiro, int segundo, void *contexto) {
    const Populacao *populacao = (const Populacao *)contexto;
    return populacao->individuos[primeiro].crowding >
           populacao->individuos[segundo].crowding;
}

int selecionarAmbiente(Populacao *combinada, int tamanhoDesejado,
                       Populacao *proxima) {
    int maiorRank = -1;
    int rank;
    int i;

    if (!classificarPopulacao(combinada) || !alocarPopulacao(proxima, tamanhoDesejado)) {
        return 0;
    }
    for (i = 0; i < combinada->tamanho; i++) {
        if (combinada->individuos[i].rank > maiorRank) {
            maiorRank = combinada->individuos[i].rank;
        }
    }

    for (rank = 0; rank <= maiorRank && proxima->tamanho < tamanhoDesejado; rank++) {
        int *indices = (int *)malloc((size_t)combinada->tamanho * sizeof(int));
        int quantidade = 0;

        if (indices == NULL) {
            liberarPopulacao(proxima);
            return 0;
        }
        for (i = 0; i < combinada->tamanho; i++) {
            if (combinada->individuos[i].rank == rank) {
                indices[quantidade++] = i;
            }
        }
        ordenarIndicesComoCpp(indices, quantidade, compararCrowding, combinada);
        for (i = 0; i < quantidade && proxima->tamanho < tamanhoDesejado; i++) {
            proxima->individuos[proxima->tamanho++] = combinada->individuos[indices[i]];
        }
        free(indices);
    }
    return proxima->tamanho == tamanhoDesejado;
}

int executarNSGA2ComRegistro(const Problema *problema, const Configuracao *configuracao,
                             GeradorAleatorio *gerador, Populacao *resultado,
                             Registro *registro) {
    Populacao atual = {0};
    int geracao;
    int i;
    char entrada[160];
    char saida[160];

    if (!inicializarPopulacao(problema, configuracao->populacao, gerador, &atual)) {
        return 0;
    }
    if (!classificarPopulacao(&atual)) {
        liberarPopulacao(&atual);
        return 0;
    }
    if (registro != NULL) {
        for (i = 0; i < atual.tamanho; i++) {
            registrarIndividuo(registro, 0, "populacao_inicial", i, &atual.individuos[i]);
        }
        snprintf(entrada, sizeof(entrada), "problema + seed + N=%d", configuracao->populacao);
        snprintf(saida, sizeof(saida), "P0=%d individuos avaliados", atual.tamanho);
        registrarEtapa(registro, 0, "01_populacao_inicial", entrada, saida);
        snprintf(entrada, sizeof(entrada), "lotes=%d + destinos elegiveis", problema->quantidadeResiduos);
        registrarEtapa(registro, 0, "02_codificacao_cromossomos", entrada,
                       "genes: facility[lote] e priority[lote] para cada individuo");
        for (i = 0; i < atual.tamanho; i++) {
            registrarIndividuo(registro, 0, "populacao_classificada", i, &atual.individuos[i]);
        }
        snprintf(entrada, sizeof(entrada), "P0=%d cromossomos", atual.tamanho);
        registrarEtapa(registro, 0, "03_avaliacao_e_classificacao", entrada,
                       "tres objetivos, violacoes, ranking, crowding e numero de viagens");
        {
            EstatisticaGeracao estatistica = resumirPopulacao(0, &atual);
            registrarFimGeracao(registro, &estatistica);
        }
    }

    for (geracao = 1; geracao <= configuracao->geracoes; geracao++) {
        Populacao filhos = {0};
        Populacao combinada = {0};
        Populacao proxima = {0};
        int tamanhoCombinado;

        if (registro != NULL) {
            registrarInicioGeracao(registro, geracao, &atual);
        }
        if (!classificarPopulacao(&atual) ||
            !criarDescendentes(&atual, problema, gerador, &filhos) ||
            !alocarPopulacao(&combinada, atual.tamanho + filhos.tamanho)) {
            liberarPopulacao(&filhos);
            liberarPopulacao(&combinada);
            liberarPopulacao(&atual);
            return 0;
        }

        memcpy(combinada.individuos, atual.individuos,
               (size_t)atual.tamanho * sizeof(Individuo));
        memcpy(combinada.individuos + atual.tamanho, filhos.individuos,
               (size_t)filhos.tamanho * sizeof(Individuo));
        combinada.tamanho = atual.tamanho + filhos.tamanho;
        tamanhoCombinado = combinada.tamanho;

        if (registro != NULL) {
            snprintf(entrada, sizeof(entrada), "P=%d cromossomos avaliados", atual.tamanho);
            registrarEtapa(registro, geracao, "03_avaliacao_e_classificacao", entrada,
                           "objetivos, violacoes, ranking e crowding disponiveis");
            for (i = 0; i < filhos.tamanho; i++) {
                registrarIndividuo(registro, geracao, "descendente", i, &filhos.individuos[i]);
            }
            snprintf(entrada, sizeof(entrada), "P=%d classificados", atual.tamanho);
            snprintf(saida, sizeof(saida), "pares selecionados=%d", filhos.tamanho);
            registrarEtapa(registro, geracao, "04_selecao_torneio", entrada, saida);
            snprintf(saida, sizeof(saida), "descendentes codificados=%d", filhos.tamanho);
            registrarEtapa(registro, geracao, "05_cruzamento", "pares de cromossomos parentais", saida);
            snprintf(saida, sizeof(saida), "genes modificados probabilisticamente; descendentes avaliados=%d", filhos.tamanho);
            registrarEtapa(registro, geracao, "06_mutacao", "genes dos descendentes", saida);
        }

        if (!selecionarAmbiente(&combinada, configuracao->populacao, &proxima)) {
            liberarPopulacao(&filhos);
            liberarPopulacao(&combinada);
            liberarPopulacao(&atual);
            return 0;
        }

        liberarPopulacao(&filhos);
        liberarPopulacao(&combinada);
        liberarPopulacao(&atual);
        atual = proxima;
        if (!classificarPopulacao(&atual)) {
            liberarPopulacao(&atual);
            return 0;
        }
        if (registro != NULL) {
            snprintf(entrada, sizeof(entrada), "R=%d solucoes", tamanhoCombinado);
            snprintf(saida, sizeof(saida), "populacao selecionada=%d", atual.tamanho);
            registrarEtapa(registro, geracao, "07_ordenacao_nao_dominancia", entrada, saida);
            registrarEtapa(registro, geracao, "08_distancia_aglomeracao",
                           "objetivos das solucoes por frente", "distancias calculadas nos 3 objetivos");
            registrarEtapa(registro, geracao, "09_selecao_ambiental", entrada, saida);
            for (i = 0; i < atual.tamanho; i++) {
                registrarIndividuo(registro, geracao, "populacao_final_geracao", i,
                                   &atual.individuos[i]);
            }
            {
                EstatisticaGeracao estatistica = resumirPopulacao(geracao, &atual);
                registrarFimGeracao(registro, &estatistica);
            }
            snprintf(entrada, sizeof(entrada), "geracao=%d limite=%d", geracao,
                     configuracao->geracoes);
            registrarEtapa(registro, geracao, "10_criterio_parada", entrada,
                           geracao == configuracao->geracoes
                           ? "encerrar e extrair frente final" : "continuar evolucao");
        }
    }

    *resultado = atual;
    return 1;
}

int executarNSGA2(const Problema *problema, const Configuracao *configuracao,
                  GeradorAleatorio *gerador, Populacao *resultado) {
    return executarNSGA2ComRegistro(problema, configuracao, gerador, resultado, NULL);
}
