#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdatomic.h>

#include "ecat_system.h"

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "5020"

atomic_int plc_out_reg = 0;
atomic_int plc_in_reg = 0;

static char banner[512];


//1 client version

unsigned __stdcall plc_tcp_server_thread(void* arg) {
	EcatSystem* sys = (EcatSystem*)arg;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	WSADATA wsaData;
	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;
	struct addrinfo* result = NULL, hints;
	int iResult;

	int banner_size = snprintf(
		banner, sizeof(banner),
		"PLC-EtherCAT Server\r\n"
		"Master state: %s\r\n"
		"Slaves discovered: %d\r\n"
		"System: %s\r\n"
		"---------------------------------------\r\n"
		"Commands: READ, WRITE, STATE, QUIT\r\n",
		ecat_state_to_string(sys->system_state),
		sys->slave_count,
		atomic_load(&sys->running) ? "RUNNING" : "STOPPED"
	);





	printf("[PLC TCP] Starting server...\n");

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("[PLC TCP] getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("[PLC TCP] socket failed: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	//pour éviter un trop gros jitter à la fermeture de la socket
	struct linger lin;
	lin.l_onoff = 1;     // activer linger
	lin.l_linger = 0;    // fermer immédiatement
	setsockopt(ClientSocket, SOL_SOCKET, SO_LINGER, (char*)&lin, sizeof(lin));


	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	freeaddrinfo(result);

	if (iResult == SOCKET_ERROR) {
		printf("[PLC TCP] bind failed: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("[PLC TCP] listen failed: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	printf("[PLC TCP] Waiting for client...\n");

	while (atomic_load(&sys->running)) {
		ClientSocket = accept(ListenSocket, NULL, NULL);

		if (!atomic_load(&sys->running))
			break;

		if (ClientSocket == INVALID_SOCKET) {
			printf("[PLC TCP] accept failed: %d\n", WSAGetLastError());
			continue;
		}

		printf("[PLC TCP] Client connected\n");

		send(ClientSocket, banner, banner_size, 0);

		char rxbuf[256];

		while (atomic_load(&sys->running)) {
			int bytes = recv(ClientSocket, rxbuf, sizeof(rxbuf) - 1, 0);
			if (bytes <= 0)
				break;

			rxbuf[bytes] = 0;

			// Commandes simples
			if (strncmp(rxbuf, "READ", 4) == 0)
			{
				int v = atomic_load(&plc_in_reg);
				char tx[64];
				sprintf_s(tx, "VALUE %d\n", v);
				send(ClientSocket, tx, (int)strlen(tx), 0);
			}
			else if (strncmp(rxbuf, "WRITE ", 6) == 0)
			{
				int v = atoi(rxbuf + 6);
				atomic_store(&plc_out_reg, v);
				send(ClientSocket, "OK\n", 3, 0);
			}
			else
			{
				send(ClientSocket, "ERR\n", 4, 0);
			}
		}

		shutdown(ClientSocket, SD_BOTH);
		closesocket(ClientSocket);
		//printf("[PLC TCP] Client disconnected\n");
	}

	closesocket(ListenSocket);
	WSACleanup();
	return 0;
}
