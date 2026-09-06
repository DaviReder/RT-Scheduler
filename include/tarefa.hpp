#ifndef TAREFA_HPP_INCLUDED
#define TAREFA_HPP_INCLUDED

class Tarefa{
    int periodo, execFixo, ultimaChegada;
    bool periodica;

public:
    Tarefa(int p, int eF, bool per) : periodo(p), execFixo(eF), ultimaChegada(-100), periodica(per){}
    ~Tarefa() = default;
    int getPeriodo() const { return periodo; }
    int getExecFixo() const { return execFixo; }

    // Demorei achar: aqui ocorre um caso raro no tempo T0. (tick - (- 100)) = tick + 100, isso obriga a tarefa a ser esporadica.
    void reset() {
        ultimaChegada = -100;
    }

    bool deveChegar(int tick, std::mt19937& gerador){
        if(periodica){
            return (tick % periodo == 0);
        }
        else{
            //Esporadica
            if(tick-ultimaChegada<periodo) { return false; }
            std::uniform_int_distribution<int> distribuicao(0, 9);
            if(distribuicao(gerador) != 0){
                return false;
            }
            ultimaChegada = tick;
            return true;
        }
    }
};

class Job{
    Tarefa* tarefa;
    int tickChegada, deadlineAbsoluto, tempoRestante;
    bool ativo;

public:
    Job(Tarefa* t, int tick) : tarefa(t), tickChegada(tick){
        deadlineAbsoluto = t->getPeriodo() + tick;
        tempoRestante = t->getExecFixo();
        ativo = true;
    }
    ~Job() = default;

    bool getAtivo() const { return ativo; }
    int getDeadline() const { return deadlineAbsoluto; }
    int getTempoRestante() const { return tempoRestante; }
    int getPeriodo() const { return tarefa->getPeriodo(); }
    int getTickChegada() const { return tickChegada; }

    void executar(){
        if(tempoRestante > 0){
           tempoRestante-=1;
        }
    }

     int estado(int tick){
        if(tempoRestante <= 0){
            ativo = false;
            return 1;
        }
        return 3;
    }
    int verificar(int tick){
        if (tick >= deadlineAbsoluto) {
            ativo = false;
            return 2;
        }
        executar();
        return estado(tick);
    }
};

//Por deadline.
struct Compara_Job {
    bool operator()(const std::unique_ptr<Job>& a, const std::unique_ptr<Job>& b) const {
        return a->getDeadline() > b->getDeadline();
    }
};

//Por periodo.
struct Compara_Job_RM {
    bool operator()(const std::unique_ptr<Job>& a, const std::unique_ptr<Job>& b) const {
        return a->getPeriodo() > b->getPeriodo();
    }
};

#endif // TAREFA_HPP_INCLUDED
