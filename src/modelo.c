#include "modelo.h"

#include <string.h>

/* Cria os mesmos valores padrao usados pela versao C++. */
Configuracao criarConfiguracaoPadrao(void) {
    Configuracao configuracao;

    memset(&configuracao, 0, sizeof(configuracao));
    configuracao.populacao = 100;
    configuracao.geracoes = 200;
    configuracao.semente = 20261102u;
    strcpy(configuracao.cenario, "both");
    strcpy(configuracao.saida, "results");
    strcpy(configuracao.nivelLog, "summary");
    configuracao.intervaloLog = 10;
    configuracao.salvarIndividuos = 0;

    return configuracao;
}
