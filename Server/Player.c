//Authors – Tomer Shinhar 205627524 Yael schwartz 206335010
//Project – Server

//Description - this modul contains all the functions for sending data to the clients 
//and calculating the game results 

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
		printf("Unable to delete the game session file\n");
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


// init a new player
int NewUser(Player* player, char* username) {
	strcpy_s(player->name, USERNAME_MAX_LENGTH, username);

	player->turn_finished = NewEvent(NULL, AUTO_RESET);
	player->versus_chose = NewEvent(NULL, AUTO_RESET);
	player->calc_finished = NewEvent(NULL, AUTO_RESET);
	player->versus = FALSE;
	player->lost = FALSE;
	player->wrote_to_file = FALSE;
	printf("user %s created\n", username);
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

// set the number choosen by the player
int PlayerSetup(Player* player, char* num_str) {
	player->chosen_num = atoi(num_str);
	player->lost = FALSE; // we reset this flag for a new game
	printf("user chose a number and it was set\n");
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
	printf("move request sent\n");
	return STATUS_CODE_SUCCESS;
}


//calculates the number of bulls in the guess
int GetNumOfBulls(int *num, int *guess) {
	int bulls_counter = 0;
	for (int i = 0; i < 4; i++){
		if (num[i] == guess[i])
			bulls_counter++;
	}
	return bulls_counter;
}


//calculate the number of hits in the guess
int GetNumOfHits(int* num, int* guess) {
	int hits_counter = 0;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++)
			if (j != i) {//if i=j this is a bull
				if (num[i] == guess[j])
					hits_counter++;
			}
	}
	return hits_counter;
}


//Generate win message for other player
int CreateWinMessage(Player* player, Player* other_player, char** send_buffer) {
	int send_buffer_length;
	char player_num[sizeof(int) * 4 + 1];// for converting int to string
	sprintf_s(player_num, sizeof(int) * 4 + 1, "%d", player->chosen_num);
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
	int name_len = strlen(name);
	if (name[name_len - 1] == '\n')
		name[name_len - 1] = '\0';
	char guess_str[sizeof(int) * 4 + 1];// for converting int to string
	sprintf_s(guess_str, sizeof(int) * 4 + 1, "%d", player->guess);
	int send_buffer_length;
	int num_arry[] = { 0, 0, 0, 0 };
	num_to_arry(player->chosen_num, num_arry);
	int guess_arry[] = { 0, 0, 0, 0 };
	num_to_arry(guess, guess_arry);
	char num_of_hits[2] = "\0";
	num_of_hits[0] = GetNumOfHits(num_arry, guess_arry) + '0';
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
		strcat_s(*send_buffer, send_buffer_length, player->name);
		strcat_s(*send_buffer, send_buffer_length, ";");
		strcat_s(*send_buffer, send_buffer_length, guess_str);
	}
	return STATUS_CODE_SUCCESS;
}


int SendResults(Player* player, Player* other_player) {
	char* game_results_buffer = NULL;
	BOOL main_menu = TRUE;
	if (!(player->lost) && !(other_player->lost)) {//no winner - play again
		main_menu = FALSE;
		printf("no winner, starting another round\n");
		if (SendString(SERVER_PLAYER_MOVE_REQUEST, player->player_socket) == TRNS_FAILED) {
			printf("Socket error when trying to send data\n");
			return STATUS_CODE_FAILURE;
		}
		printf("move request sent\n");
	}
	else if ((player->lost) && (other_player->lost)) {// draw
		printf("draw\n");
		if (SendString(SERVER_DRAW, player->player_socket) == TRNS_FAILED) {
			printf("Socket error when trying to send data\n");
			return STATUS_CODE_FAILURE;
		}
		if(!IsFileExist(GAME_SESSION_FILE_NAME))
			RemoveGameSessionFile();
	}
	else { // someone won
		printf("winner found\n");
		if (CreateWinMessage(player, other_player, &game_results_buffer) == STATUS_CODE_FAILURE) {
			return STATUS_CODE_FAILURE;
		}
		if (SendString(game_results_buffer, other_player->player_socket) == TRNS_FAILED) {
			printf("Socket error when trying to send data\n");
			return STATUS_CODE_FAILURE;
		}
	}
	if (main_menu) {// game is over, send main menu and reset the game
		printf("returning to main menu\n");
		if (!IsFileExist(GAME_SESSION_FILE_NAME))
			RemoveGameSessionFile();
		if (SendString(SERVER_MAIN_MENU, player->player_socket) == TRNS_FAILED) {
			printf("Socket error when trying to send data\n");
			return STATUS_CODE_FAILURE;
		}
	}
	free(game_results_buffer);
	return STATUS_CODE_SUCCESS;
}


// does a round of the game, waits for both players to make a guess and sends back the results
int PlayMoveVesus(Player* player, Player* other_player) {
	DWORD wait_res;
	BOOL read_from_file = FALSE;
	char* game_results_buffer = NULL;
	errno_t err;
	FILE* game_session_file_p;
	int oponent_guess = 0;
	char oponent_name[USERNAME_MAX_LENGTH];
	char guess_read_buffer[MOVE_MAX_LENGTH];
	player->wrote_to_file = FALSE;

	wait_res = WaitForSingleObject(GameSessionMutex, INFINITE); //infinite as this waits for user to play his move
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
	if (other_player->wrote_to_file == TRUE) { //needs to read from file	
		fgets(oponent_name, USERNAME_MAX_LENGTH, game_session_file_p); // first line is other player's name
		fgets(guess_read_buffer, MOVE_MAX_LENGTH, game_session_file_p); //second line is the guess
		oponent_guess = atoi(guess_read_buffer);
		other_player->wrote_to_file = FALSE;//data was read, reset flag
		read_from_file = TRUE;
	}
	fclose(game_session_file_p);
	// write move to file
	err = fopen_s(&game_session_file_p, GAME_SESSION_FILE_NAME, "w");
	if (err != 0 || game_session_file_p == NULL) {
		printf("Error opening file\n");
		return STATUS_CODE_FAILURE;
	}
	if (0 > fprintf_s(game_session_file_p, "%s\n", player->name)){
		printf("Failed to write to file.\n");
		return STATUS_CODE_FAILURE;
	}
	if (0 > fprintf_s(game_session_file_p, "%d\n", player->guess)){
		printf("Failed to write to file.\n");
		return STATUS_CODE_FAILURE;
	}
	fclose(game_session_file_p);
	player->wrote_to_file = TRUE;
	ReleaseMutex(GameSessionMutex);
	if (read_from_file == TRUE) { //if this is ture turn is finished
		if (FALSE == SetEvent(player->turn_finished)) {//signal guess is made
			printf("Couldn't set Turn_Finished event - %d\n", GetLastError());
			return STATUS_CODE_FAILURE;
		}
	}
	// wait for other player to finish his turn
	wait_res = WaitForSingleObject(other_player->turn_finished, INFINITE);
	if ((WAIT_OBJECT_0 != wait_res)) {
		printf("Couldn't wait for Turn_Finished event - %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	if (other_player->wrote_to_file == TRUE) {//data needs to be read from file
		wait_res = WaitForSingleObject(GameSessionMutex, INFINITE);
		if (WAIT_OBJECT_0 != wait_res) {
			printf("Couldn't wait for GameSessionMutex - %d\n", GetLastError());
			return STATUS_CODE_FAILURE;
		}
		err = fopen_s(&game_session_file_p, GAME_SESSION_FILE_NAME, "r");
		if (err != 0) {
			printf("Error opening file\n");
			return STATUS_CODE_FAILURE;
		}
		fgets(oponent_name, USERNAME_MAX_LENGTH, game_session_file_p); // first line is other player's name
		fgets(guess_read_buffer, MOVE_MAX_LENGTH, game_session_file_p); //second line is the guess
		oponent_guess = atoi(guess_read_buffer);
		fclose(game_session_file_p);
		other_player->wrote_to_file = FALSE; // update flag
		ReleaseMutex(GameSessionMutex);
		if (FALSE == SetEvent(player->turn_finished)) {//signal turn is done
			printf("Couldn't set Turn_Finished event - %d\n", GetLastError());
			return STATUS_CODE_FAILURE;
		}
	}
	// both players are done - calculate result results
	if (CreateResultsMessage(player, oponent_guess, oponent_name, &game_results_buffer) == STATUS_CODE_FAILURE) {
		return STATUS_CODE_FAILURE;
	}
	SendString(game_results_buffer, other_player->player_socket);
	free(game_results_buffer);
	if (FALSE == SetEvent(player->calc_finished)) {//signal results calculation is done
		printf("Couldn't set calc_finish event - %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	wait_res = WaitForSingleObject(other_player->calc_finished, 15000);	// wait for other player to finish calculation
	printf("results calculation is done, sending results\n");
	if ((WAIT_OBJECT_0 != wait_res)) {
		printf("Couldn't wait for calc_finish event - %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	int send_results_status = SendResults(player, other_player);
	return send_results_status;
}


int CreateServerInvite(Player* player,  char** send_buffer) {
	int send_buffer_length = sizeof(SERVER_GAME_RESULTS) + 2 * USERNAME_MAX_LENGTH + 1;
	*send_buffer = (char*)malloc(send_buffer_length * sizeof(char));
	if (check_malloc(*send_buffer) == EXIT_FAILURE) {
		return STATUS_CODE_FAILURE;
	}
	if (*send_buffer != 0) { // this is always true but just in case
		strcpy_s(*send_buffer, send_buffer_length, SERVER_INVITE);
		strcat_s(*send_buffer, send_buffer_length, player->name);
	}
	return STATUS_CODE_SUCCESS;
}


//conncets two players to start a game if both are willing to play
int PlayVersus(Player* player, Player* other_player, BOOL* create_game_session) {
	DWORD wait_res;
	errno_t err;
	FILE* game_session_file_p;
	char* game_invite_buffer = NULL;
	player->versus = TRUE;
	//  signal we want to play
	if (FALSE == SetEvent(player->versus_chose)) {
		printf("Couldn't set player game type event - %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	if (other_player->valid == 0) {//no oponent to play with
		if (SendString(SERVER_NO_OPPONENTS, player->player_socket) == TRNS_FAILED) {
			printf("Socket error trying to send data\n");
			return STATUS_CODE_FAILURE;
		}
		if (SendString(SERVER_MAIN_MENU, player->player_socket) == TRNS_FAILED) {
			printf("Socket error trying to send data\n");
			return STATUS_CODE_FAILURE;
		}
		return STATUS_CODE_SUCCESS;
	}
	// wait for other client to choose to play
	if(other_player->versus_chose != NULL)
		wait_res = WaitForSingleObject(other_player->versus_chose, 15000);
	if ((WAIT_OBJECT_0 != wait_res) && (WAIT_TIMEOUT != wait_res)) {
		printf("Couldn't get other player's choice - %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	if (WAIT_TIMEOUT == wait_res || other_player->valid == 0) { // the other player won't play
		if (SendString(SERVER_NO_OPPONENTS, player->player_socket) == TRNS_FAILED){
			printf("Socket error trying to send data\n");
			return STATUS_CODE_FAILURE;
		}
		if (SendString(SERVER_MAIN_MENU, player->player_socket) == TRNS_FAILED){
			printf("Socket error trying to send data\n");
			return STATUS_CODE_FAILURE;
		}
		return STATUS_CODE_SUCCESS;
	}
	else {	// the other player wants to play
		if (CreateServerInvite(player, &game_invite_buffer) == STATUS_CODE_FAILURE) {
			return STATUS_CODE_FAILURE;
		}
		if (SendString(game_invite_buffer, other_player->player_socket) == TRNS_FAILED) {
			printf("Socket error trying to send data\n");
			return STATUS_CODE_FAILURE;
		}
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
		printf("setup request sent\n");
	}
	return STATUS_CODE_SUCCESS;
}
