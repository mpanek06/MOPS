#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "MOPS.h"


void clback1(void* msg)
{
	char *data = (char*) msg;
	printf("MSG dla tematu node_sub: %s\n", data);
}

void clback2(void* msg)
{
	char *data = (char*) msg;
	printf("MSG dla tematu testpub: %s\n", data);
}


void clback3(void* msg)
{
	char *data = (char*) msg;
	printf("MSG dla tematu node_pub: %s\n", data);
}

int main(void)
{
	if(connectToMOPS())
	{
		printf("Error during connecting to MOPS broker!\n");
		return -1;
	}

	subscribeMOPS("node_sub", 0, clback1);
	subscribeMOPS("testpub",  0, clback2);
	subscribeMOPS("node_pub", 0, clback3);
	
	spinMOPS();
	
    return 0;
}