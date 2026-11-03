#include "aplicacao.h"

#include "aleatorio.h"
#include "evolucao.h"
#include "exportacao.h"
#include "problema.h"
#include "registro.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int lerInteiro(const char *texto, int *valor) {
    char *fim;
    long numero;

    errno = 0;
    numero = strtol(texto, &fim, 10);
    if (errno != 0 || *texto == '\0' || *fim != '\0' || numero < INT_MIN || numero > INT_MAX) {
        return 0;
    }
    *valor = (int)numero;
    return 1;
}

static int lerSemente(const char *texto, uint32_t *valor) {
    char *fim;
    unsigned long numero;

    errno = 0;
    numero = strtoul(texto, &fim, 10);
    if (errno != 0 || *texto == '\0' || *fim != '\0' || numero > UINT32_MAX) {
        return 0;
    }
    *valor = (uint32_t)numero;
    return 1;
}

static int copiarTexto(char *destino, size_t capacidade, const char *origem,
                       char *erro, size_t tamanhoErro) {
    if (strlen(origem) >= capacidade) {
        snprintf(erro, tamanhoErro, "Valor muito longo: %s", origem);
        return 0;
    }
    snprintf(destino, capacidade, "%s", origem);
    return 1;
}

int lerArgumentos(int argc, char **argv, Configuracao *configuracao,
                  char *erro, size_t tamanhoErro) {
    int posicional = 0;
    int i;

    *configuracao = criarConfiguracaoPadrao();
    if (erro != NULL && tamanhoErro > 0) erro[0] = '\0';

    for (i = 1; i < argc; i++) {
        const char *argumento = argv[i];

        if (strcmp(argumento, "--help") == 0 || strcmp(argumento, "-h") == 0) {
            return 2;
        }
        if (strcmp(argumento, "--individuals") == 0) {
            configuracao->salvarIndividuos = 1;
            continue;
        }
        if (strcmp(argumento, "--log") == 0 || strcmp(argumento, "--every") == 0 ||
            strcmp(argumento, "--seed") == 0 || strcmp(argumento, "--output") == 0) {
            if (++i >= argc) {
                snprintf(erro, tamanhoErro, "Falta valor para %s", argumento);
                return 0;
            }
            if (strcmp(argumento, "--log") == 0) {
                if (!copiarTexto(configuracao->nivelLog, sizeof(configuracao->nivelLog),
                                 argv[i], erro, tamanhoErro)) return 0;
            } else if (strcmp(argumento, "--every") == 0) {
                if (!lerInteiro(argv[i], &configuracao->intervaloLog)) {
                    snprintf(erro, tamanhoErro, "Intervalo invalido: %s", argv[i]);
                    return 0;
                }
            } else if (strcmp(argumento, "--seed") == 0) {
                if (!lerSemente(argv[i], &configuracao->semente)) {
                    snprintf(erro, tamanhoErro, "Semente invalida: %s", argv[i]);
                    return 0;
                }
            } else if (!copiarTexto(configuracao->saida, sizeof(configuracao->saida),
                                    argv[i], erro, tamanhoErro)) {
                return 0;
            }
            continue;
        }
        if (strncmp(argumento, "--", 2) == 0) {
            snprintf(erro, tamanhoErro, "Opcao desconhecida: %s", argumento);
            return 0;
        }

        if (posicional == 0) {
            if (!copiarTexto(configuracao->cenario, sizeof(configuracao->cenario),
                             argumento, erro, tamanhoErro)) return 0;
        } else if (posicional == 1) {
            if (!lerInteiro(argumento, &configuracao->populacao)) {
                snprintf(erro, tamanhoErro, "Populacao invalida: %s", argumento);
                return 0;
            }
        } else if (posicional == 2) {
            if (!lerInteiro(argumento, &configuracao->geracoes)) {
                snprintf(erro, tamanhoErro, "Geracoes invalidas: %s", argumento);
                return 0;
            }
        } else {
            snprintf(erro, tamanhoErro, "Argumentos posicionais em excesso");
            return 0;
        }
        posicional++;
    }

    if (strcmp(configuracao->cenario, "A") != 0 &&
        strcmp(configuracao->cenario, "B") != 0 &&
        strcmp(configuracao->cenario, "both") != 0) {
        snprintf(erro, tamanhoErro, "Cenario deve ser A, B ou both");
        return 0;
    }
    if (configuracao->populacao < 4 || configuracao->geracoes < 1 ||
        configuracao->intervaloLog < 1) {
        snprintf(erro, tamanhoErro, "Populacao >=4; geracoes e intervalo >=1");
        return 0;
    }
    if (strcmp(configuracao->nivelLog, "quiet") != 0 &&
        strcmp(configuracao->nivelLog, "summary") != 0 &&
        strcmp(configuracao->nivelLog, "steps") != 0 &&
        strcmp(configuracao->nivelLog, "full") != 0) {
        snprintf(erro, tamanhoErro, "Nivel de log invalido");
        return 0;
    }
    return 1;
}

void mostrarAjuda(void) {
    printf("Uso: ica_nsga2_c [A|B|both] [populacao] [geracoes] [opcoes]\n");
    printf("Opcoes:\n");
    printf("  --log quiet|summary|steps|full   Nivel de impressao (padrao summary)\n");
    printf("  --every N                        Intervalo de geracoes exibidas (padrao 10)\n");
    printf("  --seed N                         Semente aleatoria (padrao 20261102)\n");
    printf("  --output DIRETORIO               Pasta para CSVs (padrao results)\n");
    printf("  --individuals                    Exporta cromossomos/objetivos por geracao\n");
    printf("  --help                           Mostra esta ajuda\n");
}

int executarAplicacao(int argc, char **argv) {
    Configuracao configuracao;
    GeradorAleatorio gerador;
    char erro[256] = "";
    int leitura = lerArgumentos(argc, argv, &configuracao, erro, sizeof(erro));
    const char cenarios[] = {'A', 'B'};
    int c;

    if (leitura == 2) {
        mostrarAjuda();
        return 0;
    }
    if (!leitura) {
        fprintf(stderr, "ERRO: %s\n", erro);
        mostrarAjuda();
        return 1;
    }

    iniciarGerador(&gerador, configuracao.semente);
    for (c = 0; c < 2; c++) {
        char cenario = cenarios[c];
        Problema problema;
        Populacao populacao = {0};
        Registro registro;
        char pasta[MAX_CAMINHO];
        int quantidadePareto;

        if (strcmp(configuracao.cenario, "both") != 0 &&
            configuracao.cenario[0] != cenario) {
            continue;
        }
        problema = criarProblema(cenario);
        if (strlen(configuracao.saida) + strlen("/case_A") + 1 > sizeof(pasta)) {
            fprintf(stderr, "ERRO: caminho de saida muito longo\n");
            return 1;
        }
        strcpy(pasta, configuracao.saida);
        strcat(pasta, cenario == 'A' ? "/case_A" : "/case_B");

        if (!abrirRegistro(&registro, &configuracao, pasta, erro, sizeof(erro))) {
            fprintf(stderr, "ERRO: %s\n", erro);
            return 1;
        }
        registrarEntradaCenario(&registro, &problema);
        if (!executarNSGA2ComRegistro(&problema, &configuracao, &gerador,
                                     &populacao, &registro)) {
            fecharRegistro(&registro);
            fprintf(stderr, "ERRO: falha durante a evolucao\n");
            return 1;
        }
        fecharRegistro(&registro);
        if (!salvarResultados(&problema, &populacao, pasta, &quantidadePareto,
                              erro, sizeof(erro))) {
            liberarPopulacao(&populacao);
            fprintf(stderr, "ERRO: %s\n", erro);
            return 1;
        }
        liberarPopulacao(&populacao);
    }
    return 0;
}
