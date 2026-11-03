#include "exportacao.h"

#include "dominancia.h"
#include "registro.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int compararIndividuos(const Individuo *primeiro, const Individuo *segundo) {
    int k;

    for (k = 0; k < 3; k++) {
        if (primeiro->objetivos[k] < segundo->objetivos[k]) {
            return -1;
        }
        if (primeiro->objetivos[k] > segundo->objetivos[k]) {
            return 1;
        }
    }
    return 0;
}

static void ordenarFrente(Individuo *frente, int quantidade) {
    int i;

    for (i = 1; i < quantidade; i++) {
        Individuo atual = frente[i];
        int j = i - 1;

        while (j >= 0 && compararIndividuos(&frente[j], &atual) > 0) {
            frente[j + 1] = frente[j];
            j--;
        }
        frente[j + 1] = atual;
    }
}

static int individuosIguais(const Individuo *primeiro, const Individuo *segundo) {
    return memcmp(primeiro->objetivos, segundo->objetivos, sizeof(primeiro->objetivos)) == 0 &&
           memcmp(primeiro->destino, segundo->destino, sizeof(primeiro->destino)) == 0;
}

static FILE *abrirArquivo(const char *pasta, const char *nome,
                          char *erro, size_t tamanhoErro) {
    char caminho[MAX_CAMINHO];
    FILE *arquivo;

    snprintf(caminho, sizeof(caminho), "%s/%s", pasta, nome);
    arquivo = fopen(caminho, "w");
    if (arquivo == NULL) {
        snprintf(erro, tamanhoErro, "Nao foi possivel abrir: %s", caminho);
    }
    return arquivo;
}

int salvarResultados(const Problema *problema, Populacao *populacao,
                     const char *pasta, int *quantidadePareto,
                     char *erro, size_t tamanhoErro) {
    Individuo *frente;
    int quantidade = 0;
    int unicos = 0;
    int i;
    FILE *pareto;
    FILE *inventario;
    FILE *instalacoes;

    if (!criarDiretorios(pasta, erro, tamanhoErro) || !classificarPopulacao(populacao)) {
        return 0;
    }
    frente = (Individuo *)malloc((size_t)populacao->tamanho * sizeof(Individuo));
    if (frente == NULL) {
        snprintf(erro, tamanhoErro, "Falha ao alocar frente de Pareto");
        return 0;
    }
    for (i = 0; i < populacao->tamanho; i++) {
        if (populacao->individuos[i].rank == 0 && populacao->individuos[i].violacao < 1e-8) {
            frente[quantidade++] = populacao->individuos[i];
        }
    }
    ordenarFrente(frente, quantidade);
    for (i = 0; i < quantidade; i++) {
        if (unicos == 0 || !individuosIguais(&frente[i], &frente[unicos - 1])) {
            frente[unicos++] = frente[i];
        }
    }

    pareto = abrirArquivo(pasta, "pareto.csv", erro, tamanhoErro);
    inventario = abrirArquivo(pasta, "inventory.csv", erro, tamanhoErro);
    instalacoes = abrirArquivo(pasta, "facilities_SIMULATED.csv", erro, tamanhoErro);
    if (pareto == NULL || inventario == NULL || instalacoes == NULL) {
        if (pareto != NULL) fclose(pareto);
        if (inventario != NULL) fclose(inventario);
        if (instalacoes != NULL) fclose(instalacoes);
        free(frente);
        return 0;
    }

    fprintf(pareto, "solution,cost_simulated,risk_transport_simulated,risk_facility_simulated,trips,violation,assignment\n");
    for (i = 0; i < unicos; i++) {
        int j;
        fprintf(pareto, "%d,%.4f,%.4f,%.4f,%d,%.4f,\"", i + 1,
                frente[i].objetivos[0], frente[i].objetivos[1], frente[i].objetivos[2],
                frente[i].viagens, frente[i].violacao);
        for (j = 0; j < problema->quantidadeResiduos; j++) {
            if (j > 0) fputc(';', pareto);
            fprintf(pareto, "%s->%s", problema->residuos[j].nome,
                    problema->instalacoes[frente[i].destino[j]].nome);
        }
        fprintf(pareto, "\"\n");
    }

    fprintf(inventario, "lot,type,quantity_kg,origin\n");
    for (i = 0; i < problema->quantidadeResiduos; i++) {
        fprintf(inventario, "%s,%d,%g,%s\n", problema->residuos[i].nome,
                problema->residuos[i].tipo, problema->residuos[i].quantidadeKg,
                problema->nome[0] == 'A' ? "HYPOTHETICAL" : "JAPAN_SAMPLE_APPROXIMATE");
    }

    fprintf(instalacoes, "facility,capacity_kg,cost_per_kg,risk_per_kg,one_way_km,road_risk_coefficient,accepted_types\n");
    for (i = 0; i < problema->quantidadeInstalacoes; i++) {
        int t;
        const Instalacao *instalacao = &problema->instalacoes[i];
        fprintf(instalacoes, "%s,%g,%g,%g,%g,%g,\"", instalacao->nome,
                instalacao->capacidade, instalacao->custoTratamentoKg,
                instalacao->riscoLocalKg, instalacao->distanciaKm,
                instalacao->riscoRodoviaKgKm);
        for (t = 0; t < instalacao->quantidadeTiposAceitos; t++) {
            fprintf(instalacoes, "%d ", instalacao->tiposAceitos[t]);
        }
        fprintf(instalacoes, "\"\n");
    }

    fclose(pareto);
    fclose(inventario);
    fclose(instalacoes);
    if (quantidadePareto != NULL) {
        *quantidadePareto = unicos;
    }

    printf("\n[RESULTADO FINAL] %s: %d solucoes nao dominadas factiveis (deduplicadas)\n",
           problema->nome, unicos);
    if (unicos > 0) {
        int melhor = 0;
        for (i = 1; i < unicos; i++) {
            if (frente[i].objetivos[0] < frente[melhor].objetivos[0]) {
                melhor = i;
            }
        }
        printf("Menor custo na frente: %.2f | risco transporte=%.2f | risco instalacoes=%.2f | viagens=%d\n",
               frente[melhor].objetivos[0], frente[melhor].objetivos[1],
               frente[melhor].objetivos[2], frente[melhor].viagens);
    }
    printf("Arquivos CSV: %s\n", pasta);
    free(frente);
    return 1;
}
