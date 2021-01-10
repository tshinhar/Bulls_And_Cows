#include "SocketTools.h"

#include <stdio.h>
#include <string.h>


TransferResult_t SendBuffer(const char* Buffer, int BytesToSend, SOCKET sd)
{
	const char* CurPlacePtr = Buffer;
	int BytesTransferred;
	int RemainingBytesToSend = BytesToSend;

	while (RemainingBytesToSend > 0) // Dont forget that send does not guarantee that the entire message is sent
	{
		BytesTransferred = send(sd, CurPlacePtr, RemainingBytesToSend, 0);
		if (BytesTransferred == SOCKET_ERROR)
		{
			printf("send() failed, error %d\n", WSAGetLastError());
			return TRNS_FAILED;
		}
		RemainingBytesToSend -= BytesTransferred;
		CurPlacePtr += BytesTransferred; // <ISP> pointer arithmetic
	}
	return TRNS_SUCCEEDED;
}


TransferResult_t SendString(const char* Str, SOCKET sd)// Request is sent in two parts- first the length of the string (stored in an int variable), then 2nd is the string itself
{
	int TotalStringSizeInBytes;
	TransferResult_t SendResult;

	TotalStringSizeInBytes = (int)(strlen(Str) + 1); // Plus one since terminating zero is sent as well	
	SendResult = SendBuffer(// Sends the the request to the server on socket sd
		(const char*)(&TotalStringSizeInBytes),
		(int)(sizeof(TotalStringSizeInBytes)),
		sd);

	if (SendResult != TRNS_SUCCEEDED)
		return SendResult;

	SendResult = SendBuffer(
		(const char*)(Str),
		(int)(TotalStringSizeInBytes),
		sd);
	return SendResult;
}


TransferResult_t ReceiveBuffer(char* OutputBuffer, int BytesToReceive, SOCKET sd)
{
	char* CurPlacePtr = OutputBuffer;
	int BytesJustTransferred;
	int RemainingBytesToReceive = BytesToReceive;

	while (RemainingBytesToReceive > 0)// Dont forget that send does not guarantee that the entire message is sent
	{
		BytesJustTransferred = recv(sd, CurPlacePtr, RemainingBytesToReceive, 0);
		if (BytesJustTransferred == SOCKET_ERROR)
		{
			printf("recv() failed, error %d\n", WSAGetLastError());
			return TRNS_FAILED;
		}
		else if (BytesJustTransferred == 0)//The connection was gracefully disconnected
			return TRNS_DISCONNECTED;

		RemainingBytesToReceive -= BytesJustTransferred;
		CurPlacePtr += BytesJustTransferred; // <ISP> pointer arithmetic
	}
	return TRNS_SUCCEEDED;
}


TransferResult_t ReceiveString(char** OutputStrPtr, SOCKET sd)// The request is received in two parts- first the length of the string (which is stored in an int variable), then the string itself
{
	int TotalStringSizeInBytes;
	TransferResult_t ReceiveResult;
	char* StrBuffer = NULL;

	if ((OutputStrPtr == NULL) || (*OutputStrPtr != NULL))
	{
		printf("The first input to ReceiveString() must be "
			"a pointer to a char pointer that is initialized to NULL. For example:\n"
			"\tchar* Buffer = NULL;\n"
			"\tReceiveString( &Buffer, ___ )\n");
		return TRNS_FAILED;
	}

	ReceiveResult = ReceiveBuffer( // Receives the request to the server on socket sd
		(char*)(&TotalStringSizeInBytes),
		(int)(sizeof(TotalStringSizeInBytes)), // 4 bytes
		sd);

	if (ReceiveResult != TRNS_SUCCEEDED)
		return ReceiveResult;

	StrBuffer = (char*)malloc(TotalStringSizeInBytes * sizeof(char));

	if (StrBuffer == NULL)
		return TRNS_FAILED;

	ReceiveResult = ReceiveBuffer(
		(char*)(StrBuffer),
		(int)(TotalStringSizeInBytes),
		sd);

	if (ReceiveResult == TRNS_SUCCEEDED)
	{
		*OutputStrPtr = StrBuffer;
	}
	else
	{
		free(StrBuffer);
	}

	return ReceiveResult;
}