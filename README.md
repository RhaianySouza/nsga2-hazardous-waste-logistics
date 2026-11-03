# NSGA-II para Otimização da Logística de Resíduos Perigosos

Implementação modular em **C11 (ISO/IEC 9899:2011)** do algoritmo **NSGA-II (Non-dominated Sorting Genetic Algorithm II)**. O projeto investiga, em contexto acadêmico e experimental, a atribuição de lotes de resíduos perigosos a instalações receptoras simuladas e seu agrupamento em viagens.

O protótipo busca compromissos entre três objetivos minimizados:

- **f₁ — Custo simulado:** destinação por massa e custos associados às viagens;
- **f₂ — Risco de transporte simulado:** distância, massa transportada e coeficiente viário de risco;
- **f₃ — Risco local simulado:** massa destinada ponderada pelo coeficiente de risco da instalação.

> **Escopo e uso:** este é um protótipo acadêmico para a disciplina de Inteligência Computacional Aplicada (ICA), do PPG-MCCT/UFF. Parâmetros de instalações, logística, custos, distâncias, riscos e incompatibilidades são simulados. Os índices de risco não foram calibrados com dados operacionais. O programa não é um planejador logístico operacional, não otimiza rotas reais em redes viárias e não comprova otimalidade global.

## Relação com Rabbani et al. (2018)

O projeto é uma **adaptação experimental simplificada inspirada** no problema de localização e roteirização de resíduos perigosos de Rabbani et al. Não reproduz integralmente o modelo matemático, a rede, as instâncias ou os resultados do artigo.

O artigo original inclui decisões de localização de instalações e tecnologias, vários nós de geração, tratamento, reciclagem e disposição, frota heterogênea e riscos baseados na exposição da população. Esta implementação representa sete lotes fixos, uma origem abstrata, seis destinadores fixos e viagens de ida e volta, cada uma com um único destinador. Não inclui abertura de instalações, múltiplos pontos de coleta, múltiplas paradas ou fluxos secundários entre instalações.

**A ordem dos objetivos f₂ e f₃ difere da notação do artigo.** Em Rabbani et al., `f₂` representa risco das instalações e `f₃` risco de transporte. Neste programa, `f₂` é risco de transporte e `f₃` risco das instalações. A adaptação usa coeficientes simulados sem calibração populacional ou operacional; portanto, seus valores não são comparáveis aos resultados numéricos do artigo.

## Formulação usada no programa

Para cada lote, o cromossomo armazena a instalação de destino e uma prioridade usada pelo empacotamento construtivo das viagens. A avaliação minimiza:

1. `f₁`: massa × custo de tratamento por kg, somado ao custo fixo e ao custo por km de cada viagem de ida e volta;
2. `f₂`: distância de ida e volta × coeficiente viário de risco × massa transportada em cada viagem;
3. `f₃`: massa × coeficiente de risco local da instalação atribuída.

As unidades são dimensionalmente consistentes sob as interpretações de custo/kg, custo/km, risco/(kg·km) e risco/kg. Os coeficientes são parâmetros da simulação; o programa não define uma moeda, uma unidade de risco validada nem exposição populacional. Chai et al. (2023) é uma referência complementar sobre risco de condução e roteamento, mas não calibra os coeficientes deste projeto.

## Cenários experimentais

| Característica | Caso A | Caso B |
|---|---|---|
| Identificador no programa | `A_SSL_Thamirabarani` | `B_Sample_Ship_Japao` |
| Contexto | MV SSL Thamirabarani; instância hipotética | *Sample Ship*; quantidades aproximadas de inventário-modelo |
| Lotes | 7 | 7 |
| Destinadores | 6, simulados | 6, simulados |
| Capacidade do veículo | 150 kg | 150 kg |

Os sete lotes do Caso A são hipotéticos: 85, 40, 55, 28, 110, 130 e 35 kg. O Caso B usa quantidades aproximadas de 60, 3,5, 10, 50, 20, 96 e 3,8 kg; não são quantidades comprovadamente removidas como resíduos. A procedência primária desses valores não foi confirmada neste projeto.

As capacidades, elegibilidades, custos, distâncias e riscos das seis instalações são simulados. A matriz de incompatibilidade é ilustrativa e não foi validada quimicamente por FDS/FISPQ ou especialista. O IHM pode informar materiais e sua localização na embarcação, mas não determina, por si só, os custos, as licenças, as distâncias ou os riscos da rede de destinação simulada.

## Como funciona o NSGA-II

A implementação usa população inicial aleatória, torneio binário, cruzamento, mutação, ordenação por não dominância, crowding distance e seleção ambiental elitista entre pais e descendentes. O ciclo geral segue NSGA-II de Deb et al. (2002); a representação, os operadores, o tratamento de restrições e as funções objetivo são específicos deste protótipo.

O registro do programa apresenta dez etapas didáticas. A ordem da tabela não significa que todas as operações sejam executadas linearmente uma única vez: avaliação, classificação e crowding são recalculados nos pontos necessários do ciclo.

| Etapa | Operação | Entrada → saída principal |
|---|---|---|
| 01 | População inicial | Problema e semente → `P₀` |
| 02 | Codificação dos cromossomos | Lotes e destinos elegíveis → genes de destino e prioridade |
| 03 | Avaliação e classificação | Cromossomos → objetivos, violações, rank, crowding e viagens |
| 04 | Seleção por torneio | Pais classificados → pares selecionados |
| 05 | Cruzamento | Cromossomos parentais → descendentes |
| 06 | Mutação | Genes descendentes → novas combinações |
| 07 | Ordenação por não dominância | Pais e descendentes → frentes |
| 08 | Crowding distance | Frentes e objetivos → estimativa de diversidade |
| 09 | Seleção ambiental elitista | Pais + descendentes → população de tamanho `N` |
| 10 | Critério de parada | Geração atual e limite → continuar ou encerrar |

Uma solução factível tem precedência sobre uma inviável; soluções inviáveis são comparadas pela penalidade agregada. Nas factíveis, a comparação usa dominância de Pareto nos três objetivos. Os destinos são escolhidos entre instalações elegíveis. O avaliador verifica capacidades e separa tipos incompatíveis em viagens distintas; os operadores não reparam toda violação de capacidade para instâncias futuras.

Parâmetros padrão: população `100`, gerações `200`, semente `20261102`, capacidade do veículo `150 kg`, custo fixo simulado por viagem `120`, custo simulado por km `4.2`, mutação de destino `0.14` por gene e mutação de prioridade `0.4` por indivíduo. Cada gene de destino é selecionado no cruzamento com probabilidade `0.5`. Não há evidência de calibração por Taguchi ou de varredura de parâmetros nesta implementação. A sequência pseudoaleatória fixa é reproduzível neste build; outras bibliotecas/compiladores podem produzir sequências diferentes.

## Estrutura do repositório

```text
.
├── include/                   # Interfaces e tipos C11 (.h)
├── src/                       # Implementações C11 (.c)
├── tests/
│   ├── testes.c               # Testes da implementação
│   ├── validar_frente.ps1     # Validação de factibilidade e frente final
│   └── verificar_paridade.ps1 # Comparação de regressão opcional
├── docs/
│   └── AUDITORIA_TECNICA.md   # Auditoria científica e computacional
├── .vscode/                   # Tarefas e depuração do projeto
└── compilar.ps1               # Compilação no PowerShell
```

## Compilação e testes

Requer GCC e G++ disponíveis no `PATH`. O script compila cada fonte `.c` como C11 e usa G++ somente para ligar os objetos neste ambiente Windows; o executável não contém fontes C++.

Compilar:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\compilar.ps1
```

Compilar com otimização `-O2`:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\compilar.ps1 -Otimizado
```

Compilar e executar os testes em C:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\compilar.ps1 -Testar
```

Validar descendentes, factibilidade e não dominância na população final:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tests\validar_frente.ps1
```

O executável é criado em `%TEMP%\ica_nsga2-c-build\programa_nsga2.exe`. No PowerShell, `\` não funciona como continuação de linha; use o script acima ou escreva o comando GCC em uma só linha.

## Execução

Demonstração detalhada do Caso A, com 8 indivíduos e 3 gerações:

```powershell
& "$env:TEMP\ica_nsga2-c-build\programa_nsga2.exe" A 8 3 --seed 20261102 --log steps --every 1 --output "$env:TEMP\ica-nsga2-demo"
```

Execução principal dos dois cenários:

```powershell
& "$env:TEMP\ica_nsga2-c-build\programa_nsga2.exe" both 100 200 --seed 20261102 --log quiet --output "$env:TEMP\ica-nsga2-oficial"
```

Exportar histórico de indivíduos:

```powershell
& "$env:TEMP\ica_nsga2-c-build\programa_nsga2.exe" A 8 3 --log full --every 1 --individuals --output "$env:TEMP\ica-nsga2-detalhado"
```

```text
programa_nsga2.exe [A|B|both] [populacao] [geracoes] [opcoes]

--log quiet|summary|steps|full
--every N
--seed N
--output DIRETORIO
--individuals
--help
```

O padrão é executar `both`, com população `100` e `200` gerações. São exigidos pelo menos 4 indivíduos e 1 geração.

## Arquivos de saída

Por cenário, o programa gera `pareto.csv`, `inventory.csv`, `facilities_SIMULATED.csv`, `steps.csv` e `generations.csv`. `individuals.csv` é gerado quando se informa `--individuals`.

| Arquivo | Conteúdo |
|---|---|
| `pareto.csv` | Soluções factíveis de rank zero, deduplicadas por objetivos e destino |
| `inventory.csv` | Lotes, tipos, massas e indicação da natureza dos valores |
| `facilities_SIMULATED.csv` | Instalações e parâmetros simulados de capacidade, custo, distância e risco |
| `steps.csv` | Etapas registradas por geração, entrada e saída |
| `generations.csv` | Quantidade factível, frente 1 e mínimos individuais por objetivo em cada geração |
| `individuals.csv` | Genes, objetivos, violações, viagens, rank e crowding; somente com `--individuals` |

Os mínimos de custo, risco de transporte e risco das instalações em `generations.csv` podem pertencer a indivíduos diferentes; não representam necessariamente uma única solução.

## Resultados de referência

Resultados observados para `both 100 200 --seed 20261102`:

| Indicador | Caso A | Caso B |
|---|---:|---:|
| Soluções não dominadas factíveis e deduplicadas | **55** | **68** |
| Menor custo simulado entre essas soluções | 8.853,00 | 5.434,80 |
| Risco de transporte da solução de menor custo | 172,06 | 78,78 |
| Risco das instalações da solução de menor custo | 16,37 | 11,17 |
| Viagens da solução de menor custo | 5 | 3 |

São resultados experimentais da configuração indicada: não representam valores monetários ou riscos validados. A quantidade de soluções não dominadas, isoladamente, não mede convergência ou qualidade relativa a uma frente ótima conhecida. Os resultados dos dois casos não devem ser comparados como se fossem a mesma instância.

## Referências científicas

1. **Deb, K.; Pratap, A.; Agarwal, S.; Meyarivan, T.** (2002). “A Fast and Elitist Multiobjective Genetic Algorithm: NSGA-II.” *IEEE Transactions on Evolutionary Computation*, 6(2), 182–197. [https://doi.org/10.1109/4235.996017](https://doi.org/10.1109/4235.996017).
2. **Rabbani, M.; Heidari, R.; Farrokhi-Asl, H.; Rahimi, N.** (2018). “Using metaheuristic algorithms to solve a multi-objective industrial hazardous waste location-routing problem considering incompatible waste types.” *Journal of Cleaner Production*, 170, 227–241. [https://doi.org/10.1016/j.jclepro.2017.09.029](https://doi.org/10.1016/j.jclepro.2017.09.029).
3. **Chai, H.; He, R.; Kang, R.; Jia, X.; Dai, C.** (2023). “Solving Bi-Objective Vehicle Routing Problems with Driving Risk Consideration for Hazardous Materials Transportation.” *Sustainability*, 15(9), 7619. [https://doi.org/10.3390/su15097619](https://doi.org/10.3390/su15097619).

## Limitações

Os parâmetros dos cenários A e B requerem verificação e calibração antes de qualquer aplicação operacional. A matriz de incompatibilidade também requer validação especializada. O projeto não apresenta múltiplas execuções independentes, métricas de qualidade de frentes nem comparação experimental com outros métodos; os resultados publicados aqui devem ser interpretados como uma execução reprodutível da configuração informada, não como evidência de ótimo global.
