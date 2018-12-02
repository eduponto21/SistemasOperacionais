#include "queue.h"
#include <stdio.h>
//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila

void queue_append (queue_t **queue, queue_t *elem)
{
    if(queue == NULL)
    {
        printf("fila não existe\n");
        return;
    }
    if(elem == NULL)
    {
        printf("elemento não existe\n");
        return;
    }
    if(elem->prev != NULL || elem->next != NULL)
    {
        printf("elemento já está em uma fila\n");
        return;
    }
    if(*queue == NULL)
    {
        elem->next = elem;
        elem->prev = elem;
        *queue = elem;
    }
    else
    {
        elem->next = *queue;
        elem->prev = (*queue)->prev;
        (*queue)->prev->next = elem;
        (*queue)->prev = elem;
    }
}

//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: apontador para o elemento removido, ou NULL se erro

queue_t *queue_remove (queue_t **queue, queue_t *elem)
{
    if(queue == NULL)
    {
        printf("fila não existente\n");
        return NULL;
    }
    if(queue_size(*queue) == 0)
    {
        printf("fila vazia\n");
        return NULL;
    }
    if(elem == NULL)
    {
        printf("elemento inexistente\n");
        return NULL;
    }

    //Verifica se ele está na lista...
    if(!pertence_lista(queue,elem))
    {
        printf("elemento não pertence a fila\n");
        return NULL;
    }

    if(*queue != elem) //Se não for o primeiro
    {
        elem->prev->next = elem->next;
        elem->next->prev = elem->prev;
    }
    else if(queue_size(*queue) == 1) //Se for o último
    {
        *queue = NULL;
        elem->prev->next = elem->next;
        elem->next->prev = elem->prev;
    }
    else //Se for o primeiro
    {
        *queue = elem->next;
        elem->prev->next = elem->next;
        elem->next->prev = elem->prev;
    }

    elem->next = NULL;
    elem->prev = NULL;

    return elem;
}
//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size (queue_t *queue)
{
    if(queue == NULL)
    {
        return 0;
    }
    queue_t *check = queue->next;
    int qnt = 1;
    while(check != queue)
    {
        qnt++;
        check = check->next;
    }
    return qnt;
}

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca. Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir

void queue_print (char *name, queue_t *queue, void print_elem (void*) )
{
    if(queue == NULL)
    {
        return;
    }
    queue_t *check = queue;
    do
    {
        print_elem(check);
        check = check->next;
    }
    while(check != queue);
}

//------------------------------------------------------------------------------
// Percorre a fila para saber se o elemento faz parte dela
//
// Retorno: 1 caso faça parte, 0 caso não faça parte.

int pertence_lista(queue_t **queue, queue_t *x)
{
    queue_t *check = *queue;
    do
    {
        if(x == check)
        {
            return 1;
        }
        check = check->next;
    }
    while(check != *queue);
    return 0;
}
