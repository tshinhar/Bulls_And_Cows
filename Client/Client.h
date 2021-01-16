#define _WINSOCK_DEPRECATED_NO_WARNINGS //for "client_connection_Service" in main
#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <string.h>
#include <winsock2.h>


#include "SocketTools.h"
#include "ClientTools.h"

// Macros

#define CPU_LONG 3

// Functions
static DWORD SendDataThread(LPVOID lpParam);
static DWORD ReceiveDataThread(LPVOID lpParam);

#endif // CLIENT_H