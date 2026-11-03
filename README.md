# NSGA-II para Otimização da Logística de Resíduos Perigosos

Implementação experimental e didática do NSGA-II em **C11 (ISO/IEC 9899:2011)**. O programa atribui lotes de resíduos a destinadores simulados, agrupa lotes em viagens de ida e volta e minimiza custo, risco de transporte e risco das instalações.

Este projeto é uma **adaptação experimental simplificada inspirada em Rabbani et al. (2018)**. Não reproduz o modelo matemático, a rede logística, as instâncias, os parâmetros calibrados nem os resultados daquele artigo. A implementação mantém uma codificação e uma logística deliberadamente reduzidas para fins de estudo.

> **Limite de uso:** não é um planejador logístico operacional. Não deve orientar transporte, tratamento, licenciamento ou segurança de resíduos reais. Os riscos não são probabilidades calibradas e a matriz de incompatibilidade é ilustrativa, sem validação química.

## Escopo e diferenças em relação a Rabbani et al. (2018)

O artigo de Rabbani et al. formula um problema industrial de location-routing com múltiplos nós de geração, decisões de localização de instalações e tecnologias, centros de tratamento, reciclagem e disposição, frota heterogênea, restrições de rota e incompatibilidade entre resíduos. Esta adaptação considera sete lotes fixos, uma origem abstrata, seis destinadores fixos e uma viagem de ida e volta por destinador/viagem. Não decide localização, abertura de instalações, rede rodoviária, múltiplos pontos de coleta, múltiplas paradas ou fluxos secundários entre tratamento, reciclagem e disposição.

O NSGA-II implementado segue o ciclo de população inicial, torneio binário, cruzamento, mutação, ordenação não dominada, crowding distance e seleção ambiental elitista entre pais e filhos. Isso segue a estrutura geral de Deb et al. (2002), mas a representação, os operadores, o tratamento das restrições e a função de avaliação são específicos deste protótipo.

### Objetivos e ordem das funções

O artigo de Rabbani et al. apresenta `f1` como custo total, `f2` como risco de instalação e `f3` como risco de transporte por exposição da população. **A ordem de `f2` e `f3` na implementação é inversa à notação do artigo.**

No programa, os três objetivos minimizados são:

1. `f1` (`objetivos[0]`): custo simulado de destinação, `massa × custo/kg`, mais custo fixo por viagem e `distância de ida e volta × custo/km`.
2. `f2` (`objetivos[1]`): risco de transporte simulado, somando `distância de ida e volta × coeficiente de risco do destinador × massa da viagem`.
3. `f3` (`objetivos[2]`): risco local simulado, somando `massa × coeficiente de risco local do destinador`.

As unidades são internamente consistentes **se** os coeficientes forem interpretados como custo por kg, custo por km, risco por kg-km e risco local por kg, respectivamente. Os coeficientes são inventados para a simulação, não calibrados, e não há moeda, exposição populacional ou unidade de risco validada. Logo, os valores numéricos não são comparáveis aos valores do artigo nem representam impactos reais. A fundamentação complementar de Chai et al. (2023) trata de risco de condução e roteamento; ela não valida os coeficientes simplificados deste código.

## Instâncias, dados e limitações

- **Caso A — `A_SSL_Thamirabarani_ILUSTRATIVO`:** sete lotes codificados como hipotéticos (85, 40, 55, 28, 110, 130 e 35 kg). Não foram autenticados nem extraídos integralmente de um IHM do navio.
- **Caso B — `B_Sample_Ship_Japao`:** sete quantidades aproximadas (60, 3,5, 10, 50, 20, 96 e 3,8 kg) associadas ao inventário-modelo de uma embarcação. São inventário aproximado, não quantidades comprovadamente removidas como resíduos.
- **Destinadores F1–F6:** instalações, capacidades, elegibilidades, custos, distâncias, riscos e matriz de incompatibilidade são simulados. A matriz de incompatibilidade não foi validada por FDS/FISPQ ou especialista.
- **Logística:** cada viagem serve um destinador e retorna à origem abstrata. Um empacotamento first-fit agrupa lotes elegíveis na ordem de prioridade, respeitando capacidade do veículo e separando pares incompatíveis dentro de cada viagem.
- **Restrições:** destinos elegíveis são mantidos pelos operadores. Capacidade das instalações, capacidade por viagem e incompatibilidades são verificadas na avaliação; violações recebem penalidade e indivíduos inviáveis são preteridos por dominância de restrições. Os operadores não reparam toda violação de capacidade para instâncias futuras.
- **Procedência disponível:** os três artigos citados estão no diretório dois níveis acima desta pasta. O IHM autenticado e o arquivo `Inventory_GL_Appendix.pdf` do exemplo japonês não estão presentes no workspace analisado; portanto, a procedência primária das quantidades do Caso B não pôde ser confirmada aqui.

## Parâmetros experimentais

Valores padrão: população `100`, gerações `200`, semente `20261102`, capacidade do veículo `150 kg`, custo fixo simulado por viagem `120`, custo simulado por km `4.2`, mutação de destino `0.14` por gene e mutação de prioridade `0.4` por indivíduo. O cruzamento seleciona cada gene de destino com probabilidade `0.5`; prioridades são combinadas por chaves aleatórias e ordenadas. Não foi encontrada evidência de calibração por Taguchi ou varredura de parâmetros nesta implementação. A semente fixa reproduz a execução neste build e biblioteca; não garante a mesma sequência de distribuições de C++ em outras bibliotecas/compiladores.

Os totais anteriormente observados de **55 soluções para A** e **68 para B** são resultados computacionais de uma execução com esses parâmetros. A paridade CSV com a versão C++ adjacente é um teste de regressão entre implementações, não uma validação científica do modelo ou prova de ótimo global. Consulte [a auditoria técnica](docs/AUDITORIA_TECNICA.md) para os achados por arquivo e as verificações da frente final.

## Compilação e execução

Requer GCC e G++ disponíveis no `PATH`. O script compila cada `.c` como C11 e usa G++ somente para ligar os objetos neste ambiente Windows; nenhum fonte C++ integra o executável C11.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\compilar.ps1 -Otimizado
& "$env:TEMP\ica_nsga2-c-build\programa_nsga2.exe" A 8 3 --log steps --every 1 --output "$env:TEMP\ica-nsga2-demo"
```

Suíte de testes:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\compilar.ps1 -Testar -Otimizado
```

Validação dos descendentes, factibilidade e não dominância da população final:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tests\validar_frente.ps1
```

Execução oficial dos dois casos:

```powershell
& "$env:TEMP\ica_nsga2-c-build\programa_nsga2.exe" both 100 200 --seed 20261102 --log quiet --output "$env:TEMP\ica-nsga2-oficial"
```

Comparação de regressão com a versão C++17 irmã, quando ambas as pastas estiverem lado a lado:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tests\verificar_paridade.ps1
```

Opções: `A|B|both [populacao] [geracoes]`, `--log quiet|summary|steps|full`, `--every N`, `--seed N`, `--output DIRETORIO`, `--individuals`, `--help`.

## Saídas

Por cenário, são exportados `pareto.csv`, `inventory.csv`, `facilities_SIMULATED.csv`, `steps.csv` e `generations.csv`. `individuals.csv` é incluído quando `--individuals` é solicitado. A frente exportada contém soluções factíveis de rank zero, deduplicadas por objetivos e destino.

## Organização

```text
include/   Contratos e estruturas C11
src/       Implementação modular C11
tests/     Testes em C e comparação de regressão C/C++17
docs/      Auditoria científica, matemática e computacional
```

## Referências científicas

1. **Rabbani, M.; Heidari, R.; Farrokhi-Asl, H.; Rahimi, N.** (2018). “Using metaheuristic algorithms to solve a multi-objective industrial hazardous waste location-routing problem considering incompatible waste types.” *Journal of Cleaner Production*, 170, 227–241. DOI: [10.1016/j.jclepro.2017.09.029](https://doi.org/10.1016/j.jclepro.2017.09.029). Fundamenta o problema industrial, os objetivos originais e as incompatibilidades; não é a formulação implementada.
2. **Deb, K.; Pratap, A.; Agarwal, S.; Meyarivan, T.** (2002). “A Fast and Elitist Multiobjective Genetic Algorithm: NSGA-II.” *IEEE Transactions on Evolutionary Computation*, 6(2), 182–197. DOI: [10.1109/4235.996017](https://doi.org/10.1109/4235.996017). Fundamenta os mecanismos gerais do NSGA-II.
3. **Chai, H.; He, R.; Kang, R.; Jia, X.; Dai, C.** (2023). “Solving Bi-Objective Vehicle Routing Problems with Driving Risk Consideration for Hazardous Materials Transportation.” *Sustainability*, 15(9), 7619. DOI: [10.3390/su15097619](https://doi.org/10.3390/su15097619). Referência complementar sobre risco de condução e roteamento; não fornece os dados simulados deste projeto.

Os PDFs usados na auditoria estavam disponíveis no workspace original (`1-s2.0-S0959652617320176-main (1).pdf`, `DebPratapAgarwalMeyarivan.pdf` e `sustainability-15-07619-v2.pdf`), mas não foram copiados para este repositório. Use os DOIs acima para localizar as publicações; o IHM autenticado e o inventário japonês primário também não acompanham este projeto.
