#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <ws2tcpip.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <stdbool.h>
#include <SocketTools.h>
#include <Utilities.h>

#include "Messages.h"


#define USERNAME_MAX_LENGTH 20
#define MANUAL_RESET TRUE
#define AUTO_RESET FALSE
#define GAME_SESSION_FILE_NAME "GameSession.txt"
#define MOVE_MAX_LENGTH 8

HANDLE GameSessionMutex;
HANDLE NumOfActivePlayersMutex;
int NumOfActivePlayers;

typedef struct {
	char name[USERNAME_MAX_LENGTH];
	SOCKET player_socket;
	HANDLE turn_finished; // event that set when this player made a guess
	int versus; // flag if the player wants to play vs another player
	HANDLE versus_chose; // event that the player chose to play vs another player
	int chosen_num; //the 4 digit number the player chose at setup
	int guess; // the guess of the oponent's number
	BOOL lost; // is the player lost the game
	int valid; // flag if the player is active or not
} Player;

int RemoveGameSessionFile();
int PlayerDisconnect(Player* player);
int NewUser(Player* player, char* username);
int PlayerSetup(Player* player, char* num_str);
int PlayMoveVesus(Player* player, Player* other_player);
int PlayVersus(Player* player, Player* other_player, BOOL* create_game_session);
#pragma once
