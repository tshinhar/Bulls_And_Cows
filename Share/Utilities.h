#ifndef UTILITIES_H
#define UTILITIES_H


#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <stdbool.h>


int check_malloc(const void* pointer);
HANDLE create_file(char* file_path, char mode);
void close_handles_of_threads(thread_handles, num_of_threads);
int check_realloc(const void* pointer);
int* num_to_arry(int num)


#endif // UTILITIES_H