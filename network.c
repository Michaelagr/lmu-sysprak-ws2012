#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

#include "network.h"
#include "util.h"
#include "debug.h"

void _receiveLine(int sock, char **buf) {
	size_t length;
	FILE *fd = fdopen(sock, "r");
	if(getline(buf, &length, fd) <= 0) {
		die("Could not read from socket, I guess the network connection died!", EXIT_FAILURE);
	}
	DEBUG("_receiveLine: %d %d '%s'\n", (int) length, (int) strlen(*buf), *buf);
}

char *recvLine(int sock) {
	char *buf = NULL;
	_receiveLine(sock, &buf);
	if(buf[0] != '+') {
		printf("Gameserver Error: %s", buf);
		die("Fatal gameserver error!", EXIT_FAILURE);
	}
	return buf;
}

void sendLine(int sock, const char* format, ...) {
	char buf[1024];
	va_list argptr;
	va_start(argptr, format);
	vsnprintf(buf, sizeof(buf), format, argptr);
	va_end(argptr);
	DEBUG("sendLine: %d '%s'\n", (int) strlen(buf), buf);
	if(send(sock, buf, strlen(buf), 0) < 0) {
		die("Could not write to socket, I guess the network connection died!", EXIT_FAILURE);
	}
}

void dumpLine(int sock) {
	char *buf = recvLine(sock);
	free(buf);
}

void cmdVERSION(int sock) {
	sendLine(sock, "VERSION 1.0\n");
	dumpLine(sock);
}

void cmdID(int sock, char *game_id) {
	sendLine(sock, "ID %s\n", game_id);
	char *buf = recvLine(sock);
	if(strcmp(buf, "+ PLAYING " GAMEKINDNAME "\n") != 0) {
		die("Gameserver sent wrong game kind!", EXIT_FAILURE);
	}
	free(buf);
	dumpLine(sock);
	dumpLine(sock);
}

void openConnection() {
	struct hostent *host_info;
	struct sockaddr_in server;
	if((host_info = gethostbyname(HOSTNAME)) == NULL) {
		die("Could not resolve hostname of the gameserver!", EXIT_FAILURE);
	};
	if((SOCKET = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		die("Could not open socket!", EXIT_FAILURE);
	};
	memcpy( (char *)&server.sin_addr, host_info->h_addr_list[0], host_info->h_length );
	server.sin_port = htons(PORTNUMBER);
	server.sin_family = PF_INET;
	if(connect(SOCKET, (struct sockaddr *) &server, sizeof(server)) < 0) {
		die("Could not connect to gameserver!", EXIT_FAILURE);
	}
}

void performConnection() {
	dumpLine(SOCKET);
	cmdVERSION(SOCKET);
	cmdID(SOCKET, GAME_STATE->game_id);
}
