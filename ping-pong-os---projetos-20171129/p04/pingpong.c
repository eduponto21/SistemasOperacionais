#include "pingpong.h"
#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include "queue.h"
#define STACKSIZE 32768		/* tamanho de pilha das threads */
//#define DEBUG
#define MAX_PRIORITY 20
#define MIN_PRIORITY -20
#define TASK_AGING -1
#define TASK_DEFAULT_PRIORITY 0


//Variável que controla IDs que já foram utilizados
int IDS;

task_t MainTask, Dispatcher, *CurrentTask, *prontas, *suspensas, *encerradas;

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
    prontas = NULL;
    suspensas = NULL;
    encerradas = NULL;

    //cria Dispatcher
    int id = task_create(&Dispatcher, dispatcher_body, "");
}

void dispatcher_body (void * arg) // dispatcher é uma tarefa
{
    printf("oi eu sou o dispatcher\n");
    while ( queue_size((queue_t*)prontas) > 0 )
    {
        printf("oi eu to dentro do while\n");
        task_t *next = scheduler() ; // scheduler é uma função
        if (next != NULL)
        {
            queue_remove((queue_t**)&prontas,(queue_t*)next);
            task_switch (next) ; // transfere controle para a tarefa "next"
        }
    }
    task_exit(0) ; // encerra a tarefa dispatcher
}

task_t* scheduler()
{
    task_t *check = prontas, *maxPriority = NULL;
    // Procura a tarefa com a maior prioridade na fila de prontas
    do
    {
        if(maxPriority == NULL || check->prioridade > maxPriority->prioridade) {
            maxPriority = check;
        }
        check = check->next;
    }
    while(check != prontas);
    // Envelhece a tarefa assim que a escolhe como próxia para ser executada
    task_setprio(maxPriority, task_getprio(maxPriority) + TASK_AGING);
    return maxPriority;
}


// gerência de tarefas =========================================================

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task,			// descritor da nova tarefa
                 void (*start_func)(void *),	// funcao corpo da tarefa
                 void *arg) 			// argumentos para a tarefa
{
    printf("criei");
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
        task->estado = PRONTA;
        task_setprio(task, TASK_DEFAULT_PRIORITY);
    }
    else
    {
        perror ("Erro na criação da pilha: ");
        return -1;
    }

    makecontext (&task->context, (void*)(start_func), 1,arg);

    if(task != &Dispatcher)
    {
        queue_append((queue_t **) &prontas, (queue_t*) task);
    }

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

    if(CurrentTask != &Dispatcher) {
        CurrentTask->estado = ENCERRADA;
        queue_append((queue_t **) &encerradas, (queue_t*) CurrentTask);
    }


    if(CurrentTask == &Dispatcher)
    {
        task_switch(&MainTask);
    }else
    {
        task_switch(&Dispatcher);
    }

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

// suspende uma tarefa, retirando-a de sua fila atual, adicionando-a à fila
// queue e mudando seu estado para "suspensa"; usa a tarefa atual se task==NULL
void task_suspend (task_t *task, task_t **queue) ;

// acorda uma tarefa, retirando-a de sua fila atual, adicionando-a à fila de
// tarefas prontas ("ready queue") e mudando seu estado para "pronta"
void task_resume (task_t *task) ;

// operações de escalonamento ==================================================

// libera o processador para a próxima tarefa, retornando à fila de tarefas
// prontas ("ready queue")
void task_yield ()
{
    if(CurrentTask != &MainTask)
    {
        queue_append((queue_t**)&prontas,(queue_t*)CurrentTask);
    }
    task_switch(&Dispatcher);

}

// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio)
{
    if(prio > MAX_PRIORITY) {
        prio = MAX_PRIORITY;
    } else if(prio < MIN_PRIORITY) {
        prio = MIN_PRIORITY;
    }

    if(task) {
        task->prioridade = prio;
    } else {
        CurrentTask->prioridade = prio;
    }

}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task)
{
    if(task) {
        return task->prioridade;
    } else {
        return CurrentTask->prioridade;
    }
}

// operações de sincronização ==================================================

// a tarefa corrente aguarda o encerramento de outra task
int task_join (task_t *task) ;

// operações de gestão do tempo ================================================

// suspende a tarefa corrente por t segundos
void task_sleep (int t) ;

// retorna o relógio atual (em milisegundos)
unsigned int systime () ;

// operações de IPC ============================================================

// semáforos

// cria um semáforo com valor inicial "value"
int sem_create (semaphore_t *s, int value) ;

// requisita o semáforo
int sem_down (semaphore_t *s) ;

// libera o semáforo
int sem_up (semaphore_t *s) ;

// destroi o semáforo, liberando as tarefas bloqueadas
int sem_destroy (semaphore_t *s) ;

// mutexes

// Inicializa um mutex (sempre inicialmente livre)
int mutex_create (mutex_t *m) ;

// Solicita um mutex
int mutex_lock (mutex_t *m) ;

// Libera um mutex
int mutex_unlock (mutex_t *m) ;

// Destrói um mutex
int mutex_destroy (mutex_t *m) ;

// barreiras

// Inicializa uma barreira
int barrier_create (barrier_t *b, int N) ;

// Chega a uma barreira
int barrier_join (barrier_t *b) ;

// Destrói uma barreira
int barrier_destroy (barrier_t *b) ;

// filas de mensagens

// cria uma fila para até max mensagens de size bytes cada
int mqueue_create (mqueue_t *queue, int max, int size) ;

// envia uma mensagem para a fila
int mqueue_send (mqueue_t *queue, void *msg) ;

// recebe uma mensagem da fila
int mqueue_recv (mqueue_t *queue, void *msg) ;

// destroi a fila, liberando as tarefas bloqueadas
int mqueue_destroy (mqueue_t *queue) ;

// informa o número de mensagens atualmente na fila
int mqueue_msgs (mqueue_t *queue) ;

//==============================================================================
