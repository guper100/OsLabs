#include <stdio.h>

int main(int argv, char *argc[])
{
    if (argv != 3)
    {
        perror("You should enter name of input file and count of reading bytes\n");
        return -1;
    }

#define input_file_name argc[1]
    int byte_count = atoi(argc[2]);
    printf("%d\n", byte_count);

    FILE *fin = fopen(input_file_name, "r");
    if (NULL == fin)
    {
        perror("cannot open file");
        return -1;
    }

    printf("read result = ");
    for (int i = 0; i < byte_count; i++)
    {
        char data;
        fread(&data, sizeof(char), 1, fin);
        printf("%c", data);
    }
    printf("\n");
    fclose(fin);
    return 0;
}
