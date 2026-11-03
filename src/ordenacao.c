#include "ordenacao.h"

#define LIMITE_INSERCAO 16

static void trocar(int *primeiro, int *segundo) {
    int temporario = *primeiro;
    *primeiro = *segundo;
    *segundo = temporario;
}

static void moverMedianaParaInicio(int *valores, int inicio, int a, int b, int c,
                                   ComparadorIndice comparar, void *contexto) {
    if (comparar(valores[a], valores[b], contexto)) {
        if (comparar(valores[b], valores[c], contexto)) {
            trocar(&valores[inicio], &valores[b]);
        } else if (comparar(valores[a], valores[c], contexto)) {
            trocar(&valores[inicio], &valores[c]);
        } else {
            trocar(&valores[inicio], &valores[a]);
        }
    } else if (comparar(valores[a], valores[c], contexto)) {
        trocar(&valores[inicio], &valores[a]);
    } else if (comparar(valores[b], valores[c], contexto)) {
        trocar(&valores[inicio], &valores[c]);
    } else {
        trocar(&valores[inicio], &valores[b]);
    }
}

static int particionarSemGuarda(int *valores, int primeiro, int ultimo, int pivo,
                                ComparadorIndice comparar, void *contexto) {
    while (1) {
        while (comparar(valores[primeiro], valores[pivo], contexto)) {
            primeiro++;
        }
        ultimo--;
        while (comparar(valores[pivo], valores[ultimo], contexto)) {
            ultimo--;
        }
        if (primeiro >= ultimo) {
            return primeiro;
        }
        trocar(&valores[primeiro], &valores[ultimo]);
        primeiro++;
    }
}

static int particionarPivo(int *valores, int primeiro, int ultimo,
                           ComparadorIndice comparar, void *contexto) {
    int meio = primeiro + (ultimo - primeiro) / 2;
    int segundo = primeiro + 1;

    moverMedianaParaInicio(valores, primeiro, segundo, meio, ultimo - 1,
                           comparar, contexto);
    return particionarSemGuarda(valores, segundo, ultimo, primeiro, comparar, contexto);
}

static void inserirSemGuarda(int *valores, int posicao,
                             ComparadorIndice comparar, void *contexto) {
    int valor = valores[posicao];
    int anterior = posicao - 1;

    while (comparar(valor, valores[anterior], contexto)) {
        valores[posicao] = valores[anterior];
        posicao = anterior;
        anterior--;
    }
    valores[posicao] = valor;
}

static void ordenarInsercao(int *valores, int primeiro, int ultimo,
                            ComparadorIndice comparar, void *contexto) {
    int i;

    if (primeiro == ultimo) {
        return;
    }
    for (i = primeiro + 1; i < ultimo; i++) {
        if (comparar(valores[i], valores[primeiro], contexto)) {
            int valor = valores[i];
            int j;
            for (j = i; j > primeiro; j--) {
                valores[j] = valores[j - 1];
            }
            valores[primeiro] = valor;
        } else {
            inserirSemGuarda(valores, i, comparar, contexto);
        }
    }
}

static void ajustarHeap(int *valores, int inicio, int quantidade, int raiz,
                        ComparadorIndice comparar, void *contexto) {
    int maior = raiz;

    while (1) {
        int esquerda = 2 * raiz + 1;
        int direita = esquerda + 1;

        if (esquerda < quantidade &&
            comparar(valores[inicio + maior], valores[inicio + esquerda], contexto)) {
            maior = esquerda;
        }
        if (direita < quantidade &&
            comparar(valores[inicio + maior], valores[inicio + direita], contexto)) {
            maior = direita;
        }
        if (maior == raiz) {
            return;
        }
        trocar(&valores[inicio + raiz], &valores[inicio + maior]);
        raiz = maior;
    }
}

static void ordenarHeap(int *valores, int primeiro, int ultimo,
                        ComparadorIndice comparar, void *contexto) {
    int quantidade = ultimo - primeiro;
    int i;

    for (i = quantidade / 2 - 1; i >= 0; i--) {
        ajustarHeap(valores, primeiro, quantidade, i, comparar, contexto);
    }
    for (i = quantidade - 1; i > 0; i--) {
        trocar(&valores[primeiro], &valores[primeiro + i]);
        ajustarHeap(valores, primeiro, i, 0, comparar, contexto);
    }
}

static void introSort(int *valores, int primeiro, int ultimo, int limiteProfundidade,
                      ComparadorIndice comparar, void *contexto) {
    while (ultimo - primeiro > LIMITE_INSERCAO) {
        int corte;

        if (limiteProfundidade == 0) {
            ordenarHeap(valores, primeiro, ultimo, comparar, contexto);
            return;
        }
        limiteProfundidade--;
        corte = particionarPivo(valores, primeiro, ultimo, comparar, contexto);
        introSort(valores, corte, ultimo, limiteProfundidade, comparar, contexto);
        ultimo = corte;
    }
}

static void finalizarInsercao(int *valores, int quantidade,
                              ComparadorIndice comparar, void *contexto) {
    int i;

    if (quantidade > LIMITE_INSERCAO) {
        ordenarInsercao(valores, 0, LIMITE_INSERCAO, comparar, contexto);
        for (i = LIMITE_INSERCAO; i < quantidade; i++) {
            inserirSemGuarda(valores, i, comparar, contexto);
        }
    } else {
        ordenarInsercao(valores, 0, quantidade, comparar, contexto);
    }
}

void ordenarIndicesComoCpp(int *valores, int quantidade,
                           ComparadorIndice comparar, void *contexto) {
    int logaritmo = 0;
    int valor = quantidade;

    if (quantidade <= 1) {
        return;
    }
    while (valor > 1) {
        valor >>= 1;
        logaritmo++;
    }
    introSort(valores, 0, quantidade, logaritmo * 2, comparar, contexto);
    finalizarInsercao(valores, quantidade, comparar, contexto);
}
