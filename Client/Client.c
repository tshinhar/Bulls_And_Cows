#include "Client.h"


TimeTogoOut = 0;
ServerOpponentsQuit = 0;
ServerNoOpponemts = 0;
game_next_step = SERVER_PLAYER_MOVE_REQUEST;

int main(int argc, char** argv) {
	SOCKADDR_IN client_connection_Service; //Specifies a transport address and port for the AF_INET address family
	HANDLE hThread[2];
	WSADATA wsaData; //Create a WSADATA object called wsaData, which contains information about the Windows Sockets implementation
	DWORD wait_result;
	int check;
	client player;

	if (argc < 4) {
		printf("Not enough arguments.\n");
		return STATUS_CODE_FAILURE;
	}
	//Create player (IP address, port, username)
	strcpy_s(player.ip_address, USERNAME_MAX_LENGTH_IP_ADRESS, argv[1]);
	sscanf_s(argv[2], "%d", &player.port_name);
	strcpy_s(player.username, USERNAME_MAX_LENGTH, argv[3]);
	//Create the Semphores
	if (CreateAllSemphores() == STATUS_CODE_FAILURE)
		return STATUS_CODE_FAILURE;
	while (1) {
		//Call WSAStartup and check for errors
		int initialize_windows_socket_Result = WSAStartup(MAKEWORD(2, 2), &wsaData); // Initialize Winsock as seen on recitation 10
		if (initialize_windows_socket_Result != NO_ERROR)
			printf("There was an error at WSAStartup()\n");
		//Create a socket
		m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//We use the internet address family, streaming sockets, and the TCP/IP protocol
		//Check for errors to ensure that the socket is indeed a valid socket
		if (m_socket == INVALID_SOCKET) {
			printf("There was an error with socket(): %ld\n", WSAGetLastError());
			WSACleanup();
			return;
		}
		//Create a SOCKADDR_IN object named "client_connection_Service" and set its values
		client_connection_Service.sin_family = AF_INET;
		client_connection_Service.sin_addr.s_addr = inet_addr(player.ip_address);
		client_connection_Service.sin_port = htons(player.port_name);
		//Call the connect function, and check for general errors
		if (connect(m_socket, (SOCKADDR*)&client_connection_Service, sizeof(client_connection_Service)) == SOCKET_ERROR) {
			printf("Failed connecting to server on %s:%d\n", player.ip_address, player.port_name);
			check = Check_what_to_do_next();
			while (check == notValid) {
				check = Check_what_to_do_next();
			}
			if (check == endofwork) {
				WSACleanup();
				closesocket(m_socket);
				closeAllHandle();
				return STATUS_CODE_SUCCESS;
			}
			continue;//Getting here means that the player wants to try to reconnect again (The continue jumps back to to the while(1))
		}
		printf("Connected to server on %s:%d\n", player.ip_address, player.port_name);

		//Send and receive data
		hThread[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendDataThread, &player, 0, NULL);
		if (NULL == hThread[0])//case 4 in the explicit user's program information (page 8)
		{
			printf("Couldn't create SendDataThread\n");
			printf("Failed connecting to server on %s:%d.\n", player.ip_address, player.port_name);
			ReleaseSemaphore(TimeOut_connectionProblem_GoOut_Semphore, 1, NULL);
		}
		hThread[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReceiveDataThread, &player, 0, NULL);
		if (NULL == hThread[1])//case 4 in the explicit user's program information (page 8)
		{
			printf("Couldn't create RecieveDataThread\n");
			printf("Failed connecting to server on %s:%d.\n", player.ip_address, player.port_name);
			ReleaseSemaphore(TimeOut_connectionProblem_GoOut_Semphore, 1, NULL);
		}
		wait_result = WaitForMultipleObjects(2, hThread, TRUE, INFINITE);// wait for both of the threads to finish
		if ((WAIT_OBJECT_0 != wait_result)) {
			printf("SendDataThread or ReceiveDataThread (or both) occured an issue (timeout/failed/abandoned)\n");
		}

		wait_result = WaitForSingleObject(TimeOut_connectionProblem_GoOut_Semphore, INFINITE);
		if ((WAIT_OBJECT_0 != wait_result)) {
			printf("Couldn't wait for end of TimeOut_connectionProblem_GoOut_Semphore\n");
		}
		if (hThread[0] != 0) {
			TerminateThread(hThread[0], 0x555);//****** 1365: ERROR_BAD_LOGON_SESSION_STATE, The logon session is not in a state that is consistent with the requested operation ????????
			CloseHandle(hThread[0]);
		}
		if (hThread[1] != 0) {
			TerminateThread(hThread[1], 0x555);//****** 1365: ERROR_BAD_LOGON_SESSION_STATE, The logon session is not in a state that is consistent with the requested operation ????????
			CloseHandle(hThread[1]);
		}
		closesocket(m_socket);
		WSACleanup();
		if (TimeTogoOut) {//The player wants to disconnect
			closeAllHandle();
			return STATUS_CODE_SUCCESS;
		}
		//Timeout or connection issues
		check = Check_what_to_do_next();
		while (check == notValid) {
			check = Check_what_to_do_next();
		}
		if (check == endofwork) {
			closeAllHandle();
			return STATUS_CODE_SUCCESS;
		}
	}
}

//Sending data to the server
static DWORD SendDataThread(LPVOID lpParam) {
	char message[LONGEST_MESSAGE], SendStr[USERNAME_MAX_LENGTH];
	TransferResult_t SendResult;
	DWORD wait_result;
	client* user = (client*)lpParam;
	int main_menu = 0;
	HANDLE reuslt_sempahores[3], client_quit_or_invited[2];
	strcpy_s(message, LONGEST_MESSAGE, "CLIENT_REQUEST:");
	strcat_s(message, LONGEST_MESSAGE, user->username);
	strcat_s(message, LONGEST_MESSAGE, "\n");
	SendResult = SendString(message, m_socket);
	if (check_if_SendReceiveString(SendResult)) {
		ReleaseSemaphore(TimeOut_connectionProblem_GoOut_Semphore, 1, NULL);
		return 0x555;
	}
	wait_result = WaitForSingleObject(SERVER_APROVED_Semphore, WAIT_15S);
	if ((WAIT_OBJECT_0 != wait_result)) {//case 4 in the explicit user's program information (page 8)//
		return (Server_connection_issue(user));
	}
	while (TRUE) {
		// getting response from server of SERVER_MAIN_MENU
		wait_result = WaitForSingleObject(SERVER_MAIN_MENU_Semphore, WAIT_30S);// 30 since 15 for user and 15 for server response to the user (page 8 note number 7). ****should it be 30 seconds here or on SERVER_INVITE_Semphore?
		if ((WAIT_OBJECT_0 != wait_result)) {//case 4 in the explicit user's program information (page 8)
			return (Server_connection_issue(user));
		}
		printf("Choose what to do next:\n");
		printf("1. Play against another client\n");
		printf("2. Quit\n");
		gets_s(SendStr, sizeof(SendStr)); //Reading a string from SERVER_MAIN_MENU: 1- Play against another client, 2- Quit
		if (atoi(SendStr) == 1) {//Play against another client
			//CLIENT_VERSUS
			strcpy_s(message, LONGEST_MESSAGE, "CLIENT_VERSUS\n");
			SendResult = SendString(message, m_socket);
			if (check_if_SendReceiveString(SendResult)) {
				ReleaseSemaphore(TimeOut_connectionProblem_GoOut_Semphore, 1, NULL);
				return 0x555;
			}
			// getting response from server of SERVER_NO_OPPONENTS or SERVER_INVITE (either there is no other client or it wants to join)
			client_quit_or_invited[0] = SERVER_OPPONENT_QUIT_semaphore;
			client_quit_or_invited[1] = SERVER_INVITE_Semphore;
			wait_result = WaitForMultipleObjects(2, client_quit_or_invited, FALSE, WAIT_30S);
			if ((WAIT_OBJECT_0 + 1 != wait_result) && (WAIT_OBJECT_0 != wait_result)) {//case 4 in the explicit user's program information (page 8)
				printf("Couldn't wait for SERVER_INVITE Semphore or SERVER_OPPONENT_QUIT semaphore - %d\n", GetLastError());
				return (Server_connection_issue(user));
			}
			if (ServerNoOpponemts) {
				ServerNoOpponemts = 0;
				continue;
			}
			//getting response from server of SERVER_SETUP_REUEST
			wait_result = WaitForSingleObject(SERVER_SETUP_REUEST_Semphore, WAIT_15S);
			if ((WAIT_OBJECT_0 != wait_result)) {//case 4 in the explicit user's program information (page 8)
				printf("Couldn't wait for server_setup_semaphore - %d\n", GetLastError());
				return (Server_connection_issue(user));
			}
			if (ServerOpponentsQuit) {
				ServerOpponentsQuit = 0;
				continue;
			}
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
			while (TRUE) {
				//checking if we need to wait for SERVER_PLAYER_MOVE_REQUEST or if the game has ended
				reuslt_sempahores[0] = SERVER_WIN_Semphore;
				reuslt_sempahores[1] = SERVER_DRAW_Semphore;
				reuslt_sempahores[2] = SERVER_PLAYER_MOVE_REQUEST_Semphore;
				wait_result = WaitForMultipleObjects(3, reuslt_sempahores, FALSE, WAIT_15S);
				//if ((WAIT_OBJECT_0 != wait_result)) {//case 4 in the explicit user's program information (page 8)
				//	return (Server_connection_issue(user));
				//}
				if (game_next_step == SERVER_WIN || game_next_step == SERVER_DRAW)//the game has ended
					break;
				//getting here means we got SERVER_PLAYER_MOVE_REQUEST
				if (ServerOpponentsQuit)
					break;
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
				wait_result = WaitForSingleObject(SERVER_GAME_RESULTS_Semphore, INFINITE);
				if ((WAIT_OBJECT_0 != wait_result)) {//case 4 in the explicit user's program information (page 8)
					return (Server_connection_issue(user));
				}
			}
		}
		else //Quit
		{//CLIENT_DISCONNECT
			return Client_disconnected(message, m_socket);
		}
	}
}

//Reading data from the server
static DWORD ReceiveDataThread(LPVOID lpParam)
{
	TransferResult_t ReceiveResult;
	client* user = (client*)lpParam;
	enum server val;
	char* pointer = NULL, * next_pointer = NULL, * bulls = NULL, * cows = NULL, * opponent_move = NULL, * opponent_name = NULL, * winner = NULL, * opponent_number = NULL;
	while (1)
	{
		char* RecievedStr = NULL;
		ReceiveResult = ReceiveString(&RecievedStr, m_socket);
		if (check_if_SendReceiveString(ReceiveResult))
		{
			ReleaseSemaphore(TimeOut_connectionProblem_GoOut_Semphore, 1, NULL);
			return 0x555;
		}
		pointer = strtok_s(RecievedStr, ":;", &next_pointer);
		val = Server_ReceiveString(pointer);
		switch (val) {
		case SERVER_MAIN_MENU:
			ReleaseSemaphore(SERVER_MAIN_MENU_Semphore, 1, NULL);
			free(RecievedStr);
			break;
		case SERVER_APPROVED:
			ReleaseSemaphore(SERVER_APROVED_Semphore, 1, NULL);
			free(RecievedStr);
			break;
		case SERVER_DENIED:
			printf("Server on %s:%d denied the connection request.\n", user->ip_address, user->port_name);
			ReleaseSemaphore(TimeOut_connectionProblem_GoOut_Semphore, 1, NULL);
			free(RecievedStr);
			return 0x555;
		case SERVER_INVITE:
			printf("Game is on!\n");
			ReleaseSemaphore(SERVER_INVITE_Semphore, 1, NULL);
			free(RecievedStr);
			break;
		case SERVER_SETUP_REQUEST:
			printf("Choose your 4 digits:\n");
			ReleaseSemaphore(SERVER_SETUP_REUEST_Semphore, 1, NULL);
			free(RecievedStr);
			break;
		case SERVER_PLAYER_MOVE_REQUEST:
			printf("Choose your guess:\n");
			game_next_step = SERVER_PLAYER_MOVE_REQUEST;
			ReleaseSemaphore(SERVER_PLAYER_MOVE_REQUEST_Semphore, 1, NULL);
			free(RecievedStr);
			break;
		case SERVER_GAME_RESULTS:
			bulls = strtok_s(NULL, ":;", &next_pointer);
			cows = strtok_s(NULL, ":;", &next_pointer);
			opponent_name = strtok_s(NULL, ":;", &next_pointer);
			opponent_move = strtok_s(NULL, ":;", &next_pointer);
			printf("Bulls: %s\n", bulls);
			printf("Cows: %s\n", cows);
			printf("%s played: %s\n", opponent_name, opponent_move);
			ReleaseSemaphore(SERVER_GAME_RESULTS_Semphore, 1, NULL);
			free(RecievedStr);
			break;
		case SERVER_WIN:
			game_next_step = SERVER_WIN;
			winner = strtok_s(NULL, ":;", &next_pointer);
			opponent_number = strtok_s(NULL, ":;", &next_pointer);
			printf("%s won!\n", winner);
			printf("opponents number was: %s!\n", opponent_number);
			ReleaseSemaphore(SERVER_WIN_Semphore, 1, NULL);
			free(RecievedStr);
			break;
		case SERVER_DRAW:
			game_next_step = SERVER_DRAW;
			printf("It%cs a tie\n", '\'');
			ReleaseSemaphore(SERVER_DRAW_Semphore, 1, NULL);
			free(RecievedStr);
			break;
		case SERVER_NO_OPPONENTS:
			ServerNoOpponemts = 1;
			ReleaseSemaphore(SERVER_INVITE_Semphore, 1, NULL);
			free(RecievedStr);
			break;
		case SERVER_OPPONENT_QUIT:
			ServerOpponentsQuit = 1;
			printf("Opponent quit.\n");
			ReleaseSemaphore(SERVER_OPPONENT_QUIT_semaphore, 1, NULL);
			free(RecievedStr);
			break;
		default:
			free(RecievedStr);
			return 0x555;
		}
	}
	return 0;
}