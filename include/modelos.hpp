#ifndef MODELOS_HPP_INCLUDED
#define MODELOS_HPP_INCLUDED
#include "tarefa.hpp"
#include <cmath>  // pra usar pow()

struct Cenario {
    std::string nome;
    std::vector<Tarefa> tarefas;
};

inline std::vector<Cenario> criarCenarios(){
    return {
        {
            "CASO 1 - Baseline seguro (u=0.477)",
            { Tarefa(30,2,true), Tarefa(25,2,true), Tarefa(20,1,true), Tarefa(18,1,true),
              Tarefa(22,1,false), Tarefa(20,1,false), Tarefa(16,1,false), Tarefa(15,1,false) }
        },
        {
            "CASO 2 - Acima do limite conservador, RM ainda funciona (u=0.817)",
            { Tarefa(16,2,true), Tarefa(14,2,true), Tarefa(12,1,true), Tarefa(10,1,true),
              Tarefa(14,1,false), Tarefa(12,1,false), Tarefa(10,1,false), Tarefa(9,1,false) }
        },
        {
            "CASO 3 - RM falha GARANTIDO por Response Time Analysis (u=0.883)",
            { Tarefa(10,4,true), Tarefa(15,5,true), Tarefa(20,3,true) }
        },
        {
            "CASO 4 - Sobrecarga severa: EDF colapsa PIOR que RM (u=2.767)",
            { Tarefa(6,3,true), Tarefa(5,2,true), Tarefa(4,2,true), Tarefa(4,1,true),
              Tarefa(5,1,false), Tarefa(4,1,false), Tarefa(3,1,false), Tarefa(3,1,false) }
        },
        {
            "CASO 5 - Priority inversion / monopolizacao (u=1.396)",
            { Tarefa(50,15,true), Tarefa(10,1,true), Tarefa(8,1,true), Tarefa(6,1,true),
              Tarefa(9,1,false), Tarefa(7,1,false), Tarefa(5,1,false), Tarefa(4,1,false) }
        }
    };
}

inline float calcularUtilizacaoTotal(const std::vector<Tarefa*>& tarefas) {
    float total = 0;
    for (auto& t : tarefas) {
        total += (float)t->getExecFixo() / t->getPeriodo();
    }
    return total;
}

inline float calcularLimiteRM(int n) {
    return n * (std::pow(2.0, 1.0/n) - 1);
}

// Calcula o tempo de resposta de UMA tarefa, dado quem tem prioridade maior que ela
// RTA Teórico Ajustado: Distingue o custo no pior caso
inline float calcularTempoResposta(Tarefa* alvo, const std::vector<Tarefa*>& maisPrioritarias) {
    float R = alvo->getExecFixo();

    for (int iteracao = 0; iteracao < 100; iteracao++) {
        float interferencia = 0;

        for (const auto& t : maisPrioritarias) {
            // No Pior Caso Teórico, a interferência máxima de uma esporádica acontece quando ela chega no seu intervalo mínimo (periodo)
            interferencia += std::ceil(R / t->getPeriodo()) * t->getExecFixo();
        }

        float novoR = alvo->getExecFixo() + interferencia;

        if (novoR == R) return R; // Convergiu (Tempo de Resposta Encontrado)
        if (novoR > alvo->getPeriodo()) return novoR; // Estourou o Deadline
        R = novoR;
    }
    return R;
}

// Nota: estou copiando os valores de tarefas e não passando por referencia.
inline bool analisarRTA(std::vector<Tarefa*> tarefas) {
    // 1. Ordena o vetor por prioridade do Rate Monotonic (menor período = maior prioridade)
    std::sort(tarefas.begin(), tarefas.end(), [](Tarefa* a, Tarefa* b) {
        return a->getPeriodo() < b->getPeriodo();
    });

    std::cout << "\nAnálise de Tempo de Resposta (RTA para RM)\n";
    bool conjuntoEscalonavel = true;
    std::vector<Tarefa*> maisPrioritarias;

    for (size_t i = 0; i < tarefas.size(); i++) {
        Tarefa* t = tarefas[i];
        float R = calcularTempoResposta(t, maisPrioritarias);
        bool passou = (R <= t->getPeriodo());

        std::cout << "Tarefa (P=" << t->getPeriodo() << ", C=" << t->getExecFixo() << ") "
                  << "-> R = " << R << " / D = " << t->getPeriodo()
                  << " [" << (passou ? "OK" : "FALHA") << "]\n";

        if (!passou) {
            conjuntoEscalonavel = false;
        }

        // Adiciona a tarefa atual na lista de maior prioridade para a próxima iteração
        maisPrioritarias.push_back(t);
    }

    std::cout << "Resultado RTA: O conjunto e "
              << (conjuntoEscalonavel ? "ESCALONAVEL pelo RM." : "INACESSIVEL / VAI FALHAR NO RM.") << "\n";

    return conjuntoEscalonavel;
}

#endif // MODELOS_HPP_INCLUDED
