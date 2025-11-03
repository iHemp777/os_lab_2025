#!/bin/bash

gcc -c revert_string.c -o revert_string.o
ar rcs librevert.a revert_string.o
gcc main.c librevert.a -o program_static


gcc -c -fPIC revert_string.c -o revert_string_pic.o
gcc -shared -o librevert.so revert_string_pic.o
gcc main.c -L. -lrevert -o program_dynamic


