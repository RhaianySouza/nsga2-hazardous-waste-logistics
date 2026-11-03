#include "problema.h"

#include <stdio.h>
#include <string.h>

static Residuo criarResiduo(const char *nome, int tipo, double quantidadeKg) {
    Residuo residuo;

    memset(&residuo, 0, sizeof(residuo));
    snprintf(residuo.nome, sizeof(residuo.nome), "%s", nome);
    residuo.tipo = tipo;
    residuo.quantidadeKg = quantidadeKg;
    return residuo;
}

static Instalacao criarInstalacao(const char *nome, double capacidade,
                                  double custo, double riscoLocal,
                                  double distancia, double riscoRodovia,
                                  const int *tipos, int quantidadeTipos) {
    Instalacao instalacao;
    int i;

    memset(&instalacao, 0, sizeof(instalacao));
    snprintf(instalacao.nome, sizeof(instalacao.nome), "%s", nome);
    instalacao.capacidade = capacidade;
    instalacao.custoTratamentoKg = custo;
    instalacao.riscoLocalKg = riscoLocal;
    instalacao.distanciaKm = distancia;
    instalacao.riscoRodoviaKgKm = riscoRodovia;
    instalacao.quantidadeTiposAceitos = quantidadeTipos;

    for (i = 0; i < quantidadeTipos; i++) {
        instalacao.tiposAceitos[i] = tipos[i];
    }

    return instalacao;
}

int instalacaoAceita(const Instalacao *instalacao, int tipo) {
    int i;

    for (i = 0; i < instalacao->quantidadeTiposAceitos; i++) {
        if (instalacao->tiposAceitos[i] == tipo) {
            return 1;
        }
    }
    return 0;
}

Problema criarProblema(char cenario) {
    Problema problema;
    const int tipo0[] = {0};
    const int tipo1[] = {1};
    const int tipo2[] = {2};
    const int tipo3[] = {3};
    const int tipo4[] = {4};
    const int todosTipos[] = {0, 1, 2, 3, 4};
    const int pares[][2] = {{0, 2}, {0, 3}, {1, 2}, {1, 4}, {2, 3}, {2, 4}};
    int i;

    memset(&problema, 0, sizeof(problema));
    problema.capacidadeVeiculo = 150.0;
    problema.custoFixoViagem = 120.0;
    problema.custoKm = 4.2;

    if (cenario != 'A' && cenario != 'B') {
        return problema;
    }

    problema.valido = 1;
    problema.quantidadeResiduos = MAX_RESIDUOS;
    problema.quantidadeInstalacoes = MAX_INSTALACOES;

    snprintf(problema.nome, sizeof(problema.nome), "%s",
             cenario == 'A' ? "A_SSL_Thamirabarani_ILUSTRATIVO" : "B_Sample_Ship_Japao");

    problema.instalacoes[0] = criarInstalacao("F1_Tratamento_revestimentos", 450, 12, 0.045, 85, 0.0020, tipo0, 1);
    problema.instalacoes[1] = criarInstalacao("F2_Tratamento_amianto", 350, 9, 0.028, 115, 0.0016, tipo1, 1);
    problema.instalacoes[2] = criarInstalacao("F3_Recuperacao_refrigerantes", 200, 18, 0.025, 70, 0.0014, tipo2, 1);
    problema.instalacoes[3] = criarInstalacao("F4_Reciclagem_baterias", 400, 6, 0.032, 145, 0.0012, tipo3, 1);
    problema.instalacoes[4] = criarInstalacao("F5_Multiprocesso_simulado", 500, 16, 0.055, 55, 0.0028, todosTipos, 5);
    problema.instalacoes[5] = criarInstalacao("F6_Tratamento_oleos", 450, 5, 0.025, 100, 0.0019, tipo4, 1);

    for (i = 0; i < 6; i++) {
        int a = pares[i][0];
        int b = pares[i][1];
        problema.incompatibilidade[a][b] = 1;
        problema.incompatibilidade[b][a] = 1;
    }

    if (cenario == 'B') {
        problema.residuos[0] = criarResiduo("Tinta_anti_incrustante_TBT", 0, 60);
        problema.residuos[1] = criarResiduo("Amianto_junta_motor", 1, 3.5);
        problema.residuos[2] = criarResiduo("Amianto_caldeira", 1, 10);
        problema.residuos[3] = criarResiduo("Amianto_tubulacoes", 1, 50);
        problema.residuos[4] = criarResiduo("Refrigerante_HCFC_R22", 2, 20);
        problema.residuos[5] = criarResiduo("Chumbo_em_baterias", 3, 96);
        problema.residuos[6] = criarResiduo("Amianto_isolamento", 1, 3.8);
    } else {
        problema.residuos[0] = criarResiduo("A_revestimentos_hipotetico", 0, 85);
        problema.residuos[1] = criarResiduo("A_amianto_1_hipotetico", 1, 40);
        problema.residuos[2] = criarResiduo("A_amianto_2_hipotetico", 1, 55);
        problema.residuos[3] = criarResiduo("A_refrigerante_hipotetico", 2, 28);
        problema.residuos[4] = criarResiduo("A_baterias_hipotetico", 3, 110);
        problema.residuos[5] = criarResiduo("A_oleos_hipotetico", 4, 130);
        problema.residuos[6] = criarResiduo("A_revestimentos_2_hipotetico", 0, 35);
    }

    return problema;
}

int validarProblema(const Problema *problema, char *erro, size_t tamanhoErro) {
    int i;
    int j;

    if (erro != NULL && tamanhoErro > 0) {
        erro[0] = '\0';
    }
    if (problema == NULL || !problema->valido) {
        if (erro != NULL) {
            snprintf(erro, tamanhoErro, "Cenario deve ser A ou B");
        }
        return 0;
    }
    if (problema->quantidadeResiduos <= 0 || problema->quantidadeInstalacoes <= 0) {
        if (erro != NULL) {
            snprintf(erro, tamanhoErro, "Problema vazio");
        }
        return 0;
    }
    if (problema->capacidadeVeiculo <= 0) {
        if (erro != NULL) {
            snprintf(erro, tamanhoErro, "Capacidade de veiculo invalida");
        }
        return 0;
    }

    for (i = 0; i < problema->quantidadeResiduos; i++) {
        int elegivel = 0;
        const Residuo *residuo = &problema->residuos[i];

        if (residuo->quantidadeKg <= 0 || residuo->tipo < 0 || residuo->tipo >= MAX_TIPOS) {
            if (erro != NULL) {
                snprintf(erro, tamanhoErro, "Lote invalido: %s", residuo->nome);
            }
            return 0;
        }
        for (j = 0; j < problema->quantidadeInstalacoes; j++) {
            if (instalacaoAceita(&problema->instalacoes[j], residuo->tipo)) {
                elegivel = 1;
            }
        }
        if (!elegivel) {
            if (erro != NULL) {
                snprintf(erro, tamanhoErro, "Sem destino elegivel para %s", residuo->nome);
            }
            return 0;
        }
    }
    return 1;
}
