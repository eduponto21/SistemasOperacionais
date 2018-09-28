// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DAINF UTFPR
// Versão 1.0 -- Março de 2015
//
// Estruturas de dados internas do sistema operacional

#ifndef __DATATYPES__
#define __DATATYPES__

#include <ucontext.h>

#define SUSPENSA 's'//Tarefa suspensa
#define PRONTA 'p'//Tarefa pronta
#define ENCERRADA 'e'//Tarefa encerrada


// Estrutura que define uma tarefa
typedef struct task_t
{
    // preencher quando necessário
    struct task_t *prev, *next;     //para usar com biblioteca de filas (cast)
    int tid;  //ID da tarefa
    char estado; //Estado da tarefa
    //....   demais informações da tarefa
    ucontext_t context;
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
