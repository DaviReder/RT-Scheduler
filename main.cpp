#include <iostream> //função de entrada/saida
#include <memory> //ponteiros inteligentes (saem de escopo)
#include <vector> //inclui o array dinamico
#include <queue>
#include <algorithm> //algortimos genericos
#include <random>

class Tarefa{
    int periodo, execFixo, ultimaChegada;
    bool periodica;

public:
    Tarefa(int p, int eF, bool per) : periodo(p), execFixo(eF), ultimaChegada(-100), periodica(per){}
    ~Tarefa() = default;
    int getPeriodo(){ return periodo; }
    int getExecFixo(){ return execFixo; }

    bool deveChegar(int tick, std::mt19937& gerador){
        if(periodica){
            return (tick % periodo == 0);
        }
        else{
            //Esporadica
            if(tick-ultimaChegada<periodo) { return false; }
            std::uniform_int_distribution<int> distribuicao(0, 9); //Faz com que a distribuição caia no intervalo de forma justa
            if(distribuicao(gerador) != 0){
                return false;
            }
            ultimaChegada = tick;   // ESSENCIAL — sem isso o MIT nunca é respeitado
            return true;
        }
    }
};

class Job{
    Tarefa* tarefa;
    int deadlineAbsoluto, tempoRestante;
    bool ativo;

public:
    Job(Tarefa* t, int tick) : tarefa(t){
        deadlineAbsoluto = t->getPeriodo() + tick;
        tempoRestante = t->getExecFixo();
        ativo = true;
    }
    ~Job() = default;

    bool getAtivo(){ return ativo; }
    int getDeadline(){ return deadlineAbsoluto; }
    int getTempoRestante(){ return tempoRestante; }

    void executar(){
        if(tempoRestante > 0){
           tempoRestante-=1;
        }
    }

    int estado(int tick){
        if(tempoRestante <= 0){
            //Tarefa executou com sucesso
            ativo = false;
            std::cout << "\n[SUCESSO] - Tarefa executou.";
            return 1;
        }
        else if((deadlineAbsoluto - tick) == 0){
            //Aqui excedeu o limite;
            ativo = false;
            std::cout << "\n[Dealine]- Excedeu o limite de tempo.";
            return 2;
        }
        //rodando
        return 3;
    }

    int verificar(int tick){
        executar();
        int res = estado(tick);
        return res;
    }
};

struct Compara_Job{
    bool operator()(Job *a, Job *b) const{
        return a->getDeadline() > b->getDeadline();
    }
};

int main(){
    //Configurando o randomico
    std::random_device rd; //Pega os milissegundos do relogio do computador e transforma em semente unica.
    std::mt19937 gerador(rd()); //Lista imensa de numeros, passando o inteiro gerado como ponto de partida

    //Configuração da simulação
    int time=10000; //Tempo para simulação.
    int qntDeads=0, qntRodando=0, qntSucessos=0;

    //Instancia de cada tarefa
    /*
    Tarefa t1(10, 3, true);   // periódica
    Tarefa t2(8, 2, true);    // periódica
    Tarefa t3(6, 1, false);   // esporádica
    Tarefa t4(4, 1, false);   // esporádica
    */
    Tarefa t1(20, 3, true),  t2(16, 2, true),  t3(14, 2, true),  t4(12, 1, true);
    Tarefa t5(16, 1, false), t6(12, 1, false), t7(10, 1, false), t8(9,  1, false);

    std::vector<Tarefa*> tarefas = {&t1, &t2, &t3, &t4, &t5, &t6, &t7, &t8};
    std::priority_queue<Job*, std::vector<Job*>, Compara_Job> jobs;
    Job* mais_urgente;

    for(int i=0; i<time; i++){
        for(auto& t : tarefas){
            if(t->deveChegar(i,gerador)){
               Job* novo = new Job(t, i);
               jobs.push(novo);
            }
        }

        //Deletar deads
        while (!jobs.empty() && jobs.top()->getDeadline() <= i && jobs.top()->getTempoRestante() > 0) {
            qntDeads++;
            delete jobs.top();
            jobs.pop();
        }

        //2. Verificar se alguma tarefa está em execucao, e comparar com a do schedule. Se sim, guardar essa e chamar a outra.
        if(!jobs.empty()){
            mais_urgente = jobs.top();

            switch(mais_urgente->verificar(i)){
            case 1:
                qntSucessos++;
                delete mais_urgente;
                jobs.pop();
                break;
            case 2:
                qntDeads++;
                delete mais_urgente;
                jobs.pop();
                break;
            case 3:
                qntRodando++;
                break;
            }
        }

        //3. Decrementar tempo de execução ou validar se a tarefa encerrou.

    }

    while(!jobs.empty()) {
        delete jobs.top();
        jobs.pop();
    }
    std::cout << "\n\nValores!\nSucesso: " << qntSucessos << ".\nRodando: " << qntRodando << ".\nDeads: "<< qntDeads << ".";
}
