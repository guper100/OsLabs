#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int get_random_number(int min, int max)
{
    return rand() % (max - min + 1) + min;
}

int comp(const void *a, const void *b)
{
    return *(const int *)a > *(const int *)b;
}

void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

int next(int perm[], int n)
{
    int i = n - 1;
    while (--i >= 0 && perm[i] > perm[i + 1])
        ;
    if (i == -1)
        return 0;
    for (int j = i + 1, k = n - 1; j < k; j++, k--)
        swap(perm + j, perm + k);
    int j = i + 1;
    while (perm[j] < perm[i])
        j++;
    swap(perm + i, perm + j);
    return 1;
}

struct Strmsg
{
    long mtype;
    int data[4];
    int islast;
};

void parentMainCode(int msgId)
{
    struct Strmsg localmsg;
    srand(time(NULL));
    for (int i = 0; i < 4; i++)
        localmsg.data[i] = get_random_number(0, 10000);
    printf("Parent: generate %i %i %i %i\n", localmsg.data[0], localmsg.data[1], localmsg.data[2], localmsg.data[3]);
    localmsg.islast = 1;
    localmsg.mtype = 1;
    msgsnd(msgId, &localmsg, sizeof(localmsg), 0);
    int count_of_r = 0;
    do
    {
        msgrcv(msgId, &localmsg, sizeof(localmsg), 2, 0);
        if (localmsg.islast)
            break;
        count_of_r++;
        printf("Parent: get %i: %i %i %i %i\n", count_of_r, localmsg.data[0], localmsg.data[1], localmsg.data[2], localmsg.data[3]);
    } while (!localmsg.islast);
    printf("Parent: wait until child is finished.\n");
    waitpid(0, 0, 0);
    printf("Parent: releasing the message queue.\n");
    msgctl(msgId, IPC_RMID, NULL);
    printf("Parent: Process is finished.\n");
}

void childMainCode(int msgId)
{
    struct Strmsg childlocalmsg;
    msgrcv(msgId, &childlocalmsg, sizeof(childlocalmsg), 1, 0);
    printf("Child: data reading complete.\n");
    qsort(childlocalmsg.data, 4, sizeof(int), comp);
    childlocalmsg.islast = 0;
    childlocalmsg.mtype = 2;
    msgsnd(msgId, &childlocalmsg, sizeof(childlocalmsg), 0);
    do
    {
        childlocalmsg.islast = !next(childlocalmsg.data, 4);
        childlocalmsg.mtype = 2;
        msgsnd(msgId, &childlocalmsg, sizeof(childlocalmsg), 0);
    } while (!childlocalmsg.islast);
    printf("Child: the last message was send.\n");
    printf("Child: Process is finished.\n");
}

int main()
{
    const size_t semCount = 10;
    int msgId = msgget(IPC_PRIVATE, 0600 | IPC_CREAT);
    if (msgId < 0)
    {
        perror("error with msgget()");
        return -1;
    }
    else
    {
        printf("message id = %i\n", msgId);
    }
    pid_t childId = fork();
    if (childId < 0)
    {
        perror("error with fork()\n");
    }
    else if (childId > 0)
    {
        parentMainCode(msgId);
    }
    else
    {
        childMainCode(msgId);
    }
    return 0;
}