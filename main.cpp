#include <iostream>
#include <memory>
#include <vector>
#include <queue>
#include <random>
#include <algorithm>
#include "include/schedulers.hpp"
#include "include/modelos.hpp"

int main() {
    int tempoSimulacao = 10000;
    auto cenarios = criarCenarios();

    for (auto& cenario : cenarios) {
        std::cout << "=======================================================\n";
        std::cout << "EXECUTANDO: " << cenario.nome << "\n";

        std::vector<Tarefa*> ptrosTarefas;
        for (auto& t : cenario.tarefas) {
            ptrosTarefas.push_back(&t);
        }

        // 1. Executa RTA (Teoria do RM)
        analisarRTA(ptrosTarefas);

        // 2. Executa Teoria do EDF
        float utilizacao = calcularUtilizacaoTotal(ptrosTarefas);
        bool escalonavelEDF = (utilizacao <= 1.0f);

        std::cout << "Resultado EDF (Teorico): "
                  << (escalonavelEDF ? "ESCALONAVEL (U <= 1.0)" : "SOBRECARGADO (U > 1.0 - Pode haver perda de deadline)")
                  << "\n";

        // 3. Executa as simulações empíricas com geradores e tarefas resetados
        std::random_device rd;
        unsigned int seed = rd();

        // --- SIMULAÇÃO EDF ---
        for (auto* t : ptrosTarefas) t->reset();
        std::mt19937 geradorEDF(seed);
        simulacao<Compara_Job>("EDF (Dinamico)", ptrosTarefas, geradorEDF, tempoSimulacao);

        // --- SIMULAÇÃO RM ---
        for (auto* t : ptrosTarefas) t->reset();
        std::mt19937 geradorRM(seed);
        simulacao<Compara_Job_RM>("RM (Fixo)", ptrosTarefas, geradorRM, tempoSimulacao);
        std::cout << "\n";
    }
    std::cout << "=======================================================\n";
    return 0;
}
