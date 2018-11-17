// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DAINF UTFPR
// Versão 1.0 -- Março de 2015
//
// Estruturas de dados internas do sistema operacional

#ifndef __DATATYPES__
#define __DATATYPES__

#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>

#define SUSPENSA 's'//Tarefa suspensa
#define PRONTA 'p'//Tarefa pronta
#define ENCERRADA 'e'//Tarefa encerrada
#define MAX_PRIORITY -20
#define MIN_PRIORITY 20
#define TASK_AGING -1
#define TASK_DEFAULT_PRIORITY 0
#define TAREFA_DE_SISTEMA 'S'
#define TAREFA_DE_USUARIO 'U'
#define DEFAULT_QUANTUM 20

// Estrutura que define uma tarefa
typedef struct task_t
{
    // preencher quando necessário
    struct task_t *prev, *next;     //para usar com biblioteca de filas (cast)
    ucontext_t context; //contexto
    int tid;  //ID da tarefa
    char estado; //Estado da tarefa {Suspensa, Pronta, Encerrada}
    int prioridade_estatica, prioridade_dinamica;
    char tipo; //Tipo da tarefa {Sistema, Usuário}
    int quantum; //Para controle de tempo de cada tarefa
    int initTime;
    int endTime;
    int procTime;
    int executions;
} task_t ;

// estrutura que define um semáforo
typedef struct
{
    // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
    // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
    // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
    // preencher quando necessário
} mqueue_t ;

#endif
