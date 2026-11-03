#include "aleatorio.h"
#include "aplicacao.h"
#include "avaliacao.h"
#include "dominancia.h"
#include "evolucao.h"
#include "exportacao.h"
#include "operadores.h"
#include "problema.h"
#include "registro.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int falhas = 0;

#define VERIFICAR(condicao, mensagem) \
    do { \
        if (!(condicao)) { \
            printf("FALHA: %s (linha %d)\n", mensagem, __LINE__); \
            falhas++; \
        } \
    } while (0)

static void testarProblemas(void) {
    char erro[160] = "";
    Problema cenarioA = criarProblema('A');
    Problema cenarioB = criarProblema('B');
    Problema invalido = criarProblema('X');

    VERIFICAR(cenarioA.quantidadeResiduos == 7, "cenario A deve possuir sete residuos");
    VERIFICAR(cenarioA.quantidadeInstalacoes == 6, "cenario A deve possuir seis instalacoes");
    VERIFICAR(strcmp(cenarioA.nome, "A_SSL_Thamirabarani_ILUSTRATIVO") == 0,
              "nome do cenario A deve ser preservado");
    VERIFICAR(cenarioB.residuos[5].quantidadeKg == 96.0,
              "inventario do cenario B deve ser preservado");
    VERIFICAR(validarProblema(&cenarioA, erro, sizeof(erro)) == 1,
              "cenario A deve ser valido");
    VERIFICAR(validarProblema(&cenarioB, erro, sizeof(erro)) == 1,
              "cenario B deve ser valido");
    VERIFICAR(validarProblema(&invalido, erro, sizeof(erro)) == 0,
              "cenario desconhecido deve ser rejeitado");
}

static void testarAleatorio(void) {
    const uint32_t esperados[5] = {
        3249422673u, 1032677726u, 1519133066u, 2517309840u, 3019555980u
    };
    const int inteirosEsperados[5] = {4, 1, 2, 3, 4};
    const double reaisEsperados[3] = {
        0.24043901980774599, 0.58610687040577192, 0.44986221443466962
    };
    GeradorAleatorio gerador;
    int i;

    iniciarGerador(&gerador, 20261102u);
    for (i = 0; i < 5; i++) {
        VERIFICAR(proximoU32(&gerador) == esperados[i],
                  "MT19937 deve coincidir com std::mt19937");
    }

    iniciarGerador(&gerador, 20261102u);
    for (i = 0; i < 5; i++) {
        VERIFICAR(sortearInteiro(&gerador, 0, 5) == inteirosEsperados[i],
                  "distribuicao inteira deve coincidir com libstdc++");
    }

    iniciarGerador(&gerador, 20261102u);
    for (i = 0; i < 3; i++) {
        VERIFICAR(fabs(sortearUnidade(&gerador) - reaisEsperados[i]) < 1e-15,
                  "distribuicao real deve coincidir com libstdc++");
    }
}

static void testarNucleoNSGA2(void) {
    Problema problema = criarProblema('A');
    GeradorAleatorio gerador;
    Individuo primeiro;
    Individuo pior;
    Individuo filho;
    Populacao populacao;
    Configuracao configuracao = criarConfiguracaoPadrao();
    int vistos[MAX_RESIDUOS] = {0};
    int i;

    iniciarGerador(&gerador, configuracao.semente);
    primeiro = criarIndividuoAleatorio(&problema, &gerador);

    for (i = 0; i < problema.quantidadeResiduos; i++) {
        int destino = primeiro.destino[i];
        int prioridade = primeiro.prioridade[i];

        VERIFICAR(destino >= 0 && destino < problema.quantidadeInstalacoes,
                  "destino deve apontar para uma instalacao existente");
        VERIFICAR(instalacaoAceita(&problema.instalacoes[destino], problema.residuos[i].tipo),
                  "destino sorteado deve aceitar o residuo");
        VERIFICAR(prioridade >= 0 && prioridade < problema.quantidadeResiduos,
                  "prioridade deve estar no intervalo do cromossomo");
        vistos[prioridade]++;
    }
    for (i = 0; i < problema.quantidadeResiduos; i++) {
        VERIFICAR(vistos[i] == 1, "prioridades devem formar uma permutacao");
    }
    VERIFICAR(primeiro.violacao >= 0.0 && primeiro.objetivos[0] > 0.0,
              "individuo deve sair avaliado");

    pior = primeiro;
    pior.objetivos[0] += 10.0;
    pior.objetivos[1] += 1.0;
    pior.objetivos[2] += 1.0;
    VERIFICAR(individuoDomina(&primeiro, &pior), "individuo melhor deve dominar o pior");
    VERIFICAR(!individuoDomina(&pior, &primeiro), "dominancia nao deve ser simetrica");

    VERIFICAR(alocarPopulacao(&populacao, 2), "populacao pequena deve ser alocada");
    populacao.individuos[0] = primeiro;
    populacao.individuos[1] = pior;
    populacao.tamanho = 2;
    classificarPopulacao(&populacao);
    VERIFICAR(populacao.individuos[0].rank == 0 && populacao.individuos[1].rank == 1,
              "ordenacao deve separar duas frentes");
    VERIFICAR(calcularCrowding(&populacao, 0),
              "crowding de frente classificada deve ser calculado");
    VERIFICAR(isinf(populacao.individuos[0].crowding),
              "extremo de uma frente deve ter crowding infinito");
    liberarPopulacao(&populacao);

    iniciarGerador(&gerador, configuracao.semente);
    filho = cruzarIndividuos(&primeiro, &pior, &gerador, problema.quantidadeResiduos);
    mutarIndividuo(&filho, &problema, &gerador);
    avaliarIndividuo(&filho, &problema);
    VERIFICAR(filho.objetivos[0] > 0.0, "filho cruzado e mutado deve ser avaliado");
    VERIFICAR(filho.violacao < 1e-8,
              "cruzamento e mutacao devem manter factibilidade no cenario A");
    {
        int prioridades[MAX_RESIDUOS] = {0};
        for (i = 0; i < problema.quantidadeResiduos; i++) {
            int destino = filho.destino[i];
            int prioridade = filho.prioridade[i];
            VERIFICAR(destino >= 0 && destino < problema.quantidadeInstalacoes &&
                      instalacaoAceita(&problema.instalacoes[destino],
                                       problema.residuos[i].tipo),
                      "genes de destino devem preservar elegibilidade");
            VERIFICAR(prioridade >= 0 && prioridade < problema.quantidadeResiduos,
                      "genes de prioridade devem permanecer no intervalo");
            if (prioridade >= 0 && prioridade < problema.quantidadeResiduos) {
                prioridades[prioridade]++;
            }
        }
        for (i = 0; i < problema.quantidadeResiduos; i++) {
            VERIFICAR(prioridades[i] == 1,
                      "cruzamento e mutacao devem preservar permutacao de prioridades");
        }
    }

    memset(&populacao, 0, sizeof(populacao));
    VERIFICAR(!alocarPopulacao(&populacao, -1), "tamanho negativo deve ser rejeitado");

    configuracao.populacao = 8;
    configuracao.geracoes = 3;
    iniciarGerador(&gerador, configuracao.semente);
    VERIFICAR(executarNSGA2(&problema, &configuracao, &gerador, &populacao),
              "execucao pequena do NSGA-II deve concluir");
    VERIFICAR(populacao.tamanho == 8, "populacao final deve preservar o tamanho configurado");
    for (i = 0; i < populacao.tamanho; i++) {
        VERIFICAR(populacao.individuos[i].rank >= 0,
                  "todos os individuos devem terminar classificados");
    }
    liberarPopulacao(&populacao);
}

static int arquivoComecaCom(const char *caminho, const char *cabecalho) {
    FILE *arquivo = fopen(caminho, "r");
    char linha[512];
    int resultado = 0;

    if (arquivo == NULL) {
        return 0;
    }
    if (fgets(linha, sizeof(linha), arquivo) != NULL) {
        resultado = strncmp(linha, cabecalho, strlen(cabecalho)) == 0;
    }
    fclose(arquivo);
    return resultado;
}

static void montarCaminhoTeste(char *destino, size_t capacidade,
                              const char *pasta, const char *arquivo) {
    if (strlen(pasta) + strlen(arquivo) + 2 > capacidade) {
        destino[0] = '\0';
        falhas++;
        return;
    }
    strcpy(destino, pasta);
    strcat(destino, "/");
    strcat(destino, arquivo);
}

static void testarAplicacaoEArquivos(void) {
    char erro[256] = "";
    char *argumentosValidos[] = {
        "ica_nsga2_c", "A", "8", "3", "--log", "full", "--every", "1",
        "--seed", "42", "--output", "saida_teste", "--individuals"
    };
    char *argumentosInvalidos[] = {"ica_nsga2_c", "X"};
    Configuracao configuracao;
    Problema problema = criarProblema('A');
    GeradorAleatorio gerador;
    Populacao populacao = {0};
    Registro registro;
    const char *temporario = getenv("TEMP");
    char pasta[MAX_CAMINHO];
    char caminho[MAX_CAMINHO];
    int quantidadePareto = 0;

    VERIFICAR(lerArgumentos(13, argumentosValidos, &configuracao, erro, sizeof(erro)) == 1,
              "argumentos validos devem ser aceitos");
    VERIFICAR(strcmp(configuracao.cenario, "A") == 0 && configuracao.populacao == 8 &&
              configuracao.geracoes == 3 && configuracao.semente == 42u &&
              configuracao.salvarIndividuos,
              "todos os argumentos devem ser armazenados");
    VERIFICAR(lerArgumentos(2, argumentosInvalidos, &configuracao, erro, sizeof(erro)) == 0,
              "cenario invalido deve ser rejeitado");

    snprintf(pasta, sizeof(pasta), "%s/ica_nsga2-c-testes/saida/aninhada",
             temporario != NULL ? temporario : ".");
    configuracao = criarConfiguracaoPadrao();
    configuracao.populacao = 8;
    configuracao.geracoes = 3;
    configuracao.salvarIndividuos = 1;
    strcpy(configuracao.nivelLog, "quiet");
    configuracao.intervaloLog = 1;

    VERIFICAR(abrirRegistro(&registro, &configuracao, pasta, erro, sizeof(erro)) == 1,
              "registro deve criar subdiretorios");
    iniciarGerador(&gerador, configuracao.semente);
    VERIFICAR(executarNSGA2ComRegistro(&problema, &configuracao, &gerador, &populacao, &registro),
              "NSGA-II com registro deve concluir");
    fecharRegistro(&registro);
    VERIFICAR(salvarResultados(&problema, &populacao, pasta, &quantidadePareto,
                              erro, sizeof(erro)) == 1,
              "resultados devem ser exportados");
    VERIFICAR(quantidadePareto > 0, "frente de Pareto deve possuir solucoes");
    liberarPopulacao(&populacao);

    montarCaminhoTeste(caminho, sizeof(caminho), pasta, "steps.csv");
    VERIFICAR(arquivoComecaCom(caminho, "generation,stage,input,output"),
              "steps.csv deve possuir cabecalho");
    montarCaminhoTeste(caminho, sizeof(caminho), pasta, "generations.csv");
    VERIFICAR(arquivoComecaCom(caminho, "generation,feasible"),
              "generations.csv deve possuir cabecalho");
    montarCaminhoTeste(caminho, sizeof(caminho), pasta, "individuals.csv");
    VERIFICAR(arquivoComecaCom(caminho, "generation,stage,index"),
              "individuals.csv deve ser criado quando solicitado");
    montarCaminhoTeste(caminho, sizeof(caminho), pasta, "pareto.csv");
    VERIFICAR(arquivoComecaCom(caminho, "solution,cost_simulated"),
              "pareto.csv deve possuir cabecalho");
    montarCaminhoTeste(caminho, sizeof(caminho), pasta, "inventory.csv");
    VERIFICAR(arquivoComecaCom(caminho, "lot,type,quantity_kg"),
              "inventory.csv deve possuir cabecalho");
    montarCaminhoTeste(caminho, sizeof(caminho), pasta, "facilities_SIMULATED.csv");
    VERIFICAR(arquivoComecaCom(caminho, "facility,capacity_kg"),
              "facilities_SIMULATED.csv deve possuir cabecalho");
}

int main(void) {
    testarProblemas();
    testarAleatorio();
    testarNucleoNSGA2();
    testarAplicacaoEArquivos();

    if (falhas != 0) {
        printf("TOTAL DE FALHAS: %d\n", falhas);
        return 1;
    }

    printf("PASS: modelo, problema, aleatorio e nucleo NSGA-II\n");
    return 0;
}
