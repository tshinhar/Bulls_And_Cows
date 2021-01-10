#ifndef UTILITIES_H
#define UTILITIES_H


#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <stdbool.h>


#define STATUS_CODE_FAILURE -1
#define STATUS_CODE_SUCCESS 0

int check_malloc(const void* pointer);
HANDLE create_file(char* file_path, char mode);
void close_handles_of_threads(thread_handles, num_of_threads);
int check_realloc(const void* pointer);
int* num_to_arry(int num, int* arry);
BOOL IsFileExist(char* filename);
HANDLE NewEvent(const char EVENT_NAME[], BOOL manualReset);


#endif // UTILITIES_H