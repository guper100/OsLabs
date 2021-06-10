#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <strings.h>
#include <unistd.h>
#include <cstring>
#include <map>
#include <string>
#include <poll.h>
#include <vector>

using namespace std;
#define INPUT_FAIL INT16_MIN

int sockfd;

void error(string msg)
{
    perror(msg.c_str());
    exit(1);
}

void* wait_for_answer(void* time)
{
    int answer_time = *(int*)time;
    int answer;
    pollfd mypoll = { STDIN_FILENO, POLLIN|POLLPRI }; // переменная для ожидания записи в консоль

    if (poll(&mypoll, 1, answer_time * 1000)) // если за время answer_time * 1000 (5 * 1000 = 5 сек) получим запись в консоль
    {
        scanf("%i", &answer); // то прочитываем с консоли
    }
    else // иначе ставим на INPUT_FAIL
    {
        answer = INPUT_FAIL;
    }
    
    int* result = new int; // выделяем память чтобы вернуть значение
    *result = answer;
    return (void*)result; // возвращаем результат с помощью выделенной памяти
}

// Поток игры
void* game_thread(void*)
{
    while (true) // Бесконечный цикл ождиания начала игры
    {
        bool game_flag;
        read(sockfd, &game_flag, sizeof(bool)); // Принимаем флаг начала игры
        if (game_flag != true) error("Ошибка начала игры.\n"); // Сообщение если что-то не так
        
        while (true) // Цикл игры
        {
            bool game_end;
            read(sockfd, &game_end, sizeof(bool)); // Принимаем флаг продолжается игра или кончилась
            if (game_end) break; // если кончилась то выходим из цикла игры

            int think_time;
            char expression[20]; // текстовое выражение
            read(sockfd, &think_time, sizeof(int)); // прочитываем время на решение с сервера
            read(sockfd, expression, sizeof(char) * 20); // читаем выражение с сервера
            
            printf("\nНа размышление даётся %i секунд.\n", think_time);
            printf("%s = ", expression); // пишем выражение
            fflush(stdout); // чтобы курсор не переносился после принтф

            memset(expression, 0, sizeof(char) * 20); // сбрасываем выражение чтобы не выводил старое в консоль

            int* answer;
            pthread_t thr;
            clock_t start = clock(); // получаем время начала ввода ответа
            pthread_create(&thr, NULL, wait_for_answer, (void*)&think_time); // поток для получения ответа с консоли
            pthread_join(thr, (void**)&answer); // присоединяемся к нему
            clock_t end = clock(); // время конца ответа
            printf("\n");
            //int actual_answer = *answer;

            double time = (end - start) / CLOCKS_PER_SEC; // вычисляем время ответа
            write(sockfd, answer, sizeof(int)); // отсылаем ответ на сервер
            write(sockfd, &time, sizeof(double)); // отсылаем время ответа

            delete answer;

            int result;
            read(sockfd, &result, sizeof(int)); // читаем результат ответа с сервера
            if (result == 0) //если 0
            {
                printf("Вы ответили неправильно!\n");
            }
            else if (result == 1) // если 1
            {
                printf("Вы ответили правильно, но кто-то ответил быстрее!\n");
            }
            else if (result == 2) //если 2
            {
                printf("Вы победили! +1 победное очко\n");
            }
        }

        // Сюда мы попадём если выйдем с цикла игры. Выводим результаты игры.
        printf("Раунд закончился.\n\n Результаты:\n");
        int player_cnt;
        read(sockfd, &player_cnt, sizeof(int)); // получаем кол-во игроков
        for (int i = 0; i < player_cnt; i++) // цикл по кол-ву игрохов
        {
            int id;
            int score;
            read(sockfd, &id, sizeof(int)); // получаем id игрока
            read(sockfd, &score, sizeof(int)); // и очки игрока
            printf("%i Игрок: %i победных очков.\n", id, score); // выводим результат
        }
    }
}

void* commands_thread(void*)
{
    while (true)
    {

    }
}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "Russian");
    
    if (argc < 2)
    {
        error("Предоставьте адрес сервера!\n");
    }
    
    char* adress = argv[1];
    string ip_str;
    string port_str;
    int i = 0;
    for (i; i < strlen(adress); i++)
    {
        if (adress[i] == ':') break;
        ip_str += adress[i];
    }
    i++;
    for (i; i < strlen(adress); i++)
    {
        port_str += adress[i];
    }

    /* Создаём сокет */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("Error opening socket!");
    /* */

    /* Получаем сервер (хост) по ip */
    in_addr ip;
    hostent *server;
    if (!inet_aton(ip_str.c_str(), &ip)) error("Проверьте правильность адреса!\n");
    if ((server = gethostbyaddr((const void*)&ip, sizeof(ip), AF_INET)) == NULL) error("Не существует сервера по адресу " + ip_str + "\n");
    /* */
    
    /* Подключаемся к серверу по хосту */
    int port = stoi(port_str); // Приводим строку порта к численному виду
    sockaddr_in serv_addr;
    bzero((char*)&serv_addr, sizeof(serv_addr));
    bcopy((char*)server->h_addr_list[0], (char*)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) error("Ошибка подключения к серверу\n");
    /* */

    printf("Успешное подключение! Ожидание начала игры.\n");

    int id = 0;
    read(sockfd, &id, sizeof(int)); // получаем id от сервера
    printf("Ваше ID: %i", id);

    pthread_t thr1, thr2;
    pthread_create(&thr1, NULL, commands_thread, NULL);
    pthread_create(&thr2, NULL, game_thread, NULL);
    pthread_join(thr1, NULL);
}