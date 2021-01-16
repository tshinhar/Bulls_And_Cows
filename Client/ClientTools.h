#ifndef CLIENTTOOLS_H
#define CLIENTTOOLS_H
#include <stdio.h>
#include <stdbool.h>
#include "SocketTools.h"


// Macros
#define STATUS_CODE_FAILURE FALSE
#define STATUS_CODE_SUCCESS TRUE
#define USERNAME_MAX_LENGTH 20 // As said in the exersize notes, page 4
#define USERNAME_MAX_LENGTH_IP_ADRESS 15//XXX.XXX.XXX.XXX = 12 numbers+3 dots
#define LONGEST_MESSAGE USERNAME_MAX_LENGTH+16
#define WAIT_15S 15000 // for server responses
#define WAIT_30S 30000  // in case there are no current opponent that wants to join and there's only one player

// Types
enum state { again, endofwork, notValid };
enum server {
	SERVER_MAIN_MENU, SERVER_APPROVED, SERVER_DENIED, SERVER_INVITE, SERVER_SETUP_REQUEST, SERVER_PLAYER_MOVE_REQUEST,
	SERVER_GAME_RESULTS, SERVER_WIN, SERVER_DRAW, SERVER_NO_OPPONENTS, SERVER_OPPONENT_QUIT
};
typedef struct Client {
	char ip_address[USERNAME_MAX_LENGTH_IP_ADRESS];
	int port_name;
	char username[USERNAME_MAX_LENGTH];
}client;


// Init Global Variables
HANDLE TimeOut_connectionProblem_GoOut_Semphore;
HANDLE SERVER_MAIN_MENU_Semphore;
HANDLE SERVER_APROVED_Semphore;
HANDLE SERVER_INVITE_Semphore;
HANDLE SERVER_SETUP_REUEST_Semphore;
HANDLE SERVER_PLAYER_MOVE_REQUEST_Semphore;
HANDLE SERVER_GAME_RESULTS_Semphore;
HANDLE SERVER_WIN_Semphore;
HANDLE SERVER_DRAW_Semphore;
HANDLE QUIT_OPPONENT_SERVER_semaphore;
int TimeTogoOut, ServerOpponentsQuit, ServerNoOpponemts;
int game_next_step;

// Functions
int Check_what_to_do_next();
int check_if_SendReceiveString(TransferResult_t ReceiveResult);
int str_prefix(char* str, const char* prefix);
int Server_ReceiveString(char* ptr);
//int Start_a_game(client* user);
//int Game_progress(client* user);
int Server_connection_issue(client* user);
int Client_disconnected(char message[LONGEST_MESSAGE], SOCKET m_socket);
int CreateAllSemphores();
void closeAllHandle();

SOCKET m_socket;

#endif // CLIENTTOOLS_H