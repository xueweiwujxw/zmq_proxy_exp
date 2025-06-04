#!/bin/bash
echo "[mem_check] check file $1"
echo "[mem_check] ./$1 start"
echo "[mem_check] valgrind --log-file=valgrind.log --tool=memcheck --leak-check=full --show-leak-kinds=all ./$1 ${@:2:$#-1}"
valgrind --log-file=valgrind.log --tool=memcheck --leak-check=full --show-leak-kinds=all ./"$1" "${@:2:$#-1}"
echo "[mem_check] check finish"
echo "[mem_check] see result: ./valgrind.log"
