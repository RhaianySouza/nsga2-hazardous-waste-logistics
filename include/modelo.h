#ifndef MODELO_H
#define MODELO_H

#include <stddef.h>
#include <stdint.h>

#define MAX_RESIDUOS 7
#define MAX_INSTALACOES 6
#define MAX_TIPOS 5
#define MAX_NOME 96
#define MAX_CAMINHO 512

typedef struct {
    char nome[MAX_NOME];
    int tipo;
    double quantidadeKg;
} Residuo;

typedef struct {
    char nome[MAX_NOME];
    double capacidade;
    double custoTratamentoKg;
    double riscoLocalKg;
    double distanciaKm;
    double riscoRodoviaKgKm;
    int tiposAceitos[MAX_TIPOS];
    int quantidadeTiposAceitos;
} Instalacao;

typedef struct {
    char nome[MAX_NOME];
    Residuo residuos[MAX_RESIDUOS];
    Instalacao instalacoes[MAX_INSTALACOES];
    int incompatibilidade[MAX_TIPOS][MAX_TIPOS];
    int quantidadeResiduos;
    int quantidadeInstalacoes;
    double capacidadeVeiculo;
    double custoFixoViagem;
    double custoKm;
    int valido;
} Problema;

typedef struct {
    int destino[MAX_RESIDUOS];
    int prioridade[MAX_RESIDUOS];
    double objetivos[3];
    double violacao;
    double crowding;
    int rank;
    int viagens;
} Individuo;

typedef struct {
    Individuo *individuos;
    int tamanho;
    int capacidade;
} Populacao;

typedef struct {
    int populacao;
    int geracoes;
    uint32_t semente;
    char cenario[8];
    char saida[MAX_CAMINHO];
    char nivelLog[16];
    int intervaloLog;
    int salvarIndividuos;
} Configuracao;

typedef struct {
    int geracao;
    int factiveis;
    int factiveisRankUm;
    double menorCusto;
    double menorRiscoTransporte;
    double menorRiscoInstalacao;
} EstatisticaGeracao;

Configuracao criarConfiguracaoPadrao(void);

#endif
