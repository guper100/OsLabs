#include <stdio.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <string.h>

void sem(int semId, int n, int d)
{
    struct sembuf op;
    op.sem_op = d;
    op.sem_flg = 0;
    op.sem_num = n;
    semop(semId, &op, 1);
}

void unlockSem(int semId, int n)
{
    sem(semId, n, 1);
}

void lockSem(int semId, int n)
{
    sem(semId, n, -1);
}

int getSem(int semId, int n)
{
    return semctl(semId, n, GETVAL, 0);
}

int get_random_number(int min, int max)
{
    return rand() % (max - min + 1) + min;
}

/*void qs(int semId, int *s_arr, int first, int last)
{
    if (first < last)
    {
        int left = first, right = last;
		lockSem(semId, (left + right) / 2);
		int middle = s_arr[(left + right) / 2];
		unlockSem(semId, (left + right) / 2);
        do
        {
			lockSem(semId, left);
            while (s_arr[left] < middle) 
			{
				unlockSem(semId, left);
				left++;
				lockSem(semId, left);
			}
			unlockSem(semId, left);
			lockSem(semId, right);
            while (s_arr[right] > middle)
			{
				unlockSem(semId, right);
				right--;
				lockSem(semId, right);
			}
			unlockSem(semId, right);
            if (left <= right)
            {
				lockSem(semId, left);
                int tmp = s_arr[left];
				lockSem(semId, right);
                s_arr[left] = s_arr[right];
				unlockSem(semId, left);
                s_arr[right] = tmp;
				unlockSem(semId, right);
                left++;
                right--;
            }
        } while (left <= right);
        qs(semId, s_arr, first, right);
        qs(semId, s_arr, left, last);
    }
}*/

void sort(int semId, int *mem, const size_t n)
{
    for (int i = 0; i < n; i++)
    {
        int minInd = i;
        for (int j = i + 1; j < n; j++)
        {
            lockSem(semId, minInd);
            lockSem(semId, j);
            int to_unlock = minInd;
            if (mem[j] < mem[minInd])
            {
                minInd = j;
            }
            unlockSem(semId, j);
            unlockSem(semId, to_unlock);
        }
        if (i != minInd)
        {
            lockSem(semId, i);
            int t = mem[i];
            lockSem(semId, minInd);
            mem[i] = mem[minInd];
            unlockSem(semId, i);
            mem[minInd] = t;
            unlockSem(semId, minInd);
        }
    }
}

int main(int argv, char *argc[])
{
    if (argv != 4)
    {
        printf("Not supported arguments count\n");
        return 0;
    }
    int n = atoi(argc[1]);
    int min = atoi(argc[2]);
    int max = atoi(argc[3]);

    int memId = shmget(IPC_PRIVATE, sizeof(int) * n, 0600 | IPC_CREAT | IPC_EXCL);
    int *mem = (int *)shmat(memId, 0, 0);

    srand(time(NULL));
    for (int i = 0; i < n; i++)
        mem[i] = get_random_number(min, max);

    int semId = semget(IPC_PRIVATE, n, 0600 | IPC_CREAT);

    for (int i = 0; i < n; i++)
        unlockSem(semId, i);

    /*printf("%d \n",getSem(semId, 1)); 
	lockSem(semId, 1);
	printf("%d \n",getSem(semId, 1)); 
	unlockSem(semId, 1);
	printf("%d \n",getSem(semId, 1)); */

    int child_id = fork();

    if (child_id)
    {
        int i = 0;
        int status;
        do
        {
            printf("%d: ", i);
            for (int j = 0; j < n; j++)
            {
                int status_1 = getSem(semId, j);
                lockSem(semId, j);
                if (status_1)
                {
                    printf("%d ", mem[j]);
                }
                else
                {
                    printf("[%d] ", mem[j]);
                }

                fflush(stdout);
                unlockSem(semId, j);
            }
            printf("\r\n");
            status = waitpid(child_id, NULL, WNOHANG);
            i++;
        } while (!status);

        printf("Sort finished \r\n");
        for (int j = 0; j < n; j++)
            printf("%d ", mem[j]);
        printf("\r\n");
        shmctl(memId, 0, IPC_RMID);
        semctl(semId, 0, IPC_RMID);
    }
    else
    {
        //qs(semId, mem, 0, n-1);
        sort(semId, mem, n);
    }
}
