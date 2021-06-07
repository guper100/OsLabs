#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argv, char *argc[])
{
    if (argv != 3)
    {
        perror("You should enter name of input and output file\n");
        return -1;
    }

#define input_file_name argc[1]
#define output_file_name argc[2]

    int input_file = open(input_file_name, O_RDONLY, 0600);
    int output_file = open(output_file_name, O_RDWR | O_CREAT, 0600);

    if (input_file < 0)
    {
        perror("error with open of input file");
        return -1;
    }

    if (output_file < 0)
    {
        perror("error with open of map file");
        return -1;
    }

    struct stat st;
    stat(input_file_name, &st);
#define file_size st.st_size
    ftruncate(output_file, file_size);

    char *data = (char *)mmap(NULL, file_size, PROT_WRITE | PROT_READ, MAP_SHARED, output_file, 0);
    if (MAP_FAILED == data)
    {
        perror("error with mmap");
        return -2;
    }

    read(input_file, data, file_size);

    if (0 != munmap(data, file_size))
    {
        perror("error with unmapping");
        return -3;
    }
    close(input_file);
    close(output_file);
    return 0;
}
