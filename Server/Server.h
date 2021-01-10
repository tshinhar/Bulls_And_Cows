#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <ws2tcpip.h>
#include <SocketTools.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "Connection.h"


#define SEND_STR_MAX_SIZE 


#pragma once