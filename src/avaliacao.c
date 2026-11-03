#include "avaliacao.h"

#include "problema.h"

#include <string.h>

typedef struct {
    double carga;
    int residuos[MAX_RESIDUOS];
    int quantidadeResiduos;
} Viagem;

static void ordenarPorPrioridade(int *indices, int quantidade, const Individuo *individuo) {
    int i;

    for (i = 1; i < quantidade; i++) {
        int atual = indices[i];
        int j = i - 1;

        while (j >= 0 && individuo->prioridade[indices[j]] > individuo->prioridade[atual]) {
            indices[j + 1] = indices[j];
            j--;
        }
        indices[j + 1] = atual;
    }
}

/* Calcula custo, riscos, violacoes e quantidade de viagens. */
void avaliarIndividuo(Individuo *individuo, const Problema *problema) {
    double cargas[MAX_INSTALACOES] = {0};
    int grupos[MAX_INSTALACOES][MAX_RESIDUOS];
    int tamanhos[MAX_INSTALACOES] = {0};
    int i;
    int f;

    individuo->objetivos[0] = 0.0;
    individuo->objetivos[1] = 0.0;
    individuo->objetivos[2] = 0.0;
    individuo->violacao = 0.0;
    individuo->viagens = 0;

    for (i = 0; i < problema->quantidadeResiduos; i++) {
        int destino = individuo->destino[i];
        const Residuo *residuo;
        const Instalacao *instalacao;

        if (destino < 0 || destino >= problema->quantidadeInstalacoes) {
            individuo->violacao += 10000.0;
            continue;
        }

        residuo = &problema->residuos[i];
        instalacao = &problema->instalacoes[destino];
        cargas[destino] += residuo->quantidadeKg;
        grupos[destino][tamanhos[destino]++] = i;

        if (!instalacaoAceita(instalacao, residuo->tipo)) {
            individuo->violacao += 1000.0 + residuo->quantidadeKg;
        }
        individuo->objetivos[0] += residuo->quantidadeKg * instalacao->custoTratamentoKg;
        individuo->objetivos[2] += residuo->quantidadeKg * instalacao->riscoLocalKg;
    }

    for (f = 0; f < problema->quantidadeInstalacoes; f++) {
        Viagem viagens[MAX_RESIDUOS];
        int quantidadeViagens = 0;
        int g;

        memset(viagens, 0, sizeof(viagens));
        if (cargas[f] > problema->instalacoes[f].capacidade) {
            individuo->violacao += cargas[f] - problema->instalacoes[f].capacidade;
        }

        ordenarPorPrioridade(grupos[f], tamanhos[f], individuo);
        for (g = 0; g < tamanhos[f]; g++) {
            int indiceResiduo = grupos[f][g];
            const Residuo *residuo = &problema->residuos[indiceResiduo];
            int colocado = 0;
            int v;

            if (residuo->quantidadeKg > problema->capacidadeVeiculo) {
                individuo->violacao += residuo->quantidadeKg - problema->capacidadeVeiculo;
                continue;
            }

            for (v = 0; v < quantidadeViagens && !colocado; v++) {
                int compativel = 1;
                int r;

                if (viagens[v].carga + residuo->quantidadeKg > problema->capacidadeVeiculo + 1e-9) {
                    continue;
                }
                for (r = 0; r < viagens[v].quantidadeResiduos; r++) {
                    int outro = viagens[v].residuos[r];
                    int tipoOutro = problema->residuos[outro].tipo;

                    if (problema->incompatibilidade[residuo->tipo][tipoOutro]) {
                        compativel = 0;
                        break;
                    }
                }
                if (compativel) {
                    viagens[v].residuos[viagens[v].quantidadeResiduos++] = indiceResiduo;
                    viagens[v].carga += residuo->quantidadeKg;
                    colocado = 1;
                }
            }

            if (!colocado) {
                viagens[quantidadeViagens].carga = residuo->quantidadeKg;
                viagens[quantidadeViagens].residuos[0] = indiceResiduo;
                viagens[quantidadeViagens].quantidadeResiduos = 1;
                quantidadeViagens++;
            }
        }

        for (g = 0; g < quantidadeViagens; g++) {
            double km = 2.0 * problema->instalacoes[f].distanciaKm;

            individuo->objetivos[0] += problema->custoFixoViagem + km * problema->custoKm;
            individuo->objetivos[1] += km * problema->instalacoes[f].riscoRodoviaKgKm * viagens[g].carga;
            individuo->viagens++;
        }
    }
}
