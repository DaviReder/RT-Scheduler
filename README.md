# RT Scheduler — Simulador de Escalonamento de Tempo Real (C++)

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Status: Fase 1 concluída](https://img.shields.io/badge/status-Fase%201%20conclu%C3%ADda-brightgreen.svg)](#estado-do-projeto--roadmap)

Simulador de escalonamento de tarefas de tempo real, comparando **Rate
Monotonic (RM)** e **Earliest Deadline First (EDF)** sobre os mesmos
conjuntos de tarefas, com tarefas periódicas e esporádicas coexistindo.

O projeto combina análise teórica de pior caso (Response Time Analysis —
RTA — e o teste de Liu & Layland) com simulação empírica, para expor
comportamentos sistêmicos que a teoria por si só não deixa óbvios: o
efeito dominó do EDF sob sobrecarga e a monopolização por inversão de
prioridade.

## Estado do projeto / Roadmap

Este projeto está sendo construído em fases deliberadas, alinhadas ao
que eu já domino em C++ em cada momento — não ao que seria "ideal" ter
desde o início.

- ✅ **Fase 1 (concluída) — Motor de decisão, single-thread.** Tudo que
  está descrito neste README. O tempo é representado por um contador de
  ticks lógico, não por threads reais. Isso foi uma escolha, não uma
  limitação descoberta depois: eu ainda não havia estudado
  `std::thread`, `mutex` e `condition_variable` em C++ quando comecei o
  projeto, e simular a *decisão* do scheduler (quem roda, quem espera,
  quem perde deadline) não depende de concorrência real — só de saber
  responder "dado o estado do sistema neste tick, quem deveria estar
  rodando?". Isso permitiu validar RM e EDF corretamente sem depender de
  uma disciplina (SO/concorrência) que eu ainda não tinha estudado a
  fundo.
- 🔜 **Fase 2 (planejada) — Execução concorrente real.** Trocar o loop de
  ticks por tarefas representadas como threads reais, competindo por um
  recurso de CPU protegido por sincronização (`mutex`/
  `condition_variable`), com o scheduler decidindo preempção de fato.
  Isso testaria inversão de prioridade e overhead de troca de contexto
  na prática, não apenas simulados (ver Cenário 5). Só começará depois
  de eu ter estudado concorrência em C++ com profundidade suficiente
  para não tratá-la de forma superficial.

## O que este projeto demonstra

- Implementação de dois algoritmos clássicos de escalonamento real-time
- Modelo de chegada periódica e esporádica (com tempo mínimo entre
  chegadas — *Minimum Inter-arrival Time*)
- Validação empírica contra o teste de Liu & Layland e contra o
  Response Time Analysis
- Gerenciamento de memória moderno (`unique_ptr`, sem `new`/`delete`
  manual)
- Cinco cenários de teste, cada um isolando deliberadamente um
  comportamento teórico diferente — incluindo um resultado
  contraintuitivo sobre os limites do próprio teste de Liu & Layland
- Métrica de latência média de resposta calculada com um acumulador
  online (soma + contagem), sem guardar amostras individuais

## Conceitos

**RM (Rate Monotonic)**: prioridade fixa, decidida uma única vez —
quem tem menor período sempre executa primeiro, independente do que
mais esteja acontecendo no sistema.

**EDF (Earliest Deadline First)**: prioridade dinâmica — a cada
instante, executa quem tem o deadline absoluto mais próximo.

**Teste de Liu & Layland (1973)**, usado para prever se RM consegue
garantir zero perda de deadline:

```
Σ (Ci / Ti) ≤ n · (2^(1/n) − 1)
```

onde `Ci` é o tempo de execução e `Ti` o período de cada tarefa, e `n`
o número de tarefas. É uma condição **suficiente, não necessária** —
passar do limite não implica falha garantida (ver Cenário 2 e a seção
de Achados).

Para EDF, o teste é mais simples: `Σ (Ci/Ti) ≤ 1.0` garante zero perda
de deadline — EDF é *ótimo* nesse sentido preciso: nenhum algoritmo de
escalonamento consegue aproveitar mais capacidade de CPU do que ele.

**Response Time Analysis (RTA)**: um teste exato (necessário e
suficiente) para RM, usado neste projeto para confirmar
escalonabilidade quando o teste de Liu & Layland já não é suficiente
para decidir (ver justificativa de implementação abaixo).

## Arquitetura

```
main.cpp                — cenários de teste e loop de simulação
include/tarefa.hpp      — Tarefa (modelo) e Job (instância em execução)
include/schedulers.hpp  — motor de simulação genérico (template) + comparadores RM/EDF
include/modelos.hpp     — cenários, cálculo de utilização, RTA
```

`Tarefa` representa o modelo estático (período, tempo de execução,
periódica ou esporádica). `Job` representa uma instância específica
gerada por uma `Tarefa` num tick — cada ativação cria um novo `Job`,
permitindo que múltiplas instâncias da mesma tarefa coexistam sob
sobrecarga, em vez de descartar silenciosamente ativações concorrentes.

### Por que `unique_ptr<Job>` na fila de prioridade, e não `Job*` cru

Uma versão inicial do projeto guardava **cópias de `Tarefa`** na fila
em vez de ponteiros — um bug real (ver seção de Bugs). A correção
óbvia seria usar `Job*`, mas isso reabriria a pergunta "quem é dono
desse `Job`? Quem faz o `delete`?" toda vez que um job é descartado
(sucesso, deadline perdido, ou limpeza final). `unique_ptr` responde
essa pergunta de forma estrutural: a fila é dona, e a posse termina
automaticamente quando o `unique_ptr` sai de escopo — sem `delete`
manual em nenhum ponto do código, e sem a possibilidade estrutural do
bug original de cópia silenciosa acontecer de novo.

### Por que o motor de simulação é um template (`simulacao<Comparador>`)

A alternativa seria polimorfismo em tempo de execução (uma interface
`IComparador` com método virtual). Isso funcionaria, mas pagaria custo
de indireção virtual em um hot loop que já roda até 10.000 iterações
por cenário, e — mais importante — obrigaria a alocar comparadores no
heap. Como RM e EDF diferem *apenas* no critério de prioridade,
resolver essa diferença em tempo de compilação com um parâmetro de
template elimina a indireção e deixa explícito que só o comparador
muda: nenhuma lógica de execução é duplicada entre os dois algoritmos.

### Por que os comparadores retornam `>` em vez de `<`

`std::priority_queue` é, por padrão, um **max-heap**: o topo é o
"maior" elemento segundo o comparador. Como o objetivo é que o topo
seja sempre o job **mais urgente** (menor deadline para EDF, menor
período para RM), o comparador precisa inverter a lógica: retornar
`true` quando `a` é *menos* urgente que `b`, para que o job mais
urgente termine no topo. `a->getDeadline() > b->getDeadline()`
significa "a é menos urgente que b" — por isso o `>` e não o `<`.

### Por que EDF e RM rodam com a mesma seed de aleatoriedade

Cada cenário gera um único `seed` (via `std::random_device`) e o
reutiliza para os dois geradores (`geradorEDF`, `geradorRM`). Isso
garante que as tarefas esporádicas cheguem exatamente nos mesmos ticks
nas duas simulações — sem isso, uma diferença de resultado entre RM e
EDF poderia ser efeito do acaso na geração de chegadas, e não do
algoritmo de escalonamento em si. É o que torna a comparação
empiricamente válida.

### Por que a latência média usa um acumulador, e não um vetor de amostras

A métrica reportada ao final de cada simulação é o tempo médio entre a
chegada de um job e sua conclusão com sucesso. A forma ingênua de
calcular isso seria guardar cada latência individual num
`std::vector` e tirar a média no final — mas isso paga um custo que
cresce com o número de sucessos: em cenários de baixa utilização, a
simulação produz milhares de sucessos em 10.000 ticks, o que
significaria realocações sucessivas do vetor (o clássico crescimento
amortizado do `std::vector`, mas ainda assim memória proporcional ao
número de jobs) só para, no final, colapsar tudo isso em um único
número.

Como a média é a única informação necessária, `Job` guarda o tick em
que chegou (`tickChegada`), e a cada sucesso a simulação soma
`tick_atual - tickChegada` a um acumulador (`somaLatencias`) e
incrementa o contador de sucessos que já existia. A média final é só
`somaLatencias / qntSucessos`. Isso é O(1) em memória e O(1) por job
processado, independente de o cenário produzir 10 ou 100.000
sucessos — nenhuma amostra individual precisa sobreviver além do tick
em que foi gerada.

## Os cinco cenários

Cada cenário foi calculado (utilização = Σ Ci/Ti) **antes** de rodar,
para prever o resultado teórico e só depois confirmar contra a
simulação — não o contrário.

| # | Cenário | Utilização | RM previsto | EDF previsto | Resultado |
|---|---|---|---|---|---|
| 1 | Baseline seguro | 0.477 | OK | OK | Ambos com 0 deadlines perdidos |
| 2 | Acima do limite conservador | 0.817 | Limite excedido | OK | **RM ainda funciona na prática** — ver Achados |
| 3 | RM falha comprovadamente | 0.883 | Falha (RTA) | OK | RM perde deadlines; EDF não |
| 4 | Sobrecarga severa | 2.767 | Falha | Falha | EDF falha proporcionalmente **mais** que RM |
| 5 | Priority inversion | 1.396 | Falha | Falha | Tarefa de baixa prioridade e execução longa monopoliza a CPU |

### Por que o Cenário 3 existe separado do Cenário 2

O Cenário 2 já ultrapassa o limite conservador de Liu & Layland, mas
RM não falha na prática — o que prova que o teste é suficiente, não
necessário, mas não prova o inverso (que RM *pode* falhar). O Cenário
3 foi calibrado especificamente com um conjunto de tarefas onde a
Response Time Analysis — o teste exato — indica falha certa, para
demonstrar de forma reprodutível que RM tem um limite real, e não
apenas um limite conservador de garantia.

## Achados — o que a simulação revelou, além do esperado

**Liu & Layland é suficiente, não necessário.** O Cenário 2 ultrapassa
o limite conservador de Liu & Layland (0.817 > 0.724 para 8 tarefas),
mas RM ainda entrega zero deadlines perdidos na prática. Isso não é
erro de implementação — é uma propriedade conhecida, mas fácil de
interpretar errado, do próprio teste: passar do limite remove a
*garantia* matemática simples, mas não implica falha certa. O Cenário
3 foi calibrado para forçar uma falha comprovada pelo método exato
(RTA), e nele RM de fato perde deadlines de forma consistente e
reprodutível: numa execução típica, RM entrega 2000 sucessos com 167
deadlines perdidos (latência média de ~5.33 ticks nos que sucedem),
contra 2167 sucessos e zero perdas do EDF (~5.69 ticks) — RM ainda
consegue responder um pouco mais rápido nos jobs que entrega, mas
paga isso com falhas categóricas nos que não consegue.

**EDF pode colapsar pior que RM sob sobrecarga extrema (efeito
dominó).** No Cenário 4, com utilização quase 3x a capacidade da CPU,
EDF tenta atender todas as tarefas de forma "justa" por deadline, o
que gera uma cascata: a fila de pendentes cresce continuamente, e a
maioria dos jobs perde o prazo antes mesmo de receber qualquer tempo
de CPU. Numa execução típica, EDF entrega apenas 3 sucessos em 10.000
ticks contra 2659 deadlines perdidos de RM — mas RM ainda consegue
5027 sucessos no mesmo período. RM, por decidir prioridade de forma
fixa, garante que as tarefas de maior prioridade continuem funcionando
bem mesmo quando o sistema como um todo está insustentável —
sacrificando de forma previsível apenas as de menor prioridade.

## Exemplo de saída

Saída real do CASO 3 (RM falha comprovadamente por RTA), mostrando a
análise teórica seguida das duas simulações empíricas — os números
podem variar levemente entre execuções por causa do componente
aleatório das tarefas esporádicas, mas o padrão qualitativo (RM
perdendo deadlines, EDF não) é consistente:

```
=======================================================
 EXECUTANDO: CASO 3 - RM falha GARANTIDO por Response Time Analysis (u=0.883)
=======================================================

Análise de Tempo de Resposta (RTA para RM)
Tarefa (P=10, C=4) -> R = 4 / D = 10 [OK]
Tarefa (P=15, C=5) -> R = 9 / D = 15 [OK]
Tarefa (P=20, C=3) -> R = 21 / D = 20 [FALHA]
Resultado RTA: O conjunto e INACESSIVEL / VAI FALHAR NO RM.
Resultado EDF (Teorico): ESCALONAVEL (U <= 1.0)

==========================================
Algoritmo: EDF (Dinamico)
Sucesso: 2167
Deadlines Perdidos: 0
Razao (Deads Por Sucesso):0
Latencia Media de Resposta: 5.69359 ticks
==========================================

==========================================
Algoritmo: RM (Fixo)
Sucesso: 2000
Deadlines Perdidos: 167
Razao (Deads Por Sucesso):0.0835
Latencia Media de Resposta: 5.3315 ticks
==========================================
```

## Bugs reais encontrados durante o desenvolvimento

- **Cópia silenciosa em vez de referência**: uma versão inicial guardava
  cópias de `Tarefa` na fila de prioridade em vez de ponteiros,
  desconectando o estado da fila do estado real das tarefas.
- **MIT nunca persistido**: o campo de "última chegada" de uma tarefa
  esporádica era lido mas nunca atualizado, permitindo chegadas mais
  frequentes do que o intervalo mínimo declarado.
- **Checagem de deadline dependente do critério de prioridade da fila**:
  um `while` de limpeza checava apenas o topo da fila para detectar
  deadlines vencidos — correto para EDF (onde o topo é sempre o
  deadline mais próximo), mas incorreto para RM (onde o topo é
  ordenado por período, podendo esconder um job de baixa prioridade já
  vencido atrás de um de alta prioridade). Corrigido movendo a checagem
  para dentro do momento em que cada job é de fato processado.

## Limitações conhecidas / trabalho futuro

- **Sem concorrência real** — ver Fase 2 no roadmap acima. A simulação
  é single-threaded, com tempo representado por um contador de ticks
  lógico, não relógio de parede.
- **Loop de convergência do RTA limitado a 100 iterações** (em
  `calcularTempoResposta`), sem esse limite ser configurável ou
  justificado formalmente no código — na prática converge muito antes
  disso para os conjuntos de tarefas testados, mas um conjunto
  patológico poderia teoricamente precisar de mais iterações antes de
  estourar o `if (novoR > alvo->getPeriodo())`.
- **Sem testes automatizados** (ex: um teste que verifique que o
  Cenário 3 sempre produz deadlines perdidos em RM) — hoje a validação
  é feita rodando manualmente e inspecionando a saída.
- **Parâmetro `tick` não utilizado em `Job::estado(int tick)`** — gera
  warning com `-Wextra`; mantido por enquanto por simetria com
  `verificar(int tick)`, mas deveria ser removido ou justificado.

## Compilando e rodando

```bash
g++ -std=c++20 -Wall -Wextra main.cpp -o rt_scheduler
./rt_scheduler
```

## Licença

Distribuído sob a licença MIT. Veja o badge no topo do README para
detalhes.
