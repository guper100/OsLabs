#!bin/bash
mkdir 9091
cd 9091
mkdir Kudryavtsev
date > Vladislav
date --date="next monday" > filedate.txt
cat Vladislav filedate.txt > result.txt
cat result.txt