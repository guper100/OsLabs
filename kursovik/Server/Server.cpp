#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <map>
#include <threads.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <vector>
#include <ctime>
#include "Math.cpp"
#define SCORE_INF -1
const int THINK_TIME = 5;

using namespace std;
int sockfd; // Сокет сервера
int log_level = 1;

// Данные об игроке. Его сокет, ID, очки игры, ответ, время ответа, был ли ответ верным.
struct Player
{
    int fd;
    int id;
    int score = 0;
    int answer;
    double time;
    bool is_right;
};

vector<Player> players;

void* game_thread(void*);
void* server_commands(void*);
int refresh_answers();
int refresh_round();

void error(string msg)
{
    perror(msg.c_str());
    exit(1);
}

void log_message(string message)
{
    if (log_level == 0) return; // если лог == 0 то выходим (ничего не выводим)
    char cbuff[message.size()];
    strcpy(cbuff, message.c_str());
    printf("%s\n", cbuff);
    if (log_level == 1) return; // если лог == 1 то выходим (вывели в консоль)
    FILE* file; // Тут получится что выведем и в консоль и в файл
    file = fopen("log", "w+");
    if (file != NULL)
    {
        time_t seconds = time(NULL);
        tm* timeinfo = localtime(&seconds);
        fputs(asctime(timeinfo), file);
        fputs(cbuff, file);
        fflush(file);
    }
}

// Начинает игру
void start_game(int max_score)
{
    if (players.size() > 0)
    {
        pthread_t game_thr;
        pthread_create(&game_thr, NULL, game_thread, (void*)&max_score); // Запускаем поток игры с аргументом max_score
    }
}

// Поток игры
void* game_thread(void* max_scr)
{
    srand(time(NULL)); // Сид рандома

    int possible_max_score = *(int*)max_scr; // приводим аргумент void* к инт. Это максимально возможные очки
    int players_count = players.size(); // кол-во игроков
    int player_max_score = 0; // текущее макс кол-во очков игроков

    for (int i = 0; i < players_count; i++)
    {
        bool game_flag = true;
        write(players[i].fd, &game_flag, sizeof(bool)); // Отправляем флаг старта игры
    }

    log_message("Игра началась! Макс очков " + to_string(possible_max_score) + "\n\n");

    while (player_max_score != possible_max_score) // Цикл раунда пока макс. очки игрока не достигнут макс. возможных очков (например играем до 10 победных)
    {
        bool game_end = false;
        for (int i = 0; i < players.size(); i++) // Отправляем флаг того что игра продолжается
        {
            write(players[i].fd, &game_end, sizeof(bool));
        }

        int answer = 0;
        Difficulty diff = Difficulty::Easy;
        string expression = get_expression(answer, diff); // Получаем выражение, ответ, сложность, передав последние по ссылке в функцию
        char expbuf[expression.size()]; // Переводим string выражение в C буфер
        strcpy(expbuf, expression.c_str()); // Копирум string в C буфер

        const int think_time = THINK_TIME * (int)diff; // базовое ожидание умножаем на полученную сложность (1 легко, 2 средне, 3 сложно)
        for (int i = 0; i < players_count; i++)
        {
            write(players[i].fd, &think_time, sizeof(int)); // отправляем всем игрокам время на решение
            write(players[i].fd, expbuf, strlen(expbuf) * sizeof(char)); // и само выражение
        }

        sleep(think_time); // ждём время на решение

        int winner_fd;
        int right_answ_cnt = 0;
        for (int i = 0; i < players_count; i++) // заполняем ответы игроков
        {
            int answ;
            double time;
            read(players[i].fd, &answ, sizeof(int)); // получаем ответ
            read(players[i].fd, &time, sizeof(double)); // получаем время ответа

            players[i].answer = answ; // ставим ответ игрока на полученный
            players[i].time = time; // время игрока
            if (answ == answer) // если ответ игрока равен правильному
            {
                players[i].is_right = true; // ставим ему статус правильности
                winner_fd = players[i].fd; // ставим fd победного игрока как этого (если он будет единственным победным)
                right_answ_cnt++; // счётчик правильных ответов увеличиваем
            }
            log_message("Игрок " + to_string(players[i].id) + " ответ: " + to_string(answ) + " | время: " + to_string(time));
        }
        
        if (right_answ_cnt > 1) // если кол-во правильный ответов больше 1, то будет определять по минимальному времени ответа
        {
            winner_fd = 0; // сбрасываем победного игрока
            double best_time = INT16_MAX; // инициализируем переменную лучшего времени большим числом
            for (int i = 0; i < players.size(); i++) // проверяем всех игроков
            {
                if (players[i].is_right && players[i].time < best_time) // если ответ игрока был правильным и его время меньше текущего лучшего
                {
                    best_time = players[i].time; // обновляем лучшее время
                    winner_fd = players[i].fd; // обновляем fd победного игрока
                }
            }
        }

        for (int i = 0; i < players_count; i++) // отсылаем результаты игрокам
        {
            int status = 0; // статус ответа игрока. 0 - неправильный, 1 - правильный но проиграл по времени, 2 - победил
            if (players[i].is_right && players[i].fd == winner_fd)  // если игрок был прав и его fd == победному fd
            {
                players[i].score++; // увеличиваем его очки
                if (players[i].score > player_max_score) player_max_score = players[i].score; // обновляем максимальное значение очков. если достигнет possible_max_score, то выйдет из цикла игры
                status = 2; // ставим статус на 2 (победный)
            }
            else if (players[i].is_right && right_answ_cnt > 1) // если не прошло предыдущее условие на победность, но он всё равно дал правильный ответ 
            {
                status = 1; // ставим статус на 1
            } // Если игрок был не прав то статус так и остаётся в 0
            
            write(players[i].fd, &status, sizeof(int)); // отправляем игроку его статус
        }

        for (int i = 0; i < players_count; i++) // Выводим сообщение
        {
            log_message("Игрок " + to_string(players[i].id) + " очки: " + to_string(players[i].score) + "\n");
        }

        log_message("Текущее максимальное очко: " + to_string(player_max_score) + "\n");
        refresh_answers(); // обновляем данные ответов игроков
    }

    bool game_end = true; // Отправляем флаг конца игры
    for (int i = 0; i < players.size(); i++) // Цикл для всех пользователей
    {
        write(players[i].fd, &game_end, sizeof(bool)); // отсылаем флаг
        write(players[i].fd, &players_count, sizeof(int)); // отправляем количество игроков чтобы клиент вывел информацию о каждом
        for (int j = 0; j < players.size(); j++) // Цикл в цикле. Отправим всем пользователям инфу а каждом пользователе (его id и очки)
        {
            int id = players[j].id;
            int score = players[j].score;
            write(players[i].fd, &id, sizeof(int)); // отправляем id
            write(players[i].fd, &score, sizeof(int)); // отправляем очки
        }
    }
    refresh_round(); // обновляем очки
}

void* server_commands(void*)
{
    while (true)
    {
        char command[16];
        scanf("%s", command);
        if (strcmp(command, "/start") == 0) // начинает игру
        {
            memset(command, 0, 16); // сбрасываем буфер
            scanf("%s", command);

            int max_score;
            if (strcmp(command, "inf") == 0) // бесконечно кол-во очков
            {
                max_score = SCORE_INF;
            }
            else 
            {
                max_score = atoi(command); // в численный вид
                if (max_score <= 0) // если написал 0 или меньше
                {
                    printf("Количество победных очков должно быть больше 0!\n");
                    continue;
                }
            }
            
            start_game(max_score); // начинаем игру
        }
        else if (strcmp(command, "/disconnect") == 0) // отключить пользователя
        {
            memset(command, 0, 16); // сбрасываем буфер
            scanf("%s", command);

            int client_id;
            client_id = atoi(command); // id пользователя в число
            int client_fd = -1;
            int index;

            for (int i = 0; i < players.size(); i++) // ищем среди пользователей пользователя с нужным id
            {
                if (players[i].id == client_id)
                {
                    index = i;
                    client_fd = players[i].fd; // запоминаем его сокет если нашли
                    break;
                }
            }
            if (client_fd != -1) // Если нашли
            {
                close(client_fd); // Закрываем сокет
                players.erase(players.begin() + index); // удаляем игрока
                printf("Разорвано соединение с клентом %i\n", client_id);
            }
            else // Если не нашли
            {
                printf("Такого id клиента не существует!\n");
            }
        }
        else if (strcmp(command, "/loglevel") == 0) // Поставить уровень вывода
        {
            memset(command, 0, 16); // сброс буфера
            scanf("%s", command);
            log_level = atoi(command); // перевод в число

            printf("Уровень вывода успешно поставлен на %i\n", log_level);
        }
    }
}

// Поток получения клиентов
void* get_clients(void*)
{
    socklen_t clilen;
    sockaddr_in cli_addr; // адрес клиента
    clilen = sizeof(cli_addr);

    int client_id = 1;
    while (true)
    {
        int client_fd = accept(sockfd, (sockaddr*)&cli_addr, &clilen); // Принимаем клиентов
        if (client_fd < 0) error("Ошибка при подключении клиента!\n"); // Если ошибка
        printf("Подключился игрок с ID %i\n", client_id);

        Player new_player{client_fd, client_id, 0}; // создаём нового игрока
        players.push_back(new_player); // заносим в вектор players
        write(client_fd, &client_id, sizeof(int)); // отправляем игроку его id

        client_id++; // увеличиваем ID
    }
}

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "Russian");
    int port;
    if (argc >= 2) // если аргументы есть
    {
        port = atoi(argv[1]); // строку порта в число
    }
    else
    {
        error("Укажите порт!\n");
    }
    
    /* Создаём сокет */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("Ошибка создания сокета!\n");
    /* */
    
    /* Создаём соединение */
    sockaddr_in server_adr;
    bzero((char*)&server_adr, sizeof(server_adr));
    server_adr.sin_addr.s_addr = INADDR_ANY;
    server_adr.sin_family = AF_INET;
    server_adr.sin_port = htons(port);
    if (bind(sockfd, (sockaddr*)&server_adr, sizeof(server_adr)) < 0) error("Ошибка создания соединения\n!");
    /* */

    if (listen(sockfd, 5) < 0) error("Ошибка на listen()!\n"); // даём доступ на подключение клиентов

    printf("Сервер запущен на %i порте\n", port);
    
    pthread_t thr1, thr2;
    pthread_create(&thr1, NULL, get_clients, NULL); // поток получения клиентов
    pthread_create(&thr2, NULL, server_commands, NULL); // поток команд
    pthread_join(thr2, NULL); // присоединение к потоку

}

int refresh_answers()
{
    for (int i = 0; i < players.size(); i++)
    {
        players[i].answer = 0;
        players[i].time = 0;
        players[i].is_right = false;
    }
}

int refresh_round()
{
    refresh_answers();
    for (int i = 0; i < players.size(); i++)
    {
        players[i].score = 0;
    }
}

bool max_score_check(int max_score)
{
    for (int i = 0; i < players.size(); i++)
    {
        if (players[i].score >= max_score) return true;
    }
    return false;
}