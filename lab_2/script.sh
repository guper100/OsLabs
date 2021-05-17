#!bin/bash

rm -f main.out

g++ main.cpp -o main.out
./main.out
echo "exit code = $?"