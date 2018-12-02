#include <stdio.h>
#include <stdlib.h>
#include "pingpong.h"

// operating system check
#if defined(_WIN32) || (!defined(__unix__) && !defined(__unix) && (!defined(__APPLE__) || !defined(__MACH__)))
#warning Este codigo foi planejado para ambientes UNIX (LInux, *BSD, MacOS). A compilacao e execucao em outros ambientes e responsabilidade do usuario.
#endif

#define BUFFER_SIZE 5

semaphore_t s_vaga, s_buffer, s_item;
int buffer[BUFFER_SIZE], bufferPos;
task_t p1, p2, p3, c1, c2;

void insereNoBuffer(int item);
int consumirDoBuffer();
void produtor(void *id);
void consumidor(void *id);

void produtor(void *id)
{
    int item;
    while (1)
    {
        task_sleep (1);
        item = random()%100;

        sem_down(&s_vaga);
        sem_down(&s_buffer);

        insereNoBuffer(item);

        int *tid = (int*)id;
        printf("P%d Produziu: %d\n", *tid-1, item);

        sem_up (&s_buffer);
        sem_up (&s_item);
    }
    task_exit(0);
}

void consumidor(void *id)
{
    int item;
    while (1)
    {
        sem_down(&s_item);
        sem_down(&s_buffer);

        item = consumirDoBuffer();

        sem_up(&s_buffer);
        sem_up(&s_vaga);

        int *tid = (int*)id;
        printf("                      C%d Consumiu: %d \n", *tid-4, item);

        task_sleep (1);
    }
    task_exit(0);
}

void insereNoBuffer(int item)
{
    buffer[bufferPos] = item;
    bufferPos++;
}

int consumirDoBuffer()
{
   int i, item = buffer[0];
   for(i=1; i<bufferPos; i++)
   {
        buffer[i-1] = buffer[i];
   }
   bufferPos--;
   return item;
}

int main(int argc, char *argv[])
{
    pingpong_init();

    sem_create(&s_vaga, 6);
    sem_create(&s_buffer, 2);
    sem_create(&s_item, 0);
    bufferPos = 0;

    task_create (&p1, produtor, &p1.tid);
    task_create (&p2, produtor, &p2.tid);
    task_create (&p3, produtor, &p3.tid);
    task_create (&c1, consumidor, &c1.tid);
    task_create (&c2, consumidor, &c2.tid);

    task_exit(0);
    exit(0);
}
