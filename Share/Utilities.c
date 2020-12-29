//Authors - Tomer Shinhar 205627524 Yael schwartz 206335010
//Project - Queue

//Description - this modul contains general utilities to be used by the other moduels of the project

#include "utilities.h"


int check_malloc(const void* pointer) {
	if (!pointer)
	{
		printf("ERROR - malloc failed\n");
		return 1;
	}
	return 0;
}


int queue_to_int(Node* head) {
	// gets a queue of digits and retuens the number they represents 
	//given they are ordered from LSB to MSB
	int num = 0, position = 1;
	while (head != NULL) {
		num = num + position * top(head);
		head = pop(head); // using pop also frees the memory taken by the node
		position = position * 10;
	}
	return num;
}


Node* file_to_queue(char* file_path)
{	//gets path to file/file name and returns a queue 
	//with each node containing the value in the coresponding line
	FILE* file_pointer = NULL;
	fopen_s(&file_pointer, file_path, "r");
	if (file_pointer == 0)
	{
		printf("couldn't read file");
		exit(EXIT_FAILURE);
	}
	char current_char;
	int num_in_line, temp;
	Node* head = NULL;
	Node* line_queue_head = NULL;
	while ((current_char = (char)fgetc(file_pointer)) != EOF) {
		if (current_char != '\r' && current_char != '\n') {
			temp = current_char - '0'; // we know that the line contines digits only
			if (line_queue_head == NULL)
				line_queue_head = initialize_queue(temp);
			else
				line_queue_head = push_to_beginning(line_queue_head, temp);
		}
		else
			if (current_char == '\n') {
				num_in_line = queue_to_int(line_queue_head);//this will also free the line queue
				head = push(head, num_in_line);
				line_queue_head = NULL;
			}
	}
	fclose(file_pointer);
	return head;
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