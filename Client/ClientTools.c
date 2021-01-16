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
	} // End of switch 
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
/* seperate each While loop in SendDataThread to different functions when everything already runs smoothly
int Start_a_game(client* user)
{
	DWORD wait_result;
	char message[LONGEST_MESSAGE], SendStr[USERNAME_MAX_LENGTH];
	TransferResult_t SendResult;
	while (1)
	{
		// getting response from server of SERVER_INVITE
		wait_result = WaitForSingleObject(SERVER_INVITE_Semphore, WAIT_15S);
		if ((WAIT_OBJECT_0 != wait_result)) {//case 4 in the explicit user's program information (page 8)

			return (Server_connection_issue(user));
		}
		if (ServerNoOpponemts) {
			ServerNoOpponemts = 0;
			return 0;
		}
		//getting response from server of SERVER_SETUP_REUEST
		wait_result = WaitForSingleObject(SERVER_SETUP_REUEST_Semphore, WAIT_15S);
		if ((WAIT_OBJECT_0 != wait_result)) {//case 4 in the explicit user's program information (page 8)
			return (Server_connection_issue(user));
		}
		if (ServerOpponentsQuit)
			return 0;
		//CLIENT_SETUP
		gets_s(SendStr, sizeof(SendStr));//Reading 4 numbers from the player
		strcpy_s(message, LONGEST_MESSAGE, "CLIENT_SETUP:");
		SendStr[4] = '\0';//since the number the player chooses has 4 digits
		strcat_s(message, LONGEST_MESSAGE, SendStr);
		strcat_s(message, LONGEST_MESSAGE, "\n");
		SendResult = SendString(message, m_socket);
		if (check_if_SendReceiveString(SendResult)) {
			ReleaseSemaphore(TimeOut_connectionProblem_GoOut_Semphore, 1, NULL);
			return 0x555;
		}
		if (Game_progress(user) == 0x555)
			return 0x555;
		else return 0;
	}
}

int Game_progress(client* user)
{
	DWORD wait_result;
	char message[LONGEST_MESSAGE], SendStr[USERNAME_MAX_LENGTH];
	TransferResult_t SendResult;
	HANDLE reuslt_sempahores[2];
	while (1)
	{
		//getting response from server of SERVER_PLAYER_MOVE_REQUEST
		wait_result = WaitForSingleObject(SERVER_PLAYER_MOVE_REQUEST_Semphore, WAIT_15S);
		if ((WAIT_OBJECT_0 != wait_result)) {//case 4 in the explicit user's program information (page 8)
			return (Server_connection_issue(user));
		}
		if (ServerOpponentsQuit)
			return 0;
		//CLIENT_PLAYER_MOVE
		gets_s(SendStr, sizeof(SendStr)); //Reading a string (the 4 digits the user guessed) from SERVER_PLAYER_MOVE_REQUEST
		strcpy_s(message, LONGEST_MESSAGE, "CLIENT_PLAYER_MOVE:");
		strcat_s(message, LONGEST_MESSAGE, SendStr);
		strcat_s(message, LONGEST_MESSAGE, "\n");
		SendResult = SendString(message, m_socket);
		if (check_if_SendReceiveString(SendResult)) {
			ReleaseSemaphore(TimeOut_connectionProblem_GoOut_Semphore, 1, NULL);
			return 0x555;
		}
		//getting response from server of SERVER_GAME_RESULTS
		wait_result = WaitForSingleObject(SERVER_GAME_RESULTS_Semphore, WAIT_15S);
		if ((WAIT_OBJECT_0 != wait_result)) {//case 4 in the explicit user's program information (page 8)
			return (Server_connection_issue(user));
		}
		//getting response from server of SERVER_WIN and SERVER_DRAW
		reuslt_sempahores[0] = SERVER_WIN_Semphore;
		reuslt_sempahores[1] = SERVER_DRAW_Semphore;
		wait_result = WaitForMultipleObjects(2, reuslt_sempahores, FALSE, WAIT_15S);
		if ((WAIT_OBJECT_0 != wait_result)) {//case 4 in the explicit user's program information (page 8)
			return (Server_connection_issue(user));
		}
		//getting response from server of SERVER_MAIN_MENU
		wait_result = WaitForSingleObject(SERVER_MAIN_MENU_Semphore, WAIT_30S);// 30 since 15 for user and 15 for server response to the user |(page 8 note number 7). ****should it be 30 seconds here or on SERVER_INVITE_Semphore?
		if ((WAIT_OBJECT_0 != wait_result)) {//case 4 in the explicit user's program information (page 8)
			return (Server_connection_issue(user));
		}
		gets_s(SendStr, sizeof(SendStr)); //Reading a string from SERVER_GAME_OVER_MENU: 1- Play against another client, 2- Quit
		if (atoi(SendStr) == 1) { //Play against another client{
			// CLIENT_VERSUS
			strcpy_s(message, LONGEST_MESSAGE, "CLIENT_VERSUS\n");
			SendResult = SendString(message, m_socket);
			if (check_if_SendReceiveString(SendResult)) {
				ReleaseSemaphore(TimeOut_connectionProblem_GoOut_Semphore, 1, NULL);
				return 0x555;
			}
		}
		else { //Quit
		//CLIENT_DISCONNECT
			return Client_disconnected(message, m_socket);
		}
	}
}
*/
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
	QUIT_OPPONENT_SERVER_semaphore = CreateSemaphore(NULL, 0, 1, NULL);
	if (NULL == QUIT_OPPONENT_SERVER_semaphore)
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
	CloseHandle(QUIT_OPPONENT_SERVER_semaphore);
}