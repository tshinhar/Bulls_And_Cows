//Authors - Tomer Shinhar 205627524 Yael schwartz 206335010
//Project - Queue

//Description - this modul contains general utilities to be used by the other moduels of the project

#include "utilities.h"


int check_malloc(const void* pointer) {
	if (!pointer)
	{
		printf("ERROR - malloc failed\n");
		return EXIT_FAILURE;
	}
	return 0;
}


HANDLE create_file(char* file_path, char mode) {
	// this function creates a file and returns the handle, mode is indicating read or write
	HANDLE* hFile;
	if (mode == 'r') {
		hFile = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	else
		if (mode == 'w') {
			hFile = CreateFileA(file_path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		}
		else
			if (mode == 'a') {
				hFile = CreateFileA(file_path, FILE_APPEND_DATA, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			}
			else {
				printf("ERROR: not 'r', 'a' or 'w' for file");
				return NULL;
			}

	// Check for error
	if (hFile == INVALID_HANDLE_VALUE) {
		printf("Target file not created. Error %u", GetLastError());
		return NULL;
	}
	return hFile;
}


void close_handles_of_threads(HANDLE* thread_handles, int num_of_threads)
{
	for (int i = 0; i < num_of_threads; i++)
	{
		printf("closing thread %d\n", i);
		CloseHandle(thread_handles[i]);
	}
}


int check_realloc(const void* pointer)
{
	if (!pointer)
	{
		printf("ERROR - malloc failed\n");
		return 1;
	}
	return 0;
}


int* num_to_arry(int num, int* arry) {
	for (int i = 3; i >= 0; i--)
	{
		arry[i] = num % (10 * i);
		num = num / 10;
	}
	return arry;
}


BOOL IsFileExist(char* filename) {
	FILE* file_check_p;
	errno_t err;
	err = fopen_s(&file_check_p, filename, "r");
	if (err != 0) {	// file not exist
		return FALSE;
	}
	else
		if(file_check_p != 0)// don't close there is no file (this should not happen anyway, just in case)
			fclose(file_check_p);
	return TRUE;
}


int CreateFileIfNotExist(char* filename) {
	// try to open file to read
	FILE* file_create_p;
	errno_t err;
	if (!IsFileExist(filename))	// file not exist
	{
		err = fopen_s(&file_create_p, filename, "w");	// create file
		if (err != 0) {
			printf("Unable to create file.\n");
			return STATUS_CODE_FAILURE;
		}
		if (file_create_p != 0)// this is always true here, but just in case
			fclose(file_create_p);
	}
	return STATUS_CODE_SUCCESS;
}


//create new non-signaled event or return event handler if event with this name already exist
HANDLE NewEvent(const char EVENT_NAME[], BOOL manualReset)
{
	HANDLE event_handle;
	DWORD last_error;

	// create manual-reset event, initial state is non-signaled 
	event_handle = CreateEvent(NULL, manualReset, FALSE, EVENT_NAME);
	if (NULL == event_handle)
	{
		printf("Couldn't create next day event\n");
		return NULL;
	}

	last_error = GetLastError();
	if ((last_error != ERROR_SUCCESS) && (last_error != ERROR_ALREADY_EXISTS))
	{
		return NULL;
	}

	return event_handle;
}