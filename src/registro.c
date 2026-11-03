#include "registro.h"

#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define CRIAR_DIRETORIO(caminho) _mkdir(caminho)
#else
#include <sys/stat.h>
#define CRIAR_DIRETORIO(caminho) mkdir(caminho, 0777)
#endif

static int criarUmDiretorio(const char *caminho) {
    if (CRIAR_DIRETORIO(caminho) == 0 || errno == EEXIST) {
        return 1;
    }
    return 0;
}

int criarDiretorios(const char *caminho, char *erro, size_t tamanhoErro) {
    char copia[MAX_CAMINHO];
    size_t i;
    size_t tamanho;

    if (caminho == NULL || caminho[0] == '\0' || strlen(caminho) >= sizeof(copia)) {
        snprintf(erro, tamanhoErro, "Caminho de saida invalido");
        return 0;
    }
    snprintf(copia, sizeof(copia), "%s", caminho);
    tamanho = strlen(copia);

    for (i = 1; i < tamanho; i++) {
        if (copia[i] == '/' || copia[i] == '\\') {
            char separador = copia[i];
            copia[i] = '\0';
            if (!(i == 2 && copia[1] == ':') && copia[0] != '\0' && !criarUmDiretorio(copia)) {
                snprintf(erro, tamanhoErro, "Nao foi possivel criar diretorio: %s", copia);
                return 0;
            }
            copia[i] = separador;
        }
    }
    if (!criarUmDiretorio(copia)) {
        snprintf(erro, tamanhoErro, "Nao foi possivel criar diretorio: %s", copia);
        return 0;
    }
    return 1;
}

static FILE *abrirCSV(const char *pasta, const char *nome) {
    char caminho[MAX_CAMINHO];

    snprintf(caminho, sizeof(caminho), "%s/%s", pasta, nome);
    return fopen(caminho, "w");
}

int abrirRegistro(Registro *registro, const Configuracao *configuracao,
                  const char *pasta, char *erro, size_t tamanhoErro) {
    memset(registro, 0, sizeof(*registro));
    registro->configuracao = *configuracao;

    if (!criarDiretorios(pasta, erro, tamanhoErro)) {
        return 0;
    }
    registro->etapas = abrirCSV(pasta, "steps.csv");
    registro->geracoes = abrirCSV(pasta, "generations.csv");
    if (configuracao->salvarIndividuos) {
        registro->individuos = abrirCSV(pasta, "individuals.csv");
    }
    if (registro->etapas == NULL || registro->geracoes == NULL ||
        (configuracao->salvarIndividuos && registro->individuos == NULL)) {
        snprintf(erro, tamanhoErro, "Nao foi possivel abrir os arquivos de registro");
        fecharRegistro(registro);
        return 0;
    }

    fprintf(registro->etapas, "generation,stage,input,output\n");
    fprintf(registro->geracoes,
            "generation,feasible,rank_one_feasible,best_cost,best_transport_risk,best_facility_risk\n");
    if (registro->individuos != NULL) {
        fprintf(registro->individuos,
                "generation,stage,index,facility_genes,priority_genes,cost,risk_transport,risk_facility,violation,trips,rank,crowding\n");
    }
    return 1;
}

void fecharRegistro(Registro *registro) {
    if (registro->etapas != NULL) {
        fclose(registro->etapas);
    }
    if (registro->geracoes != NULL) {
        fclose(registro->geracoes);
    }
    if (registro->individuos != NULL) {
        fclose(registro->individuos);
    }
    registro->etapas = NULL;
    registro->geracoes = NULL;
    registro->individuos = NULL;
}

int registroDetalhado(const Registro *registro) {
    return strcmp(registro->configuracao.nivelLog, "steps") == 0 ||
           strcmp(registro->configuracao.nivelLog, "full") == 0;
}

int registroCompleto(const Registro *registro) {
    return strcmp(registro->configuracao.nivelLog, "full") == 0;
}

static int mostrarGeracao(const Registro *registro, int geracao) {
    return geracao == 0 ||
           geracao % registro->configuracao.intervaloLog == 0 ||
           geracao == registro->configuracao.geracoes;
}

static void escreverTextoCSV(FILE *arquivo, const char *texto) {
    const char *p;

    fputc('"', arquivo);
    for (p = texto; *p != '\0'; p++) {
        if (*p == '"') {
            fputs("\"\"", arquivo);
        } else if (*p == '\n') {
            fputc(' ', arquivo);
        } else {
            fputc(*p, arquivo);
        }
    }
    fputc('"', arquivo);
}

void registrarEntradaCenario(Registro *registro, const Problema *problema) {
    int i;

    printf("\n========== ENTRADAS: %s ==========\n", problema->nome);
    printf("Lotes: %d | Instalacoes: %d | Capacidade veiculo: %g kg\n",
           problema->quantidadeResiduos, problema->quantidadeInstalacoes,
           problema->capacidadeVeiculo);
    printf("Populacao: %d | Geracoes: %d | Seed: %u\n",
           registro->configuracao.populacao, registro->configuracao.geracoes,
           registro->configuracao.semente);
    printf("Objetivos: [custo, risco transporte, risco instalacoes]\n");
    printf("IMPORTANTE: dados de logistica e incompatibilidade SIMULADOS.\n");

    if (registroDetalhado(registro)) {
        printf("[ENTRADAS / LOTES]\n");
        for (i = 0; i < problema->quantidadeResiduos; i++) {
            printf("  L%d %s tipo=%d massa=%g kg\n", i, problema->residuos[i].nome,
                   problema->residuos[i].tipo, problema->residuos[i].quantidadeKg);
        }
        printf("[ENTRADAS / DESTINADORES]\n");
        for (i = 0; i < problema->quantidadeInstalacoes; i++) {
            printf("  F%d %s capacidade=%g kg\n", i, problema->instalacoes[i].nome,
                   problema->instalacoes[i].capacidade);
        }
    }
}

void registrarInicioGeracao(Registro *registro, int geracao, const Populacao *populacao) {
    if (registroDetalhado(registro) && mostrarGeracao(registro, geracao)) {
        printf("\n===== GERACAO %d | entrada=%d individuos =====\n",
               geracao, populacao->tamanho);
    }
}

void registrarEtapa(Registro *registro, int geracao, const char *nome,
                    const char *entrada, const char *saida) {
    fprintf(registro->etapas, "%d,", geracao);
    escreverTextoCSV(registro->etapas, nome);
    fputc(',', registro->etapas);
    escreverTextoCSV(registro->etapas, entrada);
    fputc(',', registro->etapas);
    escreverTextoCSV(registro->etapas, saida);
    fputc('\n', registro->etapas);

    if (registroDetalhado(registro) && mostrarGeracao(registro, geracao)) {
        printf("[ETAPA] %s\n  entrada: %s\n  saida:   %s\n", nome, entrada, saida);
    }
}

static void escreverGenes(FILE *arquivo, const int *genes, int quantidade) {
    int i;

    fputc('"', arquivo);
    for (i = 0; i < quantidade; i++) {
        if (i > 0) {
            fputc(';', arquivo);
        }
        fprintf(arquivo, "%d", genes[i]);
    }
    fputc('"', arquivo);
}

void registrarIndividuo(Registro *registro, int geracao, const char *etapa,
                        int indice, const Individuo *individuo) {
    if (registro->individuos == NULL) {
        return;
    }
    fprintf(registro->individuos, "%d,", geracao);
    escreverTextoCSV(registro->individuos, etapa);
    fprintf(registro->individuos, ",%d,", indice);
    escreverGenes(registro->individuos, individuo->destino, MAX_RESIDUOS);
    fputc(',', registro->individuos);
    escreverGenes(registro->individuos, individuo->prioridade, MAX_RESIDUOS);
    fprintf(registro->individuos, ",%.12g,%.12g,%.12g,%.12g,%d,%d,%.12g\n",
            individuo->objetivos[0], individuo->objetivos[1], individuo->objetivos[2],
            individuo->violacao, individuo->viagens, individuo->rank, individuo->crowding);

    if (registroCompleto(registro) && mostrarGeracao(registro, geracao)) {
        printf("    individuo %d f1=%.3f f2=%.3f f3=%.3f viol=%.3f viagens=%d rank=%d crowd=%.3f\n",
               indice, individuo->objetivos[0], individuo->objetivos[1], individuo->objetivos[2],
               individuo->violacao, individuo->viagens, individuo->rank, individuo->crowding);
    }
}

EstatisticaGeracao resumirPopulacao(int geracao, const Populacao *populacao) {
    EstatisticaGeracao estatistica;
    int i;

    memset(&estatistica, 0, sizeof(estatistica));
    estatistica.geracao = geracao;
    estatistica.menorCusto = DBL_MAX;
    estatistica.menorRiscoTransporte = DBL_MAX;
    estatistica.menorRiscoInstalacao = DBL_MAX;

    for (i = 0; i < populacao->tamanho; i++) {
        const Individuo *individuo = &populacao->individuos[i];
        if (individuo->violacao < 1e-8) {
            estatistica.factiveis++;
            if (individuo->rank == 0) {
                estatistica.factiveisRankUm++;
            }
            if (individuo->objetivos[0] < estatistica.menorCusto) {
                estatistica.menorCusto = individuo->objetivos[0];
            }
            if (individuo->objetivos[1] < estatistica.menorRiscoTransporte) {
                estatistica.menorRiscoTransporte = individuo->objetivos[1];
            }
            if (individuo->objetivos[2] < estatistica.menorRiscoInstalacao) {
                estatistica.menorRiscoInstalacao = individuo->objetivos[2];
            }
        }
    }
    return estatistica;
}

void registrarFimGeracao(Registro *registro, const EstatisticaGeracao *estatistica) {
    fprintf(registro->geracoes, "%d,%d,%d,%g,%g,%g\n",
            estatistica->geracao, estatistica->factiveis, estatistica->factiveisRankUm,
            estatistica->menorCusto, estatistica->menorRiscoTransporte,
            estatistica->menorRiscoInstalacao);

    if (strcmp(registro->configuracao.nivelLog, "quiet") != 0 &&
        mostrarGeracao(registro, estatistica->geracao)) {
        printf("[SAIDA GERACAO %d] factiveis=%d frente1=%d min_f1=%g min_f2=%g min_f3=%g\n",
               estatistica->geracao, estatistica->factiveis, estatistica->factiveisRankUm,
               estatistica->menorCusto, estatistica->menorRiscoTransporte,
               estatistica->menorRiscoInstalacao);
    }
}
