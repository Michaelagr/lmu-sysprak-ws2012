#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include "util.h"
#include "network.h"

int SOCKET = -1;
struct game_state *GAME_STATE = (struct game_state *) -1;

void usage(int argc, char *argv[]) {
	// how to use this program
	printf("USAGE: %s <gid>\n", argv[0]);
	printf("  gid: 13-digit game-id without spaces\n");
}

int main(int argc, char *argv[]) {
	int shmid = 0;
	
	// "validate" first parameter: game id (gid)
	if(argc != 2) {
		usage(argc, argv);
		die("Error! This program needs exactly one parameter!", EXIT_FAILURE);
	}
	if(strlen(argv[1]) != 13) {
		usage(argc, argv);
		die("Error! The game id (gid) has to be exactly 13 digits!", EXIT_FAILURE);
	}
	
	// allocate shared memory for global GAME_STATE struct
	// (maybe we should not use 0x23421337 here as SHM key?)
	if((shmid = shmget(ftok("client.c", 0x23421337), sizeof(struct game_state), IPC_CREAT | IPC_EXCL | 0666)) < 0) {
		die("Could not get shared memory!", EXIT_FAILURE);
	}
	GAME_STATE = (struct game_state *) shmat(shmid, NULL, 0);
	if(GAME_STATE == (struct game_state *) -1) {
		// at this point our die() function can't figure out that the SHM was already
		// created, so we explicitly delete it here ...
		shmctl(shmid, IPC_RMID, 0);
		die("Could not attach shared memory!", EXIT_FAILURE);
	}
	GAME_STATE->shmid = shmid;
	strcpy(GAME_STATE->game_id, argv[1]);
	
	// open connection (i.e. socket + tcp connection)
	openConnection();
	// perform PROLOG phase of the protocol
	performConnection();
	
	cleanup();
	return EXIT_SUCCESS;
}