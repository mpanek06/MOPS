/*
 * local_broker.c
 *
 *  Created on: Jan 20, 2016
 *      Author: rudy
 */

#include <signal.h>
#include <stdlib.h>

#include "MOPS.h"

void sigHandler(int sig, siginfo_t *si, void *unused)
{
	if(SIGINT == sig)
	{
		StopMOPSBroker();
	}
	exit(0);
}

int main(void)
{
	struct sigaction act;
	act.sa_sigaction = sigHandler;
	act.sa_flags = SA_SIGINFO;

	if (sigaction(SIGINT, &act, NULL) < 0) 
	{
		perror ("sigaction");
		return 1;
	}
	
	StartMOPSBroker();
	return 0;
}
