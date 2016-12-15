#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "MOPS.h"

int main(void)
{
	if(connectToMOPS())
	{
		printf("Error during connecting to MOPS broker!\n");
		return -1;
	}

	PublishHandler pub = advertiseMOPS("testpub");
	
	for(;;)
	{
		pub.publish("test", &pub);
		usleep(300000);
	}
    return 0;
}
