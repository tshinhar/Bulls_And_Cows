#ifndef CONNECTION_H
#define CONNECTION_H

#include "Player.h"


#define MAX_CLIENTS 2
#define SERVER_ADDRESS "127.0.0.1"



int ClientIndex;
Player Players[MAX_CLIENTS];
HANDLE players_threads[MAX_CLIENTS];


DWORD WaitForConnection(LPVOID lpParam);
int InitServerSocket(SOCKET* server_socket, int port_num);
int DeinitializeSocket(SOCKET* socket);
DWORD RecvDataThread(int player_index);
DWORD WaitForConnection(LPVOID lpParam);
DWORD ExitInteruptThread(LPVOID lpParam);

#endif // CONECTION_H
#pragma once
