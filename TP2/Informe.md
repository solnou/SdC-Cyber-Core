nasm -f elf64 convert_to_ints.asm -o convert_to_ints.o
gcc main.c convert_to_ints.o -lcurl -lcjson -o gini_sum
