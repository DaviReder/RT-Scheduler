#ifndef SCHEDULERS_HPP_INCLUDED
#define SCHEDULERS_HPP_INCLUDED
#include "tarefa.hpp"
#include <queue>
#include <vector>
#include <iostream>
#include <string>
#include <memory>

template<typename Comparador>
void simulacao(const std::string& nomeAlg, std::vector<Tarefa*> tarefas, std::mt19937& seed, int time){
    int qntDeads=0, qntSucessos=0;
    long long somaLatencias=0; // acumulador O(1): nunca guardamos latências individuais
    std::priority_queue<std::unique_ptr<Job>, std::vector<std::unique_ptr<Job>>, Comparador> jobs;

    for(int i=0; i<time; i++){
        for(auto& t : tarefas){
            if(t->deveChegar(i,seed)){
                jobs.push(std::make_unique<Job>(t, i));
            }
        }

        if(!jobs.empty()){
            auto& mais_urgente = jobs.top();
            switch(mais_urgente->verificar(i)){
            case 1:
                qntSucessos++;
                somaLatencias += (i - mais_urgente->getTickChegada());
                jobs.pop();
                break;
            case 2:
                qntDeads++;
                jobs.pop();
                break;
            case 3:
                break;
            }
        }
    }

    // Limpeza final automática ao desempilhar
    while(!jobs.empty()) {
        jobs.pop();
    }

    std::cout << "\nAlgoritmo: " << nomeAlg;
    std::cout << "\nSucesso: " << qntSucessos;
    std::cout << "\nDeadlines Perdidos: " << qntDeads;
    if(qntSucessos != 0){
        std::cout << "\nRazao (Deads Por Sucesso): " << (float)qntDeads/qntSucessos;
        std::cout << "\nLatencia Media de Resposta: " << (double)somaLatencias/qntSucessos << " ticks\n";
    }
}
#endif
