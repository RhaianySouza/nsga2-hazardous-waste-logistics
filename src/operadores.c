#include "operadores.h"

#include "avaliacao.h"
#include "problema.h"

#include <string.h>

typedef struct {
    double valor;
    int indice;
} ChavePrioridade;

static void trocarInteiros(int *primeiro, int *segundo) {
    int temporario = *primeiro;
    *primeiro = *segundo;
    *segundo = temporario;
}

/* Reproduz a estrategia em pares usada por std::shuffle do libstdc++. */
static void embaralharPrioridades(int *valores, int quantidade, GeradorAleatorio *gerador) {
    int i = 1;

    if (quantidade <= 1) {
        return;
    }
    if ((quantidade % 2) == 0) {
        int posicao = sortearInteiro(gerador, 0, 1);
        trocarInteiros(&valores[i], &valores[posicao]);
        i++;
    }
    while (i < quantidade) {
        int intervalo = i + 1;
        int sorteio = sortearInteiro(gerador, 0, intervalo * (intervalo + 1) - 1);
        int primeiraPosicao = sorteio / (intervalo + 1);
        int segundaPosicao = sorteio % (intervalo + 1);

        trocarInteiros(&valores[i], &valores[primeiraPosicao]);
        i++;
        trocarInteiros(&valores[i], &valores[segundaPosicao]);
        i++;
    }
}

Individuo criarIndividuoAleatorio(const Problema *problema, GeradorAleatorio *gerador) {
    Individuo individuo;
    int i;

    memset(&individuo, 0, sizeof(individuo));
    for (i = 0; i < problema->quantidadeResiduos; i++) {
        int opcoes[MAX_INSTALACOES];
        int quantidadeOpcoes = 0;
        int f;

        for (f = 0; f < problema->quantidadeInstalacoes; f++) {
            if (instalacaoAceita(&problema->instalacoes[f], problema->residuos[i].tipo)) {
                opcoes[quantidadeOpcoes++] = f;
            }
        }
        individuo.destino[i] = opcoes[sortearInteiro(gerador, 0, quantidadeOpcoes - 1)];
        individuo.prioridade[i] = i;
    }

    embaralharPrioridades(individuo.prioridade, problema->quantidadeResiduos, gerador);
    avaliarIndividuo(&individuo, problema);
    return individuo;
}

int selecionarTorneio(const Populacao *populacao, GeradorAleatorio *gerador) {
    int a = sortearInteiro(gerador, 0, populacao->tamanho - 1);
    int b = sortearInteiro(gerador, 0, populacao->tamanho - 1);
    const Individuo *primeiro = &populacao->individuos[a];
    const Individuo *segundo = &populacao->individuos[b];

    if (primeiro->rank != segundo->rank) {
        return primeiro->rank < segundo->rank ? a : b;
    }
    if (primeiro->crowding != segundo->crowding) {
        return primeiro->crowding > segundo->crowding ? a : b;
    }
    return sortearInteiro(gerador, 0, 1) ? a : b;
}

static void ordenarChaves(ChavePrioridade *chaves, int quantidade) {
    int i;

    for (i = 1; i < quantidade; i++) {
        ChavePrioridade atual = chaves[i];
        int j = i - 1;

        while (j >= 0 && chaves[j].valor > atual.valor) {
            chaves[j + 1] = chaves[j];
            j--;
        }
        chaves[j + 1] = atual;
    }
}

Individuo cruzarIndividuos(const Individuo *primeiro, const Individuo *segundo,
                           GeradorAleatorio *gerador, int quantidadeGenes) {
    Individuo filho = *primeiro;
    ChavePrioridade chaves[MAX_RESIDUOS];
    int i;

    for (i = 0; i < quantidadeGenes; i++) {
        if (sortearUnidade(gerador) < 0.5) {
            filho.destino[i] = segundo->destino[i];
        }
    }
    for (i = 0; i < quantidadeGenes; i++) {
        int prioridade = sortearUnidade(gerador) < 0.5
                        ? primeiro->prioridade[i]
                        : segundo->prioridade[i];
        chaves[i].valor = (double)prioridade + sortearUnidade(gerador) * 0.1;
        chaves[i].indice = i;
    }
    ordenarChaves(chaves, quantidadeGenes);
    for (i = 0; i < quantidadeGenes; i++) {
        filho.prioridade[chaves[i].indice] = i;
    }
    return filho;
}

void mutarIndividuo(Individuo *individuo, const Problema *problema,
                    GeradorAleatorio *gerador) {
    int i;

    for (i = 0; i < problema->quantidadeResiduos; i++) {
        if (sortearUnidade(gerador) < 0.14) {
            int opcoes[MAX_INSTALACOES];
            int quantidadeOpcoes = 0;
            int f;

            for (f = 0; f < problema->quantidadeInstalacoes; f++) {
                if (instalacaoAceita(&problema->instalacoes[f], problema->residuos[i].tipo)) {
                    opcoes[quantidadeOpcoes++] = f;
                }
            }
            individuo->destino[i] = opcoes[sortearInteiro(gerador, 0, quantidadeOpcoes - 1)];
        }
    }
    if (sortearUnidade(gerador) < 0.4) {
        int a = sortearInteiro(gerador, 0, problema->quantidadeResiduos - 1);
        int b = sortearInteiro(gerador, 0, problema->quantidadeResiduos - 1);
        trocarInteiros(&individuo->prioridade[a], &individuo->prioridade[b]);
    }
}
