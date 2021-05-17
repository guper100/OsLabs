#include <stdio.h>
#include <sys/shm.h>
#include <stdlib.h>

int comp(const int *a, const int *b)
{
	return *a - *b;
}

int main(int argv, char *argc[])
{
	if (argv <= 1)
	{
		printf("not enough params\n");
		return -1;
	}

	char *paramStr = argc[1];
	int memId = atoi(paramStr);

	if (memId == 0)
	{
		printf("incorrect parameter string: %s\n", paramStr);
		return -2;
	}

	printf("receiving the memory data: shmid = %i\n", memId);

	int *mem = (int *)shmat(memId, 0, 0);
	if (NULL == mem)
	{
		printf("error with shmat()\n");
		return -3;
	}

	for (int i = 0; i < 20; i++)
		printf("%i ", mem[i]);
	printf("\n");
	qsort(mem, 20, sizeof(int), comp);
	for (int i = 0; i < 20; i++)
		printf("%i ", mem[i]);
	printf("\n");
	return 0;
}
