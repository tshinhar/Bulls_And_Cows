//Authors – Tomer Shinhar 205627524 Yael schwartz 206335010
//Project – Server

//Description - this modul contains all the functions for running the server 
//and reciving messages from the clients

#include "Connection.h"



//Check if str starts with a given prefix
int str_prefix(char* str, const char* prefix)
{
	return (strncmp(prefix, str, strlen(prefix)) == 0);
}


//Extract params from str and save them in dst
int ExtraceParams(char* str, char** dst) {
	char* colon_ptr = strchr(str, ':');
	int dst_length;
	if (colon_ptr == NULL) {
		printf("Can't find colon in message %s\n", str);
		return STATUS_CODE_FAILURE;
	}
	colon_ptr += 1;
	dst_length = (int)strlen(colon_ptr);
	colon_ptr[dst_length - 1] = '\0';		// delete \n in the end
	*dst = (char*)malloc(dst_length * sizeof(char));
	if (check_malloc(*dst) == EXIT_FAILURE) {
		return STATUS_CODE_FAILURE;
	}
	strcpy_s(*dst, dst_length, colon_ptr);
	return STATUS_CODE_SUCCESS;
}


int InitServerSocket(SOCKET* server_socket, int port_num) {
	// Initialize Winsock
	WSADATA wsa_data;
	unsigned long address;
	SOCKADDR_IN service;
	int status;

	status = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (status != 0) {
		printf("WSAStartup failed: %d\n", status);
		return STATUS_CODE_FAILURE;
	}
	// Create a socket 
	*server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*server_socket == INVALID_SOCKET){
		printf("Error at socket( ): %ld\n", WSAGetLastError());
		DeinitializeSocket(NULL);
		return STATUS_CODE_FAILURE;
	}


	address = inet_addr(SERVER_ADDRESS);
	if (address == INADDR_NONE){
		printf("The string \"%s\" cannot be converted into an ip address. ending program.\n",
			SERVER_ADDRESS);
		DeinitializeSocket(server_socket);
		return STATUS_CODE_FAILURE;
	}

	service.sin_family = AF_INET;
	service.sin_addr.s_addr = address;
	service.sin_port = htons(port_num);

	//bind
	status = bind(*server_socket, (SOCKADDR*)&service, sizeof(service));
	if (status == SOCKET_ERROR){
		printf("bind( ) failed with error %ld. Ending program\n", WSAGetLastError());
		DeinitializeSocket(server_socket);
		return STATUS_CODE_FAILURE;
	}

	// Listen on the Socket
	status = listen(*server_socket, SOMAXCONN);
	if (status == SOCKET_ERROR) {
		printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
		DeinitializeSocket(server_socket);
		return STATUS_CODE_FAILURE;
	}
	return STATUS_CODE_SUCCESS;
}


//closes the server socket
int DeinitializeSocket(SOCKET* socket) {
	int result;

	if (*socket != INVALID_SOCKET) {
		if (closesocket(*socket) == SOCKET_ERROR)
			printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());
		return STATUS_CODE_FAILURE;
	}

	result = WSACleanup();
	if (result != 0) {
		printf("WSACleanup failed: %d\n", result);
		return STATUS_CODE_FAILURE;
	}
	return STATUS_CODE_SUCCESS;
}



//thread for managing recieved data from player
DWORD RecvDataThread(int player_index) {
	TransferResult_t recv_res;
	Player* player = &Players[player_index];
	Player* other_player = &Players[1 - player_index];
	char* params;
	FILE* game_session_file = NULL;
	BOOL an_error_occured = FALSE;
	char* recv_buffer = NULL;
	BOOL game_session_created = FALSE;

	while (TRUE) {
		// ReceiveBuffer contain call for recv that is blocking function
		recv_buffer = NULL;
		recv_res = ReceiveString(&recv_buffer, player->player_socket);
		if (recv_res == TRNS_SUCCEEDED) {
			if (str_prefix(recv_buffer, CLIENT_REQUEST)) {// if we get here then connection was approved
				// save client user name
				if (ExtraceParams(recv_buffer, &params) == STATUS_CODE_FAILURE) {
					an_error_occured = TRUE;
					goto Cleanup_1;
				}
				if (STATUS_CODE_FAILURE == NewUser(player, params)) {
					free(params);
					an_error_occured = TRUE;
					goto Cleanup_1;
				}
				free(params);
			}
			else if (str_prefix(recv_buffer, CLIENT_SETUP)) {
				if (ExtraceParams(recv_buffer, &params) == STATUS_CODE_FAILURE) {
					an_error_occured = TRUE;
					goto Cleanup_1;
				}
				if (STATUS_CODE_FAILURE == PlayerSetup(player, params)) {
					free(params);
					an_error_occured = TRUE;
					goto Cleanup_1;
				}
				free(params);
			}
			else if (str_prefix(recv_buffer, CLIENT_VERSUS)) {
				if (STATUS_CODE_FAILURE == PlayVersus(player, other_player, &game_session_created)) {
					an_error_occured = TRUE;
					goto Cleanup_1;
				}
			}
			else if (str_prefix(recv_buffer, CLIENT_PLAYER_MOVE)) {
				if (ExtraceParams(recv_buffer, &params) == STATUS_CODE_FAILURE) {
					an_error_occured = TRUE;
					goto Cleanup_1;
				}
				// update player guess
				player->guess = atoi(params);
				printf("recived player guess %d\n", player->guess);
				if (STATUS_CODE_FAILURE == PlayMoveVesus(player, other_player)) {
					an_error_occured = TRUE;
					goto Cleanup_1;
				}
			}
			else if (str_prefix(recv_buffer, CLIENT_DISCONNECT)) {
				goto Cleanup_1;
			}
			else {
				printf("Wrong message from client");
				goto Cleanup_1;
			}
			free(recv_buffer);
		}
		else {
			an_error_occured = TRUE;
			goto Cleanup_1;
		}
	}
Cleanup_1:
	free(recv_buffer);
	if (STATUS_CODE_FAILURE == PlayerDisconnect(player)) {
		an_error_occured = TRUE;
		printf("Error disconnecting player\n");
	}
	if (IsFileExist(GAME_SESSION_FILE_NAME)) {	// remove file if it exists as player disconected
		RemoveGameSessionFile();
	}
	printf("player disconnected\n");
	if (an_error_occured) {
		return STATUS_CODE_FAILURE;
	}
	return STATUS_CODE_SUCCESS;
}


// Thread for incoming connection requests
DWORD WaitForConnection(LPVOID lpParam) {
	SOCKET refusal_socket = INVALID_SOCKET;
	SOCKET accept_socket = INVALID_SOCKET;
	SOCKET* main_socket = (SOCKET*)lpParam;
	DWORD wait_res;
	BOOL an_error_occured = FALSE;

	NumOfActivePlayers = 0;
	NumOfActivePlayersMutex = CreateMutex(NULL, FALSE, NULL);
	if (NumOfActivePlayersMutex == NULL) { an_error_occured = TRUE; goto Cleanup_1; }
	while (TRUE)
	{
		accept_socket = accept(*main_socket, NULL, NULL);
		if (accept_socket == INVALID_SOCKET) {
			printf("Accepting a new connection from client failed, error %ld\n", WSAGetLastError());
			an_error_occured = TRUE;
			goto Cleanup_2;
		}
		//Accept a request if MAX_CLIENTS was not reached
		wait_res = WaitForSingleObject(NumOfActivePlayersMutex, INFINITE);
		if (wait_res != WAIT_OBJECT_0) {
			an_error_occured = TRUE;
			goto Cleanup_2;
		}

		if (NumOfActivePlayers >= MAX_CLIENTS) {
			if (ReleaseMutex(NumOfActivePlayersMutex) == FALSE) {
				an_error_occured = TRUE;
				goto Cleanup_2;
			}
			printf("Max allowed clients are connected.Refusing a client.\n");
			refusal_socket = accept_socket;
			SendString(SERVER_DENIED, refusal_socket);
			TransferResult_t recv_res;
			char* recv_buffer = NULL;
			recv_res = ReceiveString(&recv_buffer, refusal_socket);// because client should send CLIENT_REQUEST also in this situation
			if (recv_res != TRNS_SUCCEEDED) {
				printf("Error reciving message\n");
				an_error_occured = TRUE;
				goto Cleanup_2;
			}
			closesocket(refusal_socket);
			refusal_socket = INVALID_SOCKET;
			continue;
		}
		else {
			// update number of active players
			NumOfActivePlayers += 1;
			if (ReleaseMutex(NumOfActivePlayersMutex) == FALSE) {
				printf("Couldn't release NumOfActivePlayersMutex\n");
				an_error_occured = TRUE;
				goto Cleanup_2;
			}
			// check which player slot is availble
			if (Players[0].valid == 0)
				ClientIndex = 0;
			else
				ClientIndex = 1;
			Players[ClientIndex].player_socket = accept_socket;
			Players[ClientIndex].valid = 1;
			// create a thread for the player
			players_threads[ClientIndex] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RecvDataThread, (LPVOID)ClientIndex, 0, NULL);
			if (NULL == players_threads[ClientIndex])
			{
				printf("Couldn't create thread\n");
				an_error_occured = TRUE;
				goto Cleanup_2;
			}
		}
	}
	wait_res = WaitForMultipleObjects(MAX_CLIENTS, players_threads, TRUE, INFINITE);
	if (WAIT_OBJECT_0 != wait_res)
	{
		printf("Couldn't wait for send/recv threads - %d\n", GetLastError());
		an_error_occured = TRUE;
	}

Cleanup_2:
	closesocket(accept_socket);
	if(players_threads[0] != 0) // so we won't close a closed handle
		CloseHandle(players_threads[0]);
	if (players_threads[1] != 0) // so we won't close a closed handle
		CloseHandle(players_threads[1]);
Cleanup_1:
	if (NumOfActivePlayersMutex != 0) // so we won't close a closed handle
		CloseHandle(NumOfActivePlayersMutex);
	if (an_error_occured) return STATUS_CODE_FAILURE;
	return STATUS_CODE_SUCCESS;
}



//thread that waits for 'exit' to be written on the server command line
DWORD ExitInteruptThread(LPVOID lpParam) {
	char str[5];
	scanf_s("%s", str, 5);
	if (str[4] == '\0') {// since the input is string this is always true, but just in case
		while (strcmp(str, "exit") != 0) {
			scanf_s("%s", str, 5);
			printf("%s\n", str);
		}
	}
	return 0;
}