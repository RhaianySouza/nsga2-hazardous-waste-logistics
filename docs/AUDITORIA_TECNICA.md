# Auditoria científica, matemática e computacional

## Escopo da auditoria

Revisão da implementação em C11, dos testes, do README, das instâncias A/B e dos três PDFs disponíveis no workspace. A versão C++17 vizinha foi usada somente como referência de regressão para CSV. Nenhuma decisão de localização foi adicionada; não foram introduzidos pontos de coleta, instalações abertas ou fluxos secundários.

O modelo de Rabbani et al. (2018) é um problema industrial integrado de localização e roteamento, com rede de geração, tratamento, reciclagem e disposição, tecnologias, instalações operacionais, frota e riscos de exposição populacional. Este programa é uma adaptação didática simplificada. Paridade com a versão C++ confirma compatibilidade entre as duas implementações locais, não equivalência com o artigo nem validação em dados reais.

## Tabela de auditoria

| Arquivo e função | Comportamento encontrado | Correspondência com Rabbani (2018) | Problema identificado | Severidade | Correção recomendada / aplicada |
|---|---|---|---|---|---|
| `src/aleatorio.c`: MT19937, `sortearInteiro`, `sortearUnidade` | Gerador próprio com semente explícita; vetores esperados cobrem saída MT e distribuições usadas para reproduzir o build de referência. | NSGA-II é estocástico; artigo usa experimentos e calibração de parâmetros. | Sequência fixa foi conferida para a biblioteca/build local; C não padroniza a sequência de distribuições de C++ entre bibliotecas. | Baixa | Preservar; documentar que reprodutibilidade numérica é local ao build e à semente. Documentado no README.
| `src/operadores.c`: `criarIndividuoAleatorio`, `cruzarIndividuos`, `mutarIndividuo`, `selecionarTorneio` | Destinos são escolhidos/mutados entre instaladores elegíveis; prioridade inicia permutada, cruzamento reconstrói permutação por chaves aleatórias e mutação troca posições; torneio compara rank e crowding. | Mecanismos correspondem em alto nível a seleção, recombinação e mutação evolutivas, mas não afirmam ser os parâmetros/operadores de Rabbani. | Capacidade não é reparada pelo operador; é verificada na avaliação e penalizada. Para as duas instâncias atuais, as capacidades simuladas são suficientes para os lotes elegíveis. | Baixa | Preservar os operadores; documentar e testar elegibilidade, permutação e avaliação de capacidade. Teste C ampliado.
| `src/avaliacao.c`: `avaliarIndividuo` | Calcula custo de tratamento por massa e custo fixo/quilometragem por viagem; risco de transporte por massa, distância e coeficiente; risco local por massa; penaliza capacidade de instalação, capacidade de veículo e elegibilidade. Incompatíveis são separados em viagens distintas pelo first-fit. | Não corresponde às equações originais de Rabbani. O artigo inclui custos e riscos baseados em nós/fluxos e exposição populacional, além de custos fixos de instalações. | A indexação no código está invertida em relação à ordem de riscos do artigo; risco local/rodoviário tem coeficientes simulados e sem calibração. | Média | Não alterar os objetivos por estar fora da autorização de replicar o artigo. Explicitar fórmulas, ordem, unidades condicionais e simplificações no README.
| `src/dominancia.c`: `individuoDomina` | Factível domina inviável; entre inviáveis, menor soma de violação domina; entre factíveis, usa dominância Pareto nos três objetivos. | É tratamento de restrições compatível em espírito com dominância por factibilidade, mas não é formulação de Rabbani. | Penalidades somam excessos em kg e penalidades artificiais de elegibilidade; não são magnitudes homogêneas. Nos dados atuais, destinos iniciais/mutados são elegíveis e excessos não ocorreram nos resultados verificados. | Média | Preservar a regra de restrições sem mudanças nos resultados; declarar a escala como penalidade algorítmica e não grandeza física. Documentado no README.
| `src/dominancia.c`: `calcularCrowdingIndices`, `calcularCrowding`, `classificarPopulacao` | Crowding normalizado por amplitude de cada um dos três objetivos; frentes com menos de três indivíduos recebem infinito. Amplitude nula é ignorada. Extremos da ordenação recebem infinito. | Corresponde ao mecanismo geral de crowding distance do NSGA-II de Deb et al. (2002). | Falha de alocação na ordenação do crowding era ignorada; a seleção poderia prosseguir com distâncias zeradas. Empates nos extremos seguem a ordem do sort e podem dar infinito a apenas alguns empatados. | Média (OOM); baixa (empates) | **Corrigido:** falha de alocação agora retorna erro e interrompe a classificação. Mantido o tratamento de empates para preservar o comparador/ordenação de referência; documentar que a escolha entre valores exatamente empatados pode depender da ordenação.
| `src/ordenacao.c`: `ordenarIndicesComoCpp` | Introsort com insertion sort/heap fallback, comparador estrito e desempate equivalente ao build C++ de referência utilizado. | Ordenação é mecanismo auxiliar; a forma específica não é exigida por Rabbani. | Valores empatados não têm ordenação estável garantida; isso pode afetar a escolha entre indivíduos com crowding igual, sem alterar a relação matemática de dominância. | Baixa | Preservar para compatibilidade. Não atribuir importância científica a uma ordem entre soluções empatadas.
| `src/evolucao.c`: `executarNSGA2ComRegistro`, `selecionarAmbiente` | Inicializa N indivíduos, classifica; cada geração seleciona N pares por torneio, cria N filhos, combina pais+filhos (2N), classifica e trunca por rank/crowding até N. | O ciclo de seleção ambiental elitista pai-filho é o elemento central de NSGA-II de Deb et al. (2002). | Não foi encontrado descarte de pais antes da seleção ambiental nem perda de elitismo no fluxo normal. Falhas de alocação encerram a geração. | — | Preservar; testes e paridade verificados.
| `src/problema.c`: `criarProblema`, `validarProblema` | Dados fixos de sete lotes, seis destinadores, cinco classes; capacidades, elegibilidades, custos, distância, risco e matriz de incompatibilidade são definidos no código. | Instâncias não são as instâncias numéricas do artigo de Rabbani. | O Caso A é explicitamente hipotético. Caso B tem quantidades aproximadas, mas o PDF/planilha de inventário japonês citado no README antigo não está no workspace. | Média | Preservar as instâncias didáticas e rotulá-las como simuladas/aproximadas; não atribuir proveniência oficial. Documentado no README.
| `src/exportacao.c`: `salvarResultados` | Exporta só indivíduos de rank zero e violação abaixo do limiar; ordena objetivos e deduplica por objetivos/destino. | Frente de Pareto é compatível com saída multiobjetivo; o critério de decisão e os dados não são os do artigo. | CSV isolado não permite certificar dominância contra a população inteira; por isso a checagem foi feita no histórico final `individuals.csv`. | — | Preservar; verificação independente contra todos os 100 indivíduos da última geração executada abaixo.
| `tests/testes.c`, `tests/verificar_paridade.ps1`, `tests/validar_frente.ps1` | Testes C cobrem RNG, estrutura, dominância, frente/crowding, operadores e arquivos; script recompila referência C++17 e compara CSV; validador percorre descendentes e população final. | Testes de regressão validam implementação e invariantes, não validam o modelo científico de Rabbani. | Antes da auditoria, o teste não verificava diretamente a factibilidade do filho nem sinalizava falha de crowding. | Baixa | **Corrigido:** acrescidos testes de crowding, factibilidade, elegibilidade e permutação; criado validador de descendentes/frente final. Paridade executada.
| `README.md` | Descreve C11, escopo, relações e diferenças científicas, dados, parâmetros, execução e referências. | Distingue claramente inspiração do modelo implementado. | A documentação anterior não trazia referências detalhadas, ordem original dos objetivos e ressalva explícita de que a paridade não valida cientificamente o modelo. | Média | **Corrigido:** README atualizado com as distinções, procedência disponível e ausência dos documentos primários de dados.

## Formulação e unidades

Para lote `i`, instalação atribuída `f(i)`, massa `q_i`, viagem `t` e instalação `f`, a avaliação corresponde a:

```text
f1 = Σ_i q_i × custoTratamentoKg[f(i)]
     + Σ_t (custoFixoViagem + 2 × distânciaKm[destino(t)] × custoKm)

f2 = Σ_t (2 × distânciaKm[destino(t)]
          × riscoRodoviaKgKm[destino(t)] × massa[t])

f3 = Σ_i q_i × riscoLocalKg[f(i)]
```

Em Rabbani et al. (2018), `f2` é o risco de instalação baseado em população exposta nas instalações e `f3` é o risco de transporte ao longo das rotas, também com população/exposição. Aqui, `f2` é transporte e `f3` é instalação. Dimensionalmente, as expressões do código fecham se os coeficientes forem definidos como custo/kg, custo/km, risco/(kg·km) e risco/kg. O programa não define unidades físicas para risco, não usa população e não utiliza custos cotados; seus resultados são scores de simulação.

## Instâncias e dados

| Caso/dado | Natureza constatada | Uso permitido na interpretação |
|---|---|---|
| Caso A: 85, 40, 55, 28, 110, 130, 35 kg | Lotes nomeados no código como hipotéticos/ilustrativos. Nenhum IHM autenticado foi encontrado no workspace. | Instância sintética de demonstração; não reportar como quantidades oficiais do MV SSL Thamirabarani. |
| Caso B: 60, 3,5, 10, 50, 20, 96, 3,8 kg | Quantidades aproximadas do inventário-modelo; arquivo primário japonês ausente do workspace. | Ilustração baseada em quantidades aproximadas; não afirmar remoção/geração real como resíduo. |
| F1–F6 e incompatibilidades | Valores construídos no código, sem empresas, licenças ou validação química identificadas. | Simulação de elegibilidade/capacidade/custo/risco; não usar operacionalmente. |
| Frota e rota | Um veículo abstrato de 150 kg; uma viagem atende um destino, saindo e retornando à origem. | Empacotamento simplificado, não VRP completo nem rede de rotas reais. |

## NSGA-II e restrições

- **População inicial:** cada lote recebe um destinador elegível; prioridades são embaralhadas.
- **Cromossomo:** vetores `destino[lote]` e `prioridade[lote]`.
- **Seleção:** torneio binário por menor rank, maior crowding e desempate aleatório.
- **Cruzamento/mutação:** combina destinos por gene; prioridades são reconstruídas como permutação; mutação troca destinos elegíveis e/ou duas prioridades.
- **Dominância:** problemas factíveis precedem inviáveis; inviáveis são comparados pela penalidade total; factíveis usam Pareto nos três objetivos.
- **Crowding:** soma das distâncias normalizadas nos objetivos; extremos recebem infinito; amplitudes nulas não são divididas.
- **Elitismo:** pais e filhos formam R; a seleção ambiental preenche N por frentes e ordena a frente de corte por crowding.
- **Parada:** termina após exatamente o número configurado de gerações; não há critério de convergência.
- **Factibilidade:** cada lote permanece em instalação elegível; avaliação soma violações de capacidade e o first-fit separa incompatíveis por viagem. As capacidades atuais deixam os resultados verificados factíveis. Para uma futura instância, operadores não prometem reparação de capacidade antes da avaliação.

## Verificações executadas

Com GCC/G++ disponível no ambiente, versão C11 otimizada compilada e suíte de testes executada. A comparação oficial usou população 100, 200 gerações e semente 20261102; `pareto.csv` C e C++17 coincidiu linha a linha nos dois casos: 55 soluções em A e 68 em B.

`tests/validar_frente.ps1` executado com população 100, 200 gerações e semente 20261102. Em cada caso foram verificados os 20.000 descendentes registrados: violation zero, destinadores elegíveis e permutação de prioridades. Nos 100 indivíduos finais de cada população, foram recalculadas atribuição elegível, carga por instalação, empacotamento por capacidade do veículo, separação dos pares incompatíveis e contagem de viagens. Para cada indivíduo rank zero, compararam-se os três objetivos com todos os indivíduos finais; não foi encontrado dominador. As frentes rank zero tiveram 100 indivíduos em cada caso e os CSVs exportaram 55 (A) e 68 (B) soluções deduplicadas. Isso verifica o classificador/frente para essas execuções, sem demonstrar ótimo global ou validade empírica dos parâmetros.

## Referências e arquivos locais

1. Rabbani, M.; Heidari, R.; Farrokhi-Asl, H.; Rahimi, N. (2018). “Using metaheuristic algorithms to solve a multi-objective industrial hazardous waste location-routing problem considering incompatible waste types.” *Journal of Cleaner Production*, 170, 227–241. DOI: [10.1016/j.jclepro.2017.09.029](https://doi.org/10.1016/j.jclepro.2017.09.029).
2. Deb, K.; Pratap, A.; Agarwal, S.; Meyarivan, T. (2002). “A Fast and Elitist Multiobjective Genetic Algorithm: NSGA-II.” *IEEE Transactions on Evolutionary Computation*, 6(2), 182–197. DOI: [10.1109/4235.996017](https://doi.org/10.1109/4235.996017).
3. Chai, H.; He, R.; Kang, R.; Jia, X.; Dai, C. (2023). “Solving Bi-Objective Vehicle Routing Problems with Driving Risk Consideration for Hazardous Materials Transportation.” *Sustainability*, 15(9), 7619. DOI: [10.3390/su15097619](https://doi.org/10.3390/su15097619).

The audit copies of these three PDFs were available in the original workspace but are not bundled in the public repository. Use the DOI links to locate each publication.
