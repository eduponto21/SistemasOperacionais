#include "pingpong.h"
#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include "queue.h"
#define STACKSIZE 32768		/* tamanho de pilha das threads */
//#define DEBUG

//Variável que controla IDs que já foram utilizados
int IDS, ticks, flag_preempcao;
task_t MainTask, Dispatcher, *CurrentTask, *prontas, *encerradas, *adormecidas;
// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action;
// estrutura de inicialização to timer
struct itimerval timer;

void task_wakeUp();

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

    //inicializa variável que controla IDS
    IDS = -1;
    ticks = 0;
    flag_preempcao = 0;
    prontas = NULL;
    encerradas = NULL;

    //cria main
    getcontext(&MainTask.context);
    CurrentTask = &MainTask;

    MainTask.tid = ++IDS;
    MainTask.next = NULL;
    MainTask.prev = NULL;
    MainTask.estado = PRONTA;
    task_setprio(&MainTask, TASK_DEFAULT_PRIORITY);
    //Por padrão, toda tarefa é de usuário, a não ser que mude em outro lugar
    MainTask.tipo = TAREFA_DE_USUARIO;
    MainTask.initTime = systime();
    MainTask.procTime = 0;
    MainTask.executions = 0;


    if(IDS != 0)
    {
        perror ("Erro ao criar main: ");
        exit (1);
    }

    //cria Dispatcher
    IDS = task_create(&Dispatcher, dispatcher_body, "");
    Dispatcher.tipo = TAREFA_DE_SISTEMA;

    if(IDS != 1)
    {
        perror ("Erro ao criar dispatcher: ");
        exit (1);
    }


    //Cria Timer
    action.sa_handler = tratador_de_sinal;
    sigemptyset (&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction (SIGALRM, &action, 0) < 0)
    {
        perror ("Erro em sigaction: ") ;
        exit (1) ;
    }

    //Ajusta temporizador
    timer.it_value.tv_usec = 100;      // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0 ;      // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 100 ;   // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0 ;   // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL (vide man setitimer)
    if (setitimer (ITIMER_REAL, &timer, 0) < 0)
    {
        perror ("Erro em setitimer: ") ;
        exit (1) ;
    }

    //abre dispatcher
    task_yield();
}

void dispatcher_body (void * arg) // dispatcher é uma tarefa
{
    while ( queue_size((queue_t*)prontas) > 0 || queue_size((queue_t*)adormecidas) > 0)
    {
        task_t *next;
        if(queue_size((queue_t*)prontas) > 0)
        {
            next = scheduler();
        }
        if (next != NULL && queue_size((queue_t*)prontas) > 0)
        {
            queue_remove((queue_t**)&prontas,(queue_t*)next);
            next->quantum = DEFAULT_QUANTUM;
            next->executions++;
            Dispatcher.executions++;
            task_switch (next) ; // transfere controle para a tarefa "next"
        }
        task_wakeUp();
    }
    task_exit(0) ; // encerra a tarefa dispatcher
}

task_t* scheduler()
{
    task_t *check = prontas, *maxPriority = NULL;
    // Procura a tarefa com a maior prioridade na fila de prontas
    do
    {
        if(maxPriority == NULL)
        {
            maxPriority = check;
        }
        else if(check->prioridade_dinamica < maxPriority->prioridade_dinamica)
        {
            maxPriority->prioridade_dinamica = maxPriority->prioridade_dinamica + TASK_AGING;
            maxPriority = check;
        }
        else
        {
            check->prioridade_dinamica = check->prioridade_dinamica + TASK_AGING;
        }
        check = check->next;
    }
    while(check != prontas);
    // desenvEnvelhece a tarefa assim que a escolhe como próxia para ser executada
    maxPriority->prioridade_dinamica = maxPriority->prioridade_estatica;
    return maxPriority;
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
        task->estado = PRONTA;
        task_setprio(task, TASK_DEFAULT_PRIORITY);
        //Por padrão, toda tarefa é de usuário, a não ser que mude em outro lugar
        task->tipo = TAREFA_DE_USUARIO;
        task->initTime = systime();
        task->procTime = 0;
        task->executions = 0;
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
    CurrentTask->endTime = systime();
    printf ("\nTask %d exit: execution time %d ms, processor time %d ms, %d activations\n\n",
            task_id(), CurrentTask->endTime-CurrentTask->initTime, CurrentTask->procTime, CurrentTask->executions) ;

    CurrentTask->exitCode = exitCode;

    while(queue_size((queue_t*)CurrentTask->suspensas) > 0)
    {
        task_resume(CurrentTask->suspensas);
    }

    CurrentTask->estado = ENCERRADA;
    queue_append((queue_t **) &encerradas, (queue_t*) CurrentTask);


    task_yield();
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task)
{
#ifdef DEBUG
    printf ("task_switch: trocando contexto %d -> %d\n",CurrentTask->tid, task->tid);
#endif
    task_t *aux = CurrentTask;
    CurrentTask = task;

//Porque não funciona fazer separado?!?!?!
//    int i = getcontext(&aux->context);
//    if(i == -1)
//    {
//        return i;
//    }
//    i = setcontext(&task->context);


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
void task_suspend (task_t *task, task_t **queue)
{
    if(task == NULL)
    {
        task = CurrentTask;
    }

    if(queue != NULL)
    {
        task->estado = SUSPENSA;
        queue_append((queue_t **) queue, (queue_t*) task);
    }
}

// acorda uma tarefa, retirando-a de sua fila atual, adicionando-a à fila de
// tarefas prontas ("ready queue") e mudando seu estado para "pronta"
void task_resume (task_t *task)
{
    queue_remove((queue_t **) &CurrentTask->suspensas, (queue_t*) task);
    task->estado = PRONTA;
    queue_append((queue_t **) &prontas, (queue_t*) task);

}

// operações de escalonamento ==================================================

// libera o processador para a próxima tarefa, retornando à fila de tarefas
// prontas ("ready queue")
void task_yield ()
{
    if(flag_preempcao == 0 && CurrentTask != &Dispatcher)
    {
        if(CurrentTask->estado == PRONTA)
        {
            queue_append((queue_t**)&prontas,(queue_t*)CurrentTask);
        }
        task_switch(&Dispatcher);
    }
}

// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio)
{
    //valida limites
    if(prio < MAX_PRIORITY)
    {
        prio = MAX_PRIORITY;
    }
    else if(prio > MIN_PRIORITY)
    {
        prio = MIN_PRIORITY;
    }

    if(task)
    {
        task->prioridade_estatica = prio;
        task->prioridade_dinamica = prio;
    }
    else
    {
        CurrentTask->prioridade_estatica = prio;
        CurrentTask->prioridade_dinamica = prio;
    }
}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task)
{
    if(task)
    {
        return task->prioridade_estatica;
    }
    else
    {
        return CurrentTask->prioridade_estatica;
    }
}

//
void tratador_de_sinal (int signum)
{
    ticks++;
    CurrentTask->procTime++;
    //Somente decrementa de tarefas de usuario, para evitar instabilidade
    if(CurrentTask->tipo == TAREFA_DE_USUARIO)
    {
        CurrentTask->quantum--;
        //Se acabar o quantum da tarefa, reseta e retorna ao dispatcher
        if (CurrentTask->quantum == 0)
        {
            task_yield();
        }
    }
}

// retorna o relógio atual (em milisegundos)
unsigned int systime ()
{
    return ticks;
}

// operações de sincronização ==================================================

// a tarefa corrente aguarda o encerramento de outra task
int task_join (task_t *task)
{
    if(task == NULL)
    {
        return -1;
    }
    else if(task->estado == ENCERRADA)
    {
        return task->exitCode;
    }
    else
    {
        flag_preempcao = 1;
        task_suspend(CurrentTask, &task->suspensas);
        flag_preempcao = 0;
        task_yield();
        return task->exitCode;
    }

}

// operações de gestão do tempo ================================================

// suspende a tarefa corrente por t segundos
void task_sleep (int t)
{
    flag_preempcao = 1;
    CurrentTask->wakeTime = systime()+t*1000;
    CurrentTask->estado = ADORMECIDA;
    queue_append((queue_t **) &adormecidas, (queue_t*) CurrentTask);
    flag_preempcao = 0;
    task_yield();
}

void task_wakeUp()
{
    flag_preempcao = 1;
    if(adormecidas != NULL)
    {
        task_t *check;
        do
        {
            check = adormecidas;
            if(check->wakeTime <= systime())
            {
                queue_remove((queue_t **)&adormecidas, (queue_t *) check);
                check->estado= PRONTA;
                queue_append((queue_t **)&prontas, (queue_t *) check);
            }
            else
            {
                check = adormecidas->next;
            }
        }
        while((check!=adormecidas) && (adormecidas!=NULL));
    }
    flag_preempcao = 0;
}
// operações de IPC ============================================================

// semáforos

// cria um semáforo com valor inicial "value"
int sem_create (semaphore_t *s, int value);

// requisita o semáforo
int sem_down (semaphore_t *s);

// libera o semáforo
int sem_up (semaphore_t *s);

// destroi o semáforo, liberando as tarefas bloqueadas
int sem_destroy (semaphore_t *s);
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
