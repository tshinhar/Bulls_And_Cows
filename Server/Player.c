#include "Player.h"


int RemoveGameSessionFile() {
	DWORD wait_res;
	wait_res = WaitForSingleObject(GameSessionMutex, INFINITE);
	if (WAIT_OBJECT_0 != wait_res)
	{
		printf("Couldn't wait for GameSessionMutex - %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	if (remove(GAME_SESSION_FILE_NAME) != 0) {
		printf("Unable to delete the game session file");
		return STATUS_CODE_FAILURE;
	}
	ReleaseMutex(GameSessionMutex);
	return STATUS_CODE_SUCCESS;
}


int PlayerDisconnect(Player* player) {
	DWORD wait_res;
	BOOL an_error_occured = FALSE;
	if (closesocket(player->player_socket) == SOCKET_ERROR) {
		printf("Failed to close player socket, error %ld. Ending program\n", WSAGetLastError());
		an_error_occured = TRUE;
	}
	wait_res = WaitForSingleObject(NumOfActivePlayersMutex, INFINITE);
	if (wait_res != WAIT_OBJECT_0) {
		printf("Couldn't wait for NumOfActivePlayersMutex - %d\n", GetLastError());
		an_error_occured = TRUE;
	}
	NumOfActivePlayers -= 1;
	player->valid = 0;
	if (ReleaseMutex(NumOfActivePlayersMutex) == FALSE) {
		printf("Couldn't releasr NumOfActivePlayersMutex - %d\n", GetLastError());
		an_error_occured = TRUE;
	}
	CloseHandle(player->turn_finished);
	if (an_error_occured)
		return STATUS_CODE_FAILURE;
	return STATUS_CODE_SUCCESS;
}


int NewUser(Player* player, char* username) {
	strcpy_s(player->name, USERNAME_MAX_LENGTH, username);

	player->turn_finished = NewEvent(NULL, AUTO_RESET);
	player->versus = FALSE;
	player->lost = FALSE;
	// return message to client
	if (SendString(SERVER_APPROVED, player->player_socket) == TRNS_FAILED)
	{
		printf("Socket error while trying to write data to socket - %d\n", WSAGetLastError());
		return STATUS_CODE_FAILURE;
	}
	if (SendString(SERVER_MAIN_MENU, player->player_socket) == TRNS_FAILED)
	{
		printf("Socket error while trying to write data to socket\n");
		return STATUS_CODE_FAILURE;
	}
	return STATUS_CODE_SUCCESS;
}


int PlayerSetup(Player* player, char* num_str) {
	player->chosen_num = atoi(num_str);
	// return message to client
	if (SendString(SERVER_APPROVED, player->player_socket) == TRNS_FAILED)
	{
		printf("Socket error when trying to send data - %d\n", WSAGetLastError());
		return STATUS_CODE_FAILURE;
	}
	if (SendString(SERVER_PLAYER_MOVE_REQUEST, player->player_socket) == TRNS_FAILED)
	{
		printf("Socket error when trying to send data\n");
		return STATUS_CODE_FAILURE;
	}
	return STATUS_CODE_SUCCESS;
}


int GetNumOfBulls(int *num, int *guess) {
	int bulls_counter = 0;
	for (int i = 0; i < 4; i++){
		if (num[i] == guess[i])
			bulls_counter++;
	}
	return bulls_counter;
}


int GetNumOfhits(int* num, int* guess) {
	int hits_counter = 0;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++)
			if (num[i] == guess[j])
				hits_counter++;
	}
	return hits_counter;
}


//Generate win message for other player
int CreateWinMessage(Player* player, Player* other_player, char** send_buffer) {
	int send_buffer_length;
	char player_num[5];
	sprintf_s(player_num, 4, "%d", player->chosen_num);
	player_num[4] = '\0';
	char winner_name[USERNAME_MAX_LENGTH];
	if (other_player->lost)
		strcpy_s(winner_name, USERNAME_MAX_LENGTH, player->name);
	else
		strcpy_s(winner_name, USERNAME_MAX_LENGTH, other_player->name);
	send_buffer_length = sizeof(SERVER_GAME_RESULTS) + 2 * USERNAME_MAX_LENGTH + 2 * MOVE_MAX_LENGTH + 1;
	*send_buffer = (char*)malloc(send_buffer_length * sizeof(char));
	if (check_malloc(*send_buffer) == EXIT_FAILURE) {
		return STATUS_CODE_FAILURE;
	}
	if (*send_buffer != 0) { // this is always true but just in case
		strcpy_s(*send_buffer, send_buffer_length, SERVER_WIN);
		strcat_s(*send_buffer, send_buffer_length, winner_name);
		strcat_s(*send_buffer, send_buffer_length, ";");
		strcat_s(*send_buffer, send_buffer_length, player_num);
	}
	return STATUS_CODE_SUCCESS;
}


//Generate results message
int CreateResultsMessage(Player* player, int guess, char* name, char** send_buffer) {
	int send_buffer_length;
	int num_arry[] = { 0, 0, 0, 0 };
	int guess_arry[] = { 0, 0, 0, 0 };
	char num_of_hits[2] = "\0";
	num_of_hits[0] = GetNumOfBulls(num_to_arry(player->chosen_num, num_arry), num_to_arry(guess, guess_arry)) + '0';
	char num_of_bulls[2] = "\0";
	num_of_bulls[0] = GetNumOfBulls(num_arry, guess_arry) + '0';
	if (num_of_bulls[0] == '4')// if 4 bulls then other player won
		player->lost = TRUE;
	send_buffer_length = sizeof(SERVER_GAME_RESULTS) + 2 * USERNAME_MAX_LENGTH + 2 * MOVE_MAX_LENGTH + 1;
	*send_buffer = (char*)malloc(send_buffer_length * sizeof(char));
	if (check_malloc(*send_buffer) == EXIT_FAILURE) {
		return STATUS_CODE_FAILURE;
	}
	if (*send_buffer != 0) { // this is always true but just in case
		strcpy_s(*send_buffer, send_buffer_length, SERVER_GAME_RESULTS);
		strcat_s(*send_buffer, send_buffer_length, num_of_bulls);
		strcat_s(*send_buffer, send_buffer_length, ";");
		strcat_s(*send_buffer, send_buffer_length, num_of_hits);
		strcat_s(*send_buffer, send_buffer_length, ";");
		strcat_s(*send_buffer, send_buffer_length, name);
		strcat_s(*send_buffer, send_buffer_length, ";");
		strcat_s(*send_buffer, send_buffer_length, name);
	}
	return STATUS_CODE_SUCCESS;
}


int PlayMoveVesus(Player* player, Player* other_player) {
	DWORD wait_res;
	BOOL opend_file = FALSE;
	char* game_results_buffer = NULL;
	errno_t err;
	FILE* game_session_file_p;
	int oponent_guess;
	char oponent_name[USERNAME_MAX_LENGTH];
	char guess_read_buffer[MOVE_MAX_LENGTH];

	wait_res = WaitForSingleObject(GameSessionMutex, INFINITE);
	if (WAIT_OBJECT_0 != wait_res)
	{
		printf("Couldn't wait for GameSessionMutex - %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	err = fopen_s(&game_session_file_p, GAME_SESSION_FILE_NAME, "r");
	if (err != 0) {
		printf("Error opening file\n");
		return STATUS_CODE_FAILURE;
	}
	fseek(game_session_file_p, 0, SEEK_END); // goto end of file
	if (ftell(game_session_file_p) != 0) { //file not empty, read from file	
		fseek(game_session_file_p, 0, SEEK_SET); // goto begin of file
		fgets(oponent_name, USERNAME_MAX_LENGTH, game_session_file_p); // first line is other player's name
		fgets(guess_read_buffer, MOVE_MAX_LENGTH, game_session_file_p);
		oponent_guess = atoi(guess_read_buffer);
	}
	fclose(game_session_file_p);
	// write move to file
	err = fopen_s(&game_session_file_p, GAME_SESSION_FILE_NAME, "w");
	if (err != 0 || game_session_file_p == NULL) {
		printf("Error opening file\n");
		return STATUS_CODE_FAILURE;
	}
	if (0 > fprintf_s(game_session_file_p, "%s\n", player->name))
	{
		printf("Failed to write to file.\n");
		return STATUS_CODE_FAILURE;
	}
	if (0 > fprintf_s(game_session_file_p, "%d", player->guess))
	{
		printf("Failed to write to file.\n");
		return STATUS_CODE_FAILURE;
	}
	fclose(game_session_file_p);
	ReleaseMutex(GameSessionMutex);
	if (FALSE == SetEvent(player->turn_finished)) {
		printf("Couldn't set Turn_Finished event - %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	// wait for other player to finish his turn
	wait_res = WaitForSingleObject(other_player->turn_finished, INFINITE);
	if ((WAIT_OBJECT_0 != wait_res)) {
		printf("Couldn't wait for Turn_Finished event - %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	// both players are done - calculate result results
	if (CreateResultsMessage(player, oponent_guess, oponent_name, &game_results_buffer) == STATUS_CODE_FAILURE) {
		return STATUS_CODE_FAILURE;
	}
	SendString(game_results_buffer, player->player_socket);
	free(game_results_buffer);
	game_results_buffer = NULL;
	if (!(player->lost) && !(other_player->lost)) {//no winner - play again
		if (SendString(SERVER_PLAYER_MOVE_REQUEST, player->player_socket) == TRNS_FAILED) {
			printf("Socket error when trying to send data\n");
			return STATUS_CODE_FAILURE;
		}
	}
	else if ((player->lost) && (other_player->lost)) {// draw
		if (SendString(SERVER_DRAW, player->player_socket) == TRNS_FAILED) {
			printf("Socket error when trying to send data\n");
			return STATUS_CODE_FAILURE;
		}
		RemoveGameSessionFile();
	}
	else { // someone won
		if (CreateWinMessage(player, other_player, &game_results_buffer) == STATUS_CODE_FAILURE) {
			return STATUS_CODE_FAILURE;
		}
		RemoveGameSessionFile();
	}
	free(game_results_buffer);
	return STATUS_CODE_SUCCESS;
}
	

int PlayVersus(Player* player, Player* other_player, BOOL* create_game_session) {
	DWORD wait_res;
	errno_t err;
	FILE* game_session_file_p;

	player->versus = TRUE;
	// tell other client we want to play
	if (FALSE == SetEvent(player->versus_chose)) {
		printf("Couldn't set player game type event - %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}

	// wait for other client to choose to play
	wait_res = WaitForSingleObject(other_player->versus_chose, 15000);
	if ((WAIT_OBJECT_0 != wait_res) && (WAIT_TIMEOUT != wait_res)) {
		printf("Couldn't get other player's choice - %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	if (WAIT_TIMEOUT == wait_res) { // the other player won't play
		if (SendString(SERVER_NO_OPPONENTS, player->player_socket) == TRNS_FAILED)
		{
			printf("Socket error trying to send data\n");
			return STATUS_CODE_FAILURE;
		}
		if (SendString(SERVER_MAIN_MENU, player->player_socket) == TRNS_FAILED)
		{
			printf("Socket error trying to send data\n");
			return STATUS_CODE_FAILURE;
		}
		return STATUS_CODE_SUCCESS;
	}
	else {	// the other player wants to play
		wait_res = WaitForSingleObject(GameSessionMutex, INFINITE);
		if (WAIT_OBJECT_0 != wait_res)
		{
			printf("Error waiting for GameSessionMutex - %d\n", GetLastError());
			return STATUS_CODE_FAILURE;
		}
		*create_game_session = FALSE;
		if (!IsFileExist(GAME_SESSION_FILE_NAME)) {	// create GameSession file if not exist
			err = fopen_s(&game_session_file_p, GAME_SESSION_FILE_NAME, "w");
			if (err != 0 || game_session_file_p == NULL) {
				printf("Unable to create file.\n");
				return STATUS_CODE_FAILURE;
			}
			fclose(game_session_file_p);
			*create_game_session = TRUE;
		}
		ReleaseMutex(GameSessionMutex);

		if (SendString(SERVER_SETUP_REQUEST, player->player_socket) == TRNS_FAILED)
		{
			printf("Socket error trying to send data\n");
			return STATUS_CODE_FAILURE;
		}
	}
	return STATUS_CODE_SUCCESS;
}
