#ifndef REGISTRO_H
#define REGISTRO_H

#include "modelo.h"

#include <stdio.h>

typedef struct Registro {
    Configuracao configuracao;
    FILE *etapas;
    FILE *geracoes;
    FILE *individuos;
} Registro;

int criarDiretorios(const char *caminho, char *erro, size_t tamanhoErro);
int abrirRegistro(Registro *registro, const Configuracao *configuracao,
                  const char *pasta, char *erro, size_t tamanhoErro);
void fecharRegistro(Registro *registro);
int registroDetalhado(const Registro *registro);
int registroCompleto(const Registro *registro);
void registrarEntradaCenario(Registro *registro, const Problema *problema);
void registrarInicioGeracao(Registro *registro, int geracao, const Populacao *populacao);
void registrarEtapa(Registro *registro, int geracao, const char *nome,
                    const char *entrada, const char *saida);
void registrarIndividuo(Registro *registro, int geracao, const char *etapa,
                        int indice, const Individuo *individuo);
EstatisticaGeracao resumirPopulacao(int geracao, const Populacao *populacao);
void registrarFimGeracao(Registro *registro, const EstatisticaGeracao *estatistica);

#endif
