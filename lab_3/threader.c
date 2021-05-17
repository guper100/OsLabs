#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

struct data
{
     const char *text;
     int count;
     int time;
};

void *print_message_function(void *data_in)
{
     struct data *input = (struct data *)data_in;
     for (int i = 0; i < input->count; i++)
     {
          printf(input->text, i);
          sleep(input->time);
     }
     return (void *)(0);
}
int main()
{
     pthread_t thread1, thread2;
     struct data data1 = {"Hello Threads (%i)\n", 10, 1};
     struct data data2 = {"This is iteration %i\n", 12, 2};
     int res1 = pthread_create(&thread1, NULL, print_message_function, (void *)&data1);
     int res2 = pthread_create(&thread2, NULL, print_message_function, (void *)&data2);
     int iret1, iret2;
     pthread_join(thread1, (void **)&iret1);
     pthread_join(thread2, (void **)&iret2);

     printf("Thread 1 returns: %d\n", iret1);
     printf("Thread 2 returns: %d\n", iret2);

     return 0;
}