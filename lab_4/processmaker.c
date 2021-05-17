#include <stdio.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int comp(const int *a, const int *b)
{
    return *a - *b;
}

void sortAndPrint(int *mem)
{
    qsort(mem, 20, sizeof(int), comp);
    for (size_t i = 0; i < 20; i++)
    {
        printf("%i ", mem[i]);
    }
    printf("\n");
}

int main()
{
    srand(time(NULL));

    int memId = shmget(IPC_PRIVATE, sizeof(int) * 20, 0600 | IPC_CREAT | IPC_EXCL);

    int *numbers = (int *)shmat(memId, 0, 0);

    for (size_t i = 0; i < 20; i++)
    {
        numbers[i] = rand() % 10000;
    }

    for (size_t i = 0; i < 20; i++)
    {
        printf("%d ", numbers[i]);
    }

    printf("\n");

    int child_id = fork();

    if (child_id == 0)
    {
        sortAndPrint(numbers);
    }
    else
    {
        waitpid(child_id, NULL, 0);
    }

    shmdt(numbers);
    shmctl(memId, 0, IPC_RMID);

    return 0;
}
