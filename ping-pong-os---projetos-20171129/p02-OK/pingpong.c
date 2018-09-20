#include "pingpong.h"
#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include "queue.h"
#define STACKSIZE 32768		/* tamanho de pilha das threads */
//#define DEBUG

//Variável que controla IDs que já foram utilizados
int IDS;
ucontext_t MainContext, AuxContext;

task_t MainTask, *CurrentTask, *tasks_queue;

// funções gerais ==============================================================

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void pingpong_init ()
{
    /**Esta função inicializa as estruturas internas do SO. Por enquanto, conterá
    apenas algumas inicializações de variáveis e a seguinte instrução, que desativa
    o buffer utilizado pela função printf, para evitar condições de disputa que
    podem ocorrer nesse buffer ao usar as funções de troca de contexto.**/

    // desativa o buffer da saida padrao (stdout), usado pela função printf
    setvbuf(stdout,0, _IONBF,0);

    getcontext(&MainTask.context);

    CurrentTask = &MainTask;

    //inicializa variável que controla IDS
    IDS = 0;
    tasks_queue = NULL;
//    char *argv[];
//    int mainId = task_create(&MainTask,main(),argv);
}

// gerência de tarefas =========================================================

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task,			// descritor da nova tarefa
                 void (*start_func)(void *),	// funcao corpo da tarefa
                 void *arg) 			// argumentos para a tarefa
{
    char *stack;

    getcontext(&task->context);
    stack = malloc (STACKSIZE);

    if (stack)
    {
        task->context.uc_stack.ss_sp = stack ;
        task->context.uc_stack.ss_size = STACKSIZE;
        task->context.uc_stack.ss_flags = 0;
        task->context.uc_link = &task->next->context;
        task->tid = ++IDS;
        task->next = NULL;
        task->prev = NULL;
    }
    else
    {
        perror ("Erro na criação da pilha: ");
        return -1;
    }

    makecontext (&task->context, (void*)(start_func), 1,arg);

    queue_append((queue_t **) &tasks_queue, (queue_t*) task);

    #ifdef DEBUG
    printf ("task_create: criou tarefa %d\n", task->tid) ;
    #endif

    return IDS;
}


// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode)
{
    #ifdef DEBUG
    printf ("task_exit: tarefa %d sendo encerrada\n", task_id()) ;
    #endif
    queue_remove((queue_t **) &tasks_queue, (queue_t*) CurrentTask);
    task_switch(&MainTask);
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task)
{
    #ifdef DEBUG
    printf ("task_switch: trocando contexto %d -> %d\n",CurrentTask->tid, task->tid);
    #endif

    task_t *aux = CurrentTask;
    CurrentTask = task;

    int i = swapcontext(&aux->context, &task->context);
    if(i == -1)
    {
        return i;
    }

}

// retorna o identificador da tarefa corrente (main eh 0)
int task_id ()
{
    return CurrentTask->tid;
}
