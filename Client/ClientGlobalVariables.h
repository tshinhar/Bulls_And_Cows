#ifndef CLIENT_GLOBAL_VARIABLES_H
#define CLIENT_GLOBAL_VARIABLES_H
#include <stdio.h>

extern HANDLE TimeOut_connectionProblem_GoOut_Semphore, SERVER_MAIN_MENU_Semphore,
SERVER_APROVED_Semphore, SERVER_INVITE_Semphore,
SERVER_SETUP_REUEST_Semphore, SERVER_PLAYER_MOVE_REQUEST_Semphore,
SERVER_GAME_RESULTS_Semphore, SERVER_WIN_Semphore,
SERVER_DRAW_Semphore, QUIT_OPPONENT_SERVER_semaphore;

extern int TimeTogoOut, ServerOpponentsQuit, ServerNoOpponemts;
extern int game_next_step;

#endif // CLIENT_GLOBAL_VARIABLES