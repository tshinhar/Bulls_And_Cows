#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <Utilities.h>

typedef enum { TRNS_FAILED, TRNS_DISCONNECTED, TRNS_SUCCEEDED } TransferResult_t;

#define STATUS_CODE_FAILURE -1
#define STATUS_CODE_SUCCESS 0
#define MAX_CLIENTS 2
#define LEADER_BOARD_MUTEX "LeaderBoardMutex"
#define LEADER_BOARD_FILE_NAME "Leaderboard.csv"
#define GAME_SESSION_FILE_NAME "GameSession.txt"
#define LEADER_BOARD_TITLE "Name,Won,Lost,W/L Ratio\n"
#define SERVER_ADDRESS_STR "127.0.0.1"

// Server Messages
#define SERVER_MAIN_MENU "SERVER_MAIN_MENU\n"
#define SERVER_APPROVED "SERVER_APPROVED\n"
#define SERVER_DENIED_REACH_MAX "SERVER_DENIED:REACH_MAX_OF_PLAYERS\n"
#define SERVER_INVITE "SERVER_INVITE\n"
#define SERVER_PLAYER_MOVE_REQUEST "SERVER_PLAYER_MOVE_REQUEST\n"
#define SERVER_GAME_RESULTS "SERVER_GAME_RESULTS:"
#define SERVER_GAME_OVER_MENU "SERVER_GAME_OVER_MENU\n"
#define SERVER_OPPONENT_QUIT "SERVER_OPPONENT_QUIT:"
#define SERVER_NO_OPPONENTS "SERVER_NO_OPPONENTS\n"
#define SERVER_LEADERBOARD "SERVER_LEADERBOARD:"
#define SERVER_LEADERBORAD_MENU "SERVER_LEADERBORAD_MENU"

#define SEND_STR_MAX_SIZE 
#define USERNAME_MAX_LENGTH 20
#define MOVE_MAX_LENGTH 8
#define MANUAL_RESET TRUE
#define AUTO_RESET FALSE


// Client Messages
#define CLIENT_REQUEST "CLIENT_REQUEST:"
#define CLIENT_VERSUS "CLIENT_VERSUS"
#define CLIENT_PLAYER_MOVE "CLIENT_PLAYER_MOVE:"
#define CLIENT_LEADERBOARD "CLIENT_LEADERBOARD"
#define CLIENT_REFRESH "CLIENT_REFRESH"
#define CLIENT_DISCONNECT "CLIENT_DISCONNECT"
#define CLIENT_CPU "CLIENT_CPU"
#define CLIENT_REPLAY "CLIENT_REPLAY"
#define CLIENT_MAIN_MENU "CLIENT_MAIN_MENU"
typedef enum { NONE, SINGLE_PLAYER, VERSUS } PlayType;
typedef enum { ROCK, PAPER, SCISSORS, LIZARD, SPOCK } Move;
extern char* MovesStrings[5];

typedef struct {
	char name[USERNAME_MAX_LENGTH];
	SOCKET player_socket;
	HANDLE type_choosed;	// event that set when this player choose which play type he want
	HANDLE turn_finished;	// event that set when this player play his move
	PlayType play_type;
	Move last_move;
	int valid;				// is this player is still in game or already diconnect
} Player;
Player Players[MAX_CLIENTS];
HANDLE players_send_recv_threads[MAX_CLIENTS];
Player server_player;
int NumOfActivePlayers;
HANDLE NumOfActivePlayersMutex;


// leader board variables
typedef struct {
	char name[USERNAME_MAX_LENGTH];
	int won;
	int lose;
	float ratio;
} PlayerScores;
PlayerScores* scores_array;
int TotalPlayersHistory;		// save how many clients play versus other players (for leaderboard)
HANDLE TotalPlayersHistoryMutex;
HANDLE LeaderBoardMutex;

HANDLE GameSessionMutex;
int ClientIndex;
int NumOfActivePlayers;

TransferResult_t ReceiveBuffer(char* OutputBuffer, int BytesToReceive, SOCKET sd);
TransferResult_t SendBuffer(const char* Buffer, int BytesToSend, SOCKET sd);
TransferResult_t ReceiveString(char** OutputStrPtr, SOCKET sd);
TransferResult_t SendString(const char* Str, SOCKET sd);

int InitServerSocket(SOCKET* server_socket, int port_num);
int DeinitializeSocket(SOCKET* socket);
static DWORD WaitForConnection(LPVOID lpParam);
static DWORD ExitInteruptThread(LPVOID lpParam);
static DWORD RecvDataThread(int player_index);
int CreateFileIfNotExist(char* filename);
int CreateResultsMessage(Player player, Player other_player, char** send_buffer);
int ExtraceParams(char* str, char** dst);
int WhosWin(Move player1_move, Move player2_move);
Move StrToMove(char* str);
int str_prefix(char* str, const char* prefix);
HANDLE NewEvent(const char EVENT_NAME[], BOOL manualReset);
int PlayVersus(Player* player, Player* other_player, BOOL* create_game_session, BOOL replay);
int PlayMoveVesus(Player* player, Player* other_player);
int RemoveGameSessionFile();
int NewUser(Player* player, char* username);
int PlayVersusServer(Player* player);
BOOL IsFileExist(char* filename);
int PlayerDisconnect(Player* player);
int SendScoreBoard(SOCKET socket);
int UpdateScoreBoard(Player* player, int is_win);
#pragma once