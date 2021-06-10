#include <string>

enum class Operands
{
    Plus = 0, Minus = 1, Multiply = 2, Divide = 3
};

enum class Difficulty
{
    Easy = 1, Medium = 2, Hard = 3
};

int get_digits(int number)
{
    int i = 0;
    for (i; number > 0; i++) number /= 10;
    return i;
}

// Получить математическое выражение, его результат, его сложность
std::string get_expression(int& result, Difficulty& diff)
{
    Operands operand = Operands(rand() % 4); // Рандомим оператор от 0 до 3 включая (знак)
    int operand1, operand2;
    int digits = 0; // Количество цифр
    std::string res; // Текстовый результат
    switch (operand)
    {
    case Operands::Plus: // Получили плюс
        operand1 = rand() % 1000; // первый операнд от 0 до 1000 (левое число)
        operand2 = rand() % 100; // второй от 0 до 100 (правое число)
        result = operand1 + operand2; // получаем числовой результат
        res = std::to_string(operand1) + " + " + std::to_string(operand2); // создаём строку из выражения to_string переводит число в строку
        digits += get_digits(operand1); // разбиваем число, разделяя его на 10, пока больше ноля. Таким образом находим кол-во цифр
        digits += get_digits(operand2); // то же самое
        if (digits = 7) diff = Difficulty::Medium; // если кол-во цифр 7, то сложность средняя
        else if (digits == 6) diff = Difficulty::Medium; // 6 - средняя
        else diff = Difficulty::Easy; // остальное лёшкое

        return res;
    
    case Operands::Minus: // Получили минус. То же самое как с плюсом, только в строке минус и результат вычитания
        operand1 = rand() % 1000;
        operand2 = rand() % 1000;
        result = operand1 - operand2;
        res = std::to_string(operand1) + " - " + std::to_string(operand2); 
        digits += get_digits(operand1); // разбиваем число, разделяя его на 10, пока больше ноля. Таким образом находим кол-во цифр
        digits += get_digits(operand2); // то же самое
        if (digits >= 7) diff = Difficulty::Hard; // высокая
        else if (digits >= 5) diff = Difficulty::Medium;
        else diff = Difficulty::Easy;
       
        return res;

    case Operands::Multiply: // Умножение
        operand1 = 2 + rand() % 10; // от 2 до 10
        operand2 = 2 + rand() % 10; // от 2 до 10
        result = operand1 * operand2;
        res = std::to_string(operand1) + " * " + std::to_string(operand2);
        digits += get_digits(operand1); // разбиваем число, разделяя его на 10, пока больше ноля. Таким образом находим кол-во цифр
        digits += get_digits(operand2); // то же самое
        if (digits == 4) diff = Difficulty::Medium; // если 4 то норм
        else Difficulty::Easy;
       
        return res;

    case Operands::Divide: // Деление
        operand1 = 2 + rand() % 100;
        operand2 = 2 + rand() % 10;
        while (operand2 > operand1) operand2 = 2 + rand() % 10; // Если операнд 2 больше операнда 1 (например 3 / 7), то перерандомим, т.к. будет получаться 0 
        result = operand1 / operand2;
        res = std::to_string(operand1) + " / " + std::to_string(operand2);
        digits += get_digits(operand1); // разбиваем число, разделяя его на 10, пока больше ноля. Таким образом находим кол-во цифр
        digits += get_digits(operand2); // то же самое
        if (digits > 4) diff = Difficulty::Hard;
        else if (digits == 4) diff = Difficulty::Medium;
        else diff = Difficulty::Easy;

        return res;
    
    default:
        break;
    }
}