#include "Server.h"



char* MovesStrings[5] = { "ROCK", "PAPER", "SCISSORS", "LIZARD", "SPOCK" };


int main(int argc, char* argv[])
{
	SOCKET server_socket = INVALID_SOCKET;
	DWORD wait_res;
	HANDLE p_days_thread[2];

	if (argc < 2) {
		printf("Not enough arguments.\n");
		return STATUS_CODE_FAILURE;
	}

	// init variables
	ClientIndex = 0;
	TotalPlayersHistory = 0;
	NumOfActivePlayers = 0;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		Players[i].player_socket = INVALID_SOCKET;
		Players[i].play_type = NONE;
		Players[i].valid = 0;
	}

	// init server player
	strcpy_s(server_player.name, USERNAME_MAX_LENGTH, "Server");

	LeaderBoardMutex = CreateMutex(NULL, FALSE, LEADER_BOARD_MUTEX);
	GameSessionMutex = CreateMutex(NULL, FALSE, NULL);
	TotalPlayersHistoryMutex = CreateMutex(NULL, FALSE, NULL);
	if ((LeaderBoardMutex == NULL) || (GameSessionMutex == NULL) || (TotalPlayersHistoryMutex == NULL)) {
		printf("Couldn't Create mutex for broadcast. Error code: %ld\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}

	scores_array = (PlayerScores*)malloc(sizeof(PlayerScores));
	if (NULL == scores_array) {
		goto Main_Cleanup_Mutex;
	}

	if (InitServerSocket(&server_socket, atoi(argv[1])) == STATUS_CODE_FAILURE) {
		goto Main_Cleanup_Mutex;
	}

	p_days_thread[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WaitForConnection, (void*)&server_socket, 0, NULL);
	p_days_thread[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ExitInteruptThread, NULL, 0, NULL);
	wait_res = WaitForMultipleObjects(2, p_days_thread, FALSE, INFINITE);	// wait for first one to finish
	if (WAIT_OBJECT_0 + 1 != wait_res) {		// WAIT_OBJECT_0 + 1 means ExitInteruptThread is closed because the user write "exit"
		printf("Couldn't wait for main thread and exit thread. Error code: %ld\n", GetLastError());
		CloseHandle(p_days_thread[1]);
	}


	if (Players[0].valid == 1)
		PlayerDisconnect(&Players[0]);
	if (Players[1].valid == 1)
		PlayerDisconnect(&Players[1]);
	CloseHandle(p_days_thread[0]);
Main_Cleanup_Mutex:
	CloseHandle(LeaderBoardMutex);
	CloseHandle(GameSessionMutex);
	CloseHandle(TotalPlayersHistoryMutex);
	free(scores_array);
	if (IsFileExist(LEADER_BOARD_FILE_NAME))		// remove leaderboard if exist
		if (remove(LEADER_BOARD_FILE_NAME) == 0)
			printf("Deleted leaderboard successfully");
		else
			printf("Unable to delete leaderboard ");
	return STATUS_CODE_SUCCESS;
}

/*
Check if str srart with prefix
*/
int str_prefix(char* str, const char* prefix)
{
	return (strncmp(prefix, str, strlen(prefix)) == 0);
}


/*
Convert from string to Move enum
*/
Move StrToMove(char* str) {
	if (str_prefix(str, "ROCK"))
		return(ROCK);
	else if (str_prefix(str, "PAPER"))
		return(PAPER);
	else if (str_prefix(str, "SCISSORS"))
		return(SCISSORS);
	else if (str_prefix(str, "LIZARD"))
		return(LIZARD);
	else
		return(SPOCK);
}


/*
Determine whether player1 win or player2 and return 0 or 1 respectively. return -1 if no one win
*/
int WhosWin(Move player1_move, Move player2_move) {
	switch (player1_move) {
	case ROCK:
		if (player2_move == ROCK)
			return -1;
		else if ((player2_move == LIZARD) || (player2_move == SCISSORS))
			return 0;
		else
			return 1;
		break;
	case PAPER:
		if (player2_move == PAPER)
			return -1;
		else if ((player2_move == ROCK) || (player2_move == SPOCK))
			return 0;
		else
			return 1;
		break;
	case SCISSORS:
		if (player2_move == SCISSORS)
			return -1;
		else if ((player2_move == LIZARD) || (player2_move == PAPER))
			return 0;
		else
			return 1;
		break;
	case LIZARD:
		if (player2_move == LIZARD)
			return -1;
		else if ((player2_move == SPOCK) || (player2_move == PAPER))
			return 0;
		else
			return 1;
		break;
	case SPOCK:
		if (player2_move == SPOCK)
			return -1;
		else if ((player2_move == ROCK) || (player2_move == SCISSORS))
			return 0;
		else
			return 1;
		break;
	default:
		return -1;
		break;
	}
}


/*
Extract params from message and store it in dst (withoud the \n in the end)
*/
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
	if (*dst == NULL) {
		printf("Can't allocate memory\n");
		return STATUS_CODE_FAILURE;
	}
	strcpy_s(*dst, dst_length, colon_ptr);
	return STATUS_CODE_SUCCESS;
}


/*
Generate results message
*/
int CreateResultsMessage(Player player, Player other_player, char** send_buffer) {
	int send_buffer_length;
	int winner;
	send_buffer_length = sizeof(SERVER_GAME_RESULTS) + 2 * USERNAME_MAX_LENGTH + 2 * MOVE_MAX_LENGTH + 1;
	*send_buffer = (char*)malloc(send_buffer_length * sizeof(char));
	if (*send_buffer == NULL) {
		printf("Can't allocate memory\n");
		return STATUS_CODE_FAILURE;
	}
	strcpy_s(*send_buffer, send_buffer_length, SERVER_GAME_RESULTS);
	strcat_s(*send_buffer, send_buffer_length, other_player.name);
	strcat_s(*send_buffer, send_buffer_length, ";");
	strcat_s(*send_buffer, send_buffer_length, MovesStrings[other_player.last_move]);
	strcat_s(*send_buffer, send_buffer_length, ";");
	strcat_s(*send_buffer, send_buffer_length, MovesStrings[player.last_move]);
	winner = WhosWin(player.last_move, other_player.last_move);
	if (winner == 0) {
		strcat_s(*send_buffer, send_buffer_length, ";");
		strcat_s(*send_buffer, send_buffer_length, player.name);
	}
	else if (winner == 1) {
		strcat_s(*send_buffer, send_buffer_length, ";");
		strcat_s(*send_buffer, send_buffer_length, other_player.name);
	}
	strcat_s(*send_buffer, send_buffer_length, ";\n");
	return STATUS_CODE_SUCCESS;
}


int PlayerDisconnect(Player* player) {
	DWORD wait_res;
	BOOL an_error_occured = FALSE;
	if (closesocket(player->player_socket) == SOCKET_ERROR) {
		printf("Failed to close player socket, error %ld. Ending program\n", WSAGetLastError());
		an_error_occured = TRUE;
	}
	if (FALSE == SetEvent(player->type_choosed)) {		// for not blocking the other player while waiting for decision of this player
		printf("Couldn't set player game type event - %d\n", GetLastError());
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
	player->play_type = NONE;
	CloseHandle(player->type_choosed);
	CloseHandle(player->turn_finished);
	if (an_error_occured)
		return STATUS_CODE_FAILURE;
	return STATUS_CODE_SUCCESS;
}


BOOL IsFileExist(char* filename) {
	FILE* file_check_p;
	errno_t err;
	err = fopen_s(&file_check_p, filename, "r");
	if (err != 0) {	// file not exist
		return FALSE;
	}
	else
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
		fclose(file_create_p);
	}
	return STATUS_CODE_SUCCESS;
}


int SendScoreBoard(SOCKET socket) {
	FILE* file_ptr;
	char* leader_board_content;
	int messege_size = 0;
	int file_size = 0;
	char* message;
	errno_t err;
	DWORD wait_res;

	wait_res = WaitForSingleObject(LeaderBoardMutex, INFINITE);
	if (WAIT_OBJECT_0 != wait_res)
	{
		printf("Couldn't wait for LeaderBoardMutex - %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	if (STATUS_CODE_FAILURE == CreateFileIfNotExist(LEADER_BOARD_FILE_NAME)) {		// if leader board file isn't exist - create it
		return STATUS_CODE_FAILURE;
	}

	// read all leader board file content
	err = fopen_s(&file_ptr, LEADER_BOARD_FILE_NAME, "r");
	if (err != 0)
	{
		printf("Unable to create file.\n");
		return STATUS_CODE_FAILURE;
	}
	fseek(file_ptr, 0, SEEK_END);
	file_size = ftell(file_ptr);
	fseek(file_ptr, 0, SEEK_SET);

	leader_board_content = (char*)malloc(sizeof(char) * file_size);
	if (leader_board_content == NULL) {
		return STATUS_CODE_FAILURE;
	}
	fread(leader_board_content, sizeof(char), file_size, file_ptr);
	leader_board_content[file_size - 1] = '\0';

	messege_size = file_size + sizeof(SERVER_LEADERBOARD) + 1;
	message = (char*)malloc(sizeof(char) * messege_size);
	if (message == NULL) {
		free(leader_board_content);
		return STATUS_CODE_FAILURE;
	}
	strcpy_s(message, messege_size, SERVER_LEADERBOARD);
	strcat_s(message, messege_size, leader_board_content);

	free(leader_board_content);
	SendString(message, socket);
	fclose(file_ptr);
	free(message);
	ReleaseMutex(LeaderBoardMutex);
	SendString(SERVER_LEADERBORAD_MENU, socket);
	return STATUS_CODE_SUCCESS;
}

int NewUser(Player* player, char* username) {
	DWORD wait_res;
	strcpy_s(player->name, USERNAME_MAX_LENGTH, username);

	player->turn_finished = NewEvent(NULL, AUTO_RESET);
	player->type_choosed = NewEvent(NULL, AUTO_RESET);
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

	wait_res = WaitForSingleObject(TotalPlayersHistoryMutex, INFINITE);
	if (WAIT_OBJECT_0 != wait_res)
	{
		printf("Couldn't wait for GameSessionMutex - %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	TotalPlayersHistory += 1;
	scores_array = (PlayerScores*)realloc(scores_array, TotalPlayersHistory * sizeof(PlayerScores));
	if (NULL == scores_array) {
		ReleaseMutex(TotalPlayersHistoryMutex);
		return STATUS_CODE_FAILURE;
	}
	strcpy_s(scores_array[TotalPlayersHistory - 1].name, USERNAME_MAX_LENGTH, username);
	scores_array[TotalPlayersHistory - 1].lose = 0;
	scores_array[TotalPlayersHistory - 1].won = 0;
	scores_array[TotalPlayersHistory - 1].ratio = 0;
	ReleaseMutex(TotalPlayersHistoryMutex);
	return STATUS_CODE_SUCCESS;
}

/*
A thread for managing recieved data from player
*/
static DWORD RecvDataThread(int player_index) {
	TransferResult_t recv_res;
	Player* player = &Players[player_index];
	Player* other_player = &Players[1 - player_index];
	char* params;
	FILE* game_session_file = NULL;
	BOOL an_error_occured = FALSE;
	char* recv_buffer = NULL;
	BOOL is_create_game_session = FALSE;

	while (TRUE) {
		// ReceiveBuffer contain call for recv that is blocking function
		recv_buffer = NULL;
		recv_res = ReceiveString(&recv_buffer, player->player_socket);
		if (recv_res == TRNS_SUCCEEDED) {
			if (str_prefix(recv_buffer, CLIENT_REQUEST)) {	// we already check that we didn't reach the maximum client so just approve
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
			else if (str_prefix(recv_buffer, CLIENT_CPU)) {
				if (STATUS_CODE_FAILURE == PlayVersusServer(player)) {
					an_error_occured = TRUE;
					goto Cleanup_1;
				}
			}
			else if (str_prefix(recv_buffer, CLIENT_VERSUS)) {
				if (STATUS_CODE_FAILURE == PlayVersus(player, other_player, &is_create_game_session, FALSE)) {
					an_error_occured = TRUE;
					goto Cleanup_1;
				}
			}
			else if (str_prefix(recv_buffer, CLIENT_PLAYER_MOVE)) {
				// update player move
				player->last_move = StrToMove(strchr(recv_buffer, ':') + 1);

				if (player->play_type == VERSUS) {
					if (STATUS_CODE_FAILURE == PlayMoveVesus(player, other_player)) {
						an_error_occured = TRUE;
						goto Cleanup_1;
					}
					if (is_create_game_session) {
						if (STATUS_CODE_FAILURE == RemoveGameSessionFile()) {
							an_error_occured = TRUE;
							goto Cleanup_1;
						}
					}
				}
				else {		// play move versus the server
					char* game_results_buffer = NULL;
					if (CreateResultsMessage(*player, server_player, &game_results_buffer) == STATUS_CODE_FAILURE) {
						an_error_occured = TRUE;
						goto Cleanup_1;
					}
					SendString(game_results_buffer, player->player_socket);
					free(game_results_buffer);
					SendString(SERVER_GAME_OVER_MENU, player->player_socket);
				}
			}
			else if (str_prefix(recv_buffer, CLIENT_REPLAY)) {
				if (player->play_type == VERSUS) {
					if (STATUS_CODE_FAILURE == PlayVersus(player, other_player, &is_create_game_session, TRUE)) {
						an_error_occured = TRUE;
						goto Cleanup_1;
					}
				}
				else {		// play again versus the server
					if (STATUS_CODE_FAILURE == PlayVersusServer(player)) {
						an_error_occured = TRUE;
						goto Cleanup_1;
					}
				}
			}
			else if (str_prefix(recv_buffer, CLIENT_MAIN_MENU)) {
				player->play_type = NONE;	// this player don't choose game type or choose not to replay
				if (FALSE == SetEvent(player->type_choosed)) {
					printf("Couldn't set player game type event - %d\n", GetLastError());
					an_error_occured = TRUE;
					goto Cleanup_1;
				}
				if (SendString(SERVER_MAIN_MENU, player->player_socket) == TRNS_FAILED)
				{
					printf("Socket error while trying to write data to socket\n");
					return STATUS_CODE_FAILURE;
				}
			}
			else if ((str_prefix(recv_buffer, CLIENT_LEADERBOARD)) || (str_prefix(recv_buffer, CLIENT_REFRESH))) {
				if (SendScoreBoard(player->player_socket) == STATUS_CODE_FAILURE) {
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
	if (STATUS_CODE_FAILURE == PlayerDisconnect(player))
		an_error_occured = TRUE;
	if (an_error_occured) {
		return STATUS_CODE_FAILURE;
	}
	return STATUS_CODE_SUCCESS;
}

int PlayVersusServer(Player* player) {
	time_t t;
	srand((unsigned)time(&t));	// for random move when play vesus the server
	int rnd_move_index = rand() % 5;
	server_player.last_move = rnd_move_index;
	if (SendString(SERVER_PLAYER_MOVE_REQUEST, player->player_socket) == TRNS_FAILED)
	{
		printf("Socket error while trying to write data to socket\n");
		return STATUS_CODE_FAILURE;
	}
	return STATUS_CODE_SUCCESS;
}

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

/*
if player win is_win = 1, otherwise is_win = 0
*/
int UpdateScoreBoard(Player* player, int is_win) {
	DWORD wait_res;
	wait_res = WaitForSingleObject(TotalPlayersHistoryMutex, INFINITE);
	if (WAIT_OBJECT_0 != wait_res)
	{
		printf("Couldn't wait for TotalPlayersHistoryMutex - %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	for (int i = 0; i < TotalPlayersHistory; i++) {
		if (strcmp(player->name, scores_array[i].name) == 0) {
			scores_array[i].won += is_win;
			scores_array[i].lose += (1 - is_win);
			if (scores_array[i].lose != 0)
				scores_array[i].ratio = (float)scores_array[i].won / (float)scores_array[i].lose;
			else
				scores_array[i].ratio = 0;
			break;
		}
	}

	// make array of ratios in descending order
	float* players_ratio_order = (float*)malloc(TotalPlayersHistory * sizeof(float));
	if (NULL == players_ratio_order) {
		printf("Unable allocate memory.\n");
		ReleaseMutex(TotalPlayersHistoryMutex);
		return STATUS_CODE_FAILURE;
	}
	for (int i = 0; i < TotalPlayersHistory; ++i) {
		players_ratio_order[i] = scores_array[i].ratio;
	}
	float tmp;
	for (int i = 0; i < TotalPlayersHistory; ++i) {
		for (int j = i + 1; j < TotalPlayersHistory; ++j) {
			if (players_ratio_order[i] < players_ratio_order[j])
			{
				tmp = players_ratio_order[i];
				players_ratio_order[i] = players_ratio_order[j];
				players_ratio_order[j] = tmp;
			}
		}
	}

	// open leaderboard file and write the array in descending order
	FILE* leader_board_file_p;
	errno_t err;
	err = fopen_s(&leader_board_file_p, LEADER_BOARD_FILE_NAME, "w");
	if (err != 0) {
		printf("Unable to create file.\n");
		ReleaseMutex(TotalPlayersHistoryMutex);
		free(players_ratio_order);
		return STATUS_CODE_FAILURE;
	}

	// write title
	if (EOF == fputs(LEADER_BOARD_TITLE, leader_board_file_p))
	{
		printf("Failed to write to file.\n");
		if (0 != fclose(leader_board_file_p))
		{
			printf("Failed to close file.\n");
			free(players_ratio_order);
			return STATUS_CODE_FAILURE;
		}
		free(players_ratio_order);
		return STATUS_CODE_FAILURE;
	}
	// write details
	for (int i = 0; i < TotalPlayersHistory; ++i) {		// loop over players_ratio_order
		for (int j = 0; j < TotalPlayersHistory; ++j) {		// loop over scores_array
			if (players_ratio_order[i] == scores_array[j].ratio) {
				fprintf(leader_board_file_p, "%s,%d,%d,%.3f\n", scores_array[j].name, scores_array[j].won, scores_array[j].lose, scores_array[j].ratio);
				scores_array[j].ratio = -1;		// this way we won't print same player twice if theres two players with same ratio
			}
		}
	}
	// re-calculate scores_array ratio values
	for (int i = 0; i < TotalPlayersHistory; i++) {
		if (scores_array[i].lose != 0)
			scores_array[i].ratio = (float)scores_array[i].won / (float)scores_array[i].lose;
		else
			scores_array[i].ratio = 0;
	}

	fclose(leader_board_file_p);
	free(players_ratio_order);
	ReleaseMutex(TotalPlayersHistoryMutex);
	return STATUS_CODE_SUCCESS;
}

int PlayMoveVesus(Player* player, Player* other_player) {
	DWORD wait_res;
	BOOL opend_file = FALSE;
	char* game_results_buffer = NULL;
	errno_t err;
	FILE* game_session_file_p;

	wait_res = WaitForSingleObject(GameSessionMutex, INFINITE);
	if (WAIT_OBJECT_0 != wait_res)
	{
		printf("Couldn't wait for GameSessionMutex - %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	// write move to file
	err = fopen_s(&game_session_file_p, GAME_SESSION_FILE_NAME, "a");
	if (err != 0) {
		printf("Unable to create file.\n");
		return STATUS_CODE_FAILURE;
	}
	if (EOF == fputs(MovesStrings[player->last_move], game_session_file_p))
	{
		printf("Failed to write to file.\n");
		if (0 != fclose(game_session_file_p))
		{
			printf("Failed to close file.\n");
			return STATUS_CODE_FAILURE;
		}
		return STATUS_CODE_FAILURE;
	}
	putc('\n', game_session_file_p);
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

	// both player play their turn - send results
	if (CreateResultsMessage(*player, *other_player, &game_results_buffer) == STATUS_CODE_FAILURE) {
		return STATUS_CODE_FAILURE;
	}
	SendString(game_results_buffer, player->player_socket);
	free(game_results_buffer);
	SendString(SERVER_GAME_OVER_MENU, player->player_socket);

	// update score board
	int is_win = WhosWin(player->last_move, other_player->last_move);
	if (is_win != -1) {	// one of them win
		if (UpdateScoreBoard(player, 1 - is_win) == STATUS_CODE_FAILURE) {
			return STATUS_CODE_FAILURE;
		}
	}
	return STATUS_CODE_SUCCESS;
}


int PlayVersus(Player* player, Player* other_player, BOOL* create_game_session, BOOL replay) {
	DWORD wait_res;
	errno_t err;
	FILE* game_session_file_p;

	player->play_type = VERSUS;
	// tell other client that this client choose play type-vesus
	if (FALSE == SetEvent(player->type_choosed)) {
		printf("Couldn't set player game type event - %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}

	// wait for other client to choose game type (Single ot Versus)
	wait_res = WaitForSingleObject(other_player->type_choosed, 15000);
	if ((WAIT_OBJECT_0 != wait_res) && (WAIT_TIMEOUT != wait_res)) {
		printf("Couldn't wait for type event - %d\n", GetLastError());
		return STATUS_CODE_FAILURE;
	}
	if (WAIT_TIMEOUT == wait_res) {
		if (SendString(SERVER_NO_OPPONENTS, player->player_socket) == TRNS_FAILED)
		{
			printf("Socket error while trying to write data to socket\n");
			return STATUS_CODE_FAILURE;
		}
		player->play_type = NONE;
		if (FALSE == ResetEvent(player->type_choosed)) {
			printf("Couldn't reseset player game type event - %d\n", GetLastError());
			return STATUS_CODE_FAILURE;
		}
		if (SendString(SERVER_MAIN_MENU, player->player_socket) == TRNS_FAILED)
		{
			printf("Socket error while trying to write data to socket\n");
			return STATUS_CODE_FAILURE;
		}
		return STATUS_CODE_SUCCESS;
	}
	// the other player decide to play vs the computer
	if (other_player->play_type != VERSUS) {
		if (!replay) {			// send SERVER_NO_OPPONENTS at first round otherwise send SERVER_OPPONENT_QUIT 
			if (SendString(SERVER_NO_OPPONENTS, player->player_socket) == TRNS_FAILED)
			{
				printf("Socket error while trying to write data to socket\n");
				return STATUS_CODE_FAILURE;
			}
			player->play_type = NONE;
			if (FALSE == ResetEvent(player->type_choosed)) {
				printf("Couldn't reseset player game type event - %d\n", GetLastError());
				return STATUS_CODE_FAILURE;
			}
			if (SendString(SERVER_MAIN_MENU, player->player_socket) == TRNS_FAILED)
			{
				printf("Socket error while trying to write data to socket\n");
				return STATUS_CODE_FAILURE;
			}
			return STATUS_CODE_SUCCESS;
		}
		else {
			char quit_string[sizeof(SERVER_OPPONENT_QUIT) + USERNAME_MAX_LENGTH + 1];
			strcpy_s(quit_string, sizeof(SERVER_OPPONENT_QUIT) + USERNAME_MAX_LENGTH + 1, SERVER_OPPONENT_QUIT);
			strcat_s(quit_string, sizeof(SERVER_OPPONENT_QUIT) + USERNAME_MAX_LENGTH + 1, other_player->name);
			strcat_s(quit_string, sizeof(SERVER_OPPONENT_QUIT) + USERNAME_MAX_LENGTH + 1, "\n");
			if (SendString(quit_string, player->player_socket) == TRNS_FAILED)
			{
				printf("Socket error while trying to write data to socket\n");
				return STATUS_CODE_FAILURE;
			}
			player->play_type = NONE;
			if (FALSE == ResetEvent(player->type_choosed)) {
				printf("Couldn't reseset player game type event - %d\n", GetLastError());
				return STATUS_CODE_FAILURE;
			}
			if (SendString(SERVER_MAIN_MENU, player->player_socket) == TRNS_FAILED)
			{
				printf("Socket error while trying to write data to socket\n");
				return STATUS_CODE_FAILURE;
			}
		}

	}
	else {		// the other player wants to play versus this player
		wait_res = WaitForSingleObject(GameSessionMutex, INFINITE);
		if (WAIT_OBJECT_0 != wait_res)
		{
			printf("Couldn't wait for GameSessionMutex - %d\n", GetLastError());
			return STATUS_CODE_FAILURE;
		}
		*create_game_session = FALSE;
		if (!IsFileExist(GAME_SESSION_FILE_NAME)) {	// create GameSession file if not exist
			err = fopen_s(&game_session_file_p, GAME_SESSION_FILE_NAME, "w");
			if (err != 0) {
				printf("Unable to create file.\n");
				return STATUS_CODE_FAILURE;
			}
			fclose(game_session_file_p);
			*create_game_session = TRUE;
		}
		ReleaseMutex(GameSessionMutex);

		if (!replay) {			// send SERVER_INVITE only at firs round
			if (SendString(SERVER_INVITE, player->player_socket) == TRNS_FAILED)
			{
				printf("Socket error while trying to write data to socket\n");
				return STATUS_CODE_FAILURE;
			}
		}
		if (SendString(SERVER_PLAYER_MOVE_REQUEST, player->player_socket) == TRNS_FAILED)
		{
			printf("Socket error while trying to write data to socket\n");
			return STATUS_CODE_FAILURE;
		}
	}
	return STATUS_CODE_SUCCESS;
}

/*
Wait for exit command from user
*/
static DWORD ExitInteruptThread(LPVOID lpParam) {
	char str[5];
	scanf_s("%s", str, 5);
	while (strcmp(str, "exit") != 0) {
		scanf_s("%s", str, 5);
		printf("%s\n", str);
	}
	return 0;
}

/*
Main thread that managing incoming connection requests
*/
static DWORD WaitForConnection(LPVOID lpParam) {
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
			printf("Accept for new connection with client failed, error %ld\n", WSAGetLastError());
			an_error_occured = TRUE;
			goto Cleanup_2;
		}
		//Accept a request , as long as didn't reach MAX_CLIENTS
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
			SendString(SERVER_DENIED_REACH_MAX, refusal_socket);
			TransferResult_t recv_res;
			char* recv_buffer = NULL;
			recv_res = ReceiveString(&recv_buffer, refusal_socket);		// because client should send CLIENT_REQUEST also in this situation
			if (recv_res != TRNS_SUCCEEDED) {
				printf("erro while recieved from socket.\n");
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

			// check which elemnt in player array is free
			if (Players[0].valid == 0)
				ClientIndex = 0;
			else
				ClientIndex = 1;
			Players[ClientIndex].player_socket = accept_socket;
			Players[ClientIndex].valid = 1;
			players_send_recv_threads[ClientIndex] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RecvDataThread, (LPVOID)ClientIndex, 0, NULL);
			if (NULL == players_send_recv_threads[ClientIndex])
			{
				printf("Couldn't create thread\n");
				an_error_occured = TRUE;
				goto Cleanup_2;
			}
		}
	}
	wait_res = WaitForMultipleObjects(MAX_CLIENTS, players_send_recv_threads, TRUE, INFINITE);
	if (WAIT_OBJECT_0 != wait_res)
	{
		printf("Couldn't wait for send/recv threads - %d\n", GetLastError());
		an_error_occured = TRUE;
	}

Cleanup_2:
	closesocket(accept_socket);
	CloseHandle(players_send_recv_threads[0]);
	CloseHandle(players_send_recv_threads[1]);
Cleanup_1:
	CloseHandle(NumOfActivePlayersMutex);
	if (an_error_occured) return STATUS_CODE_FAILURE;
	return STATUS_CODE_SUCCESS;
}

int InitServerSocket(SOCKET* server_socket, int port_num) {
	// Initialize Winsock.
	WSADATA wsa_data;
	unsigned long address;
	SOCKADDR_IN service;
	int status;

	status = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (status != 0) {
		printf("WSAStartup failed: %d\n", status);
		return STATUS_CODE_FAILURE;
	}

	// Create a socket.    
	*server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*server_socket == INVALID_SOCKET)
	{
		printf("Error at socket( ): %ld\n", WSAGetLastError());
		DeinitializeSocket(NULL);
		return STATUS_CODE_FAILURE;
	}


	address = inet_addr(SERVER_ADDRESS_STR);
	if (address == INADDR_NONE)
	{
		printf("The string \"%s\" cannot be converted into an ip address. ending program.\n",
			SERVER_ADDRESS_STR);
		DeinitializeSocket(server_socket);
		return STATUS_CODE_FAILURE;
	}

	service.sin_family = AF_INET;
	service.sin_addr.s_addr = address;
	service.sin_port = htons(port_num);

	//Call bind
	status = bind(*server_socket, (SOCKADDR*)&service, sizeof(service));
	if (status == SOCKET_ERROR)
	{
		printf("bind( ) failed with error %ld. Ending program\n", WSAGetLastError());
		DeinitializeSocket(server_socket);
		return STATUS_CODE_FAILURE;
	}

	// Listen on the Socket.
	status = listen(*server_socket, SOMAXCONN);
	if (status == SOCKET_ERROR) {
		printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
		DeinitializeSocket(server_socket);
		return STATUS_CODE_FAILURE;
	}
	return STATUS_CODE_SUCCESS;
}

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

/*
	create new non-signaled event or return event handler if event with this name already exist
*/
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
