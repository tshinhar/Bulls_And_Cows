#include "Server.h"


int main(int argc, char* argv[])
{
	SOCKET server_socket = INVALID_SOCKET;
	DWORD wait_res;
	HANDLE server_threads[2];

	if (argc < 2) {
		printf("Not enough arguments.\n");
		return STATUS_CODE_FAILURE;
	}

	// initial variables
	ClientIndex = 0;
	NumOfActivePlayers = 0;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		Players[i].player_socket = INVALID_SOCKET;
		Players[i].valid = 0;
	}

	GameSessionMutex = CreateMutex(NULL, FALSE, NULL);
	if (GameSessionMutex == NULL) {
		printf("Couldn't Create mutex for broadcast. Error code: %ld\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}

	if (InitServerSocket(&server_socket, atoi(argv[1])) == STATUS_CODE_FAILURE) {
		goto Main_Cleanup_Mutex;
	}

	server_threads[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WaitForConnection, (void*)&server_socket, 0, NULL);
	server_threads[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ExitInteruptThread, NULL, 0, NULL);
	wait_res = WaitForMultipleObjects(2, server_threads, FALSE, INFINITE);// wait for one of the threads to finish
	if (WAIT_OBJECT_0 + 1 != wait_res) {// if WAIT_OBJECT_0 + 1 then ExitInteruptThread has returned so the user wrote "exit"
		printf("Couldn't wait for main thread and exit thread. Error code: %ld\n", GetLastError());
		if (server_threads[1] != NULL) {// this is always true here but just in case
			CloseHandle(server_threads[1]);
		}
	}

	if (Players[0].valid == 1)
		PlayerDisconnect(&Players[0]);
	if (Players[1].valid == 1)
		PlayerDisconnect(&Players[1]);
	if (server_threads[0] != NULL) {// this is always true here but just in case
		CloseHandle(server_threads[0]);
	}
Main_Cleanup_Mutex:
	CloseHandle(GameSessionMutex);
	return STATUS_CODE_SUCCESS;
}
