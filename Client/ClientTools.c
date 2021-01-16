#include "ClientTools.h"



int Check_what_to_do_next()
{
	char message[USERNAME_MAX_LENGTH] = "";
	printf("Choose what to do next:\n");
	printf("1. Try to reconnect\n");
	printf("2. Exit\n");
	gets_s(message, sizeof(message)); //Reading a string from the keyboard
	switch (atoi(message)) {
	case (1):
		return again;
	case (2):
		return endofwork;
	default: // in case of an illegal command
		printf("not a permitted value!");
		return notValid;
	}
}

int check_if_SendReceiveString(TransferResult_t ReceiveResult)
{
	if (ReceiveResult == TRNS_FAILED || ReceiveResult == TRNS_DISCONNECTED)
	{
		return TRUE;
	}
	return FALSE;
}

int str_prefix(char* str, const char* prefix)
{
	return (strncmp(prefix, str, strlen(prefix)) == 0);
}

int Server_ReceiveString(char* ptr) {
	if (str_prefix(ptr, "SERVER_MAIN_MENU") == 1)
		return SERVER_MAIN_MENU;
	if (str_prefix(ptr, "SERVER_APPROVED") == 1)
		return SERVER_APPROVED;
	if (str_prefix(ptr, "SERVER_DENIED") == 1)
		return SERVER_DENIED;
	if (str_prefix(ptr, "SERVER_INVITE") == 1)
		return SERVER_INVITE;
	if (str_prefix(ptr, "SERVER_SETUP_REQUEST") == 1)
		return SERVER_SETUP_REQUEST;
	if (str_prefix(ptr, "SERVER_PLAYER_MOVE_REQUEST") == 1)
		return SERVER_PLAYER_MOVE_REQUEST;
	if (str_prefix(ptr, "SERVER_GAME_RESULTS") == 1)
		return SERVER_GAME_RESULTS;
	if (str_prefix(ptr, "SERVER_WIN") == 1)
		return SERVER_WIN;
	if (str_prefix(ptr, "SERVER_DRAW") == 1)
		return SERVER_DRAW;
	if (str_prefix(ptr, "SERVER_NO_OPPONENTS") == 1)
		return SERVER_NO_OPPONENTS;
	return SERVER_OPPONENT_QUIT;
}
int Server_connection_issue(client* user)
{
	printf("Failed connecting to server on %s:%d.\n", user->ip_address, user->port_name);
	ReleaseSemaphore(TimeOut_connectionProblem_GoOut_Semphore, 1, NULL);
	return 0x555;
}

int Client_disconnected(char message[LONGEST_MESSAGE], SOCKET m_socket)
{
	TimeTogoOut = 1;
	strcpy_s(message, LONGEST_MESSAGE, "CLIENT_DISCONNECT\n");
	TransferResult_t SendResult = SendString(message, m_socket);
	ReleaseSemaphore(TimeOut_connectionProblem_GoOut_Semphore, 1, NULL);
	return 0x555;
}

int CreateAllSemphores() {
	TimeOut_connectionProblem_GoOut_Semphore = CreateSemaphore(NULL, 0, 1, NULL);
	if (NULL == TimeOut_connectionProblem_GoOut_Semphore)
	{
		CloseHandle(SERVER_APROVED_Semphore);
		printf("Error when creating semaphore: %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	SERVER_MAIN_MENU_Semphore = CreateSemaphore(NULL, 0, 1, NULL);
	if (NULL == SERVER_MAIN_MENU_Semphore)
	{
		CloseHandle(TimeOut_connectionProblem_GoOut_Semphore);
		CloseHandle(SERVER_APROVED_Semphore);
		printf("Error when creating semaphore: %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	SERVER_APROVED_Semphore = CreateSemaphore(NULL, 0, 1, NULL);
	if (NULL == SERVER_APROVED_Semphore)
	{
		printf("Error when creating semaphore: %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	SERVER_INVITE_Semphore = CreateSemaphore(NULL, 0, 1, NULL);
	if (NULL == SERVER_INVITE_Semphore)
	{
		CloseHandle(TimeOut_connectionProblem_GoOut_Semphore);
		CloseHandle(SERVER_APROVED_Semphore);
		CloseHandle(SERVER_MAIN_MENU_Semphore);
		printf("Error when creating semaphore: %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	SERVER_SETUP_REUEST_Semphore = CreateSemaphore(NULL, 0, 1, NULL);
	if (NULL == SERVER_SETUP_REUEST_Semphore)
	{
		CloseHandle(TimeOut_connectionProblem_GoOut_Semphore);
		CloseHandle(SERVER_APROVED_Semphore);
		CloseHandle(SERVER_MAIN_MENU_Semphore);
		CloseHandle(SERVER_INVITE_Semphore);
		printf("Error when creating semaphore: %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	SERVER_PLAYER_MOVE_REQUEST_Semphore = CreateSemaphore(NULL, 0, 1, NULL);
	if (NULL == SERVER_PLAYER_MOVE_REQUEST_Semphore)
	{
		CloseHandle(TimeOut_connectionProblem_GoOut_Semphore);
		CloseHandle(SERVER_APROVED_Semphore);
		CloseHandle(SERVER_MAIN_MENU_Semphore);
		CloseHandle(SERVER_INVITE_Semphore);
		CloseHandle(SERVER_SETUP_REUEST_Semphore);
		printf("Error when creating semaphore: %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	SERVER_GAME_RESULTS_Semphore = CreateSemaphore(NULL, 0, 1, NULL);
	if (NULL == SERVER_GAME_RESULTS_Semphore)
	{
		CloseHandle(TimeOut_connectionProblem_GoOut_Semphore);
		CloseHandle(SERVER_APROVED_Semphore);
		CloseHandle(SERVER_MAIN_MENU_Semphore);
		CloseHandle(SERVER_INVITE_Semphore);
		CloseHandle(SERVER_SETUP_REUEST_Semphore);
		CloseHandle(SERVER_PLAYER_MOVE_REQUEST_Semphore);
		printf("Error when creating semaphore: %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	SERVER_WIN_Semphore = CreateSemaphore(NULL, 0, 1, NULL);
	if (NULL == SERVER_WIN_Semphore)
	{
		CloseHandle(TimeOut_connectionProblem_GoOut_Semphore);
		CloseHandle(SERVER_APROVED_Semphore);
		CloseHandle(SERVER_MAIN_MENU_Semphore);
		CloseHandle(SERVER_INVITE_Semphore);
		CloseHandle(SERVER_SETUP_REUEST_Semphore);
		CloseHandle(SERVER_PLAYER_MOVE_REQUEST_Semphore);
		CloseHandle(SERVER_GAME_RESULTS_Semphore);
		printf("Error when creating semaphore: %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	SERVER_DRAW_Semphore = CreateSemaphore(NULL, 0, 1, NULL);
	if (NULL == SERVER_DRAW_Semphore)
	{
		CloseHandle(TimeOut_connectionProblem_GoOut_Semphore);
		CloseHandle(SERVER_APROVED_Semphore);
		CloseHandle(SERVER_MAIN_MENU_Semphore);
		CloseHandle(SERVER_INVITE_Semphore);
		CloseHandle(SERVER_SETUP_REUEST_Semphore);
		CloseHandle(SERVER_PLAYER_MOVE_REQUEST_Semphore);
		CloseHandle(SERVER_GAME_RESULTS_Semphore);
		CloseHandle(SERVER_WIN_Semphore);
		printf("Error when creating semaphore: %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	SERVER_OPPONENT_QUIT_semaphore = CreateSemaphore(NULL, 0, 1, NULL);
	if (NULL == SERVER_OPPONENT_QUIT_semaphore)
	{
		CloseHandle(TimeOut_connectionProblem_GoOut_Semphore);
		CloseHandle(SERVER_APROVED_Semphore);
		CloseHandle(SERVER_MAIN_MENU_Semphore);
		CloseHandle(SERVER_INVITE_Semphore);
		CloseHandle(SERVER_SETUP_REUEST_Semphore);
		CloseHandle(SERVER_PLAYER_MOVE_REQUEST_Semphore);
		CloseHandle(SERVER_GAME_RESULTS_Semphore);
		CloseHandle(SERVER_WIN_Semphore);
		CloseHandle(SERVER_DRAW_Semphore);
		printf("Error when creating semaphore: %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	return STATUS_CODE_SUCCESS;
}

void closeAllHandle()
{
	CloseHandle(TimeOut_connectionProblem_GoOut_Semphore);
	CloseHandle(SERVER_MAIN_MENU_Semphore);
	CloseHandle(SERVER_APROVED_Semphore);
	CloseHandle(SERVER_INVITE_Semphore);
	CloseHandle(SERVER_SETUP_REUEST_Semphore);
	CloseHandle(SERVER_PLAYER_MOVE_REQUEST_Semphore);
	CloseHandle(SERVER_GAME_RESULTS_Semphore);
	CloseHandle(SERVER_WIN_Semphore);
	CloseHandle(SERVER_DRAW_Semphore);
	CloseHandle(SERVER_OPPONENT_QUIT_semaphore);
}