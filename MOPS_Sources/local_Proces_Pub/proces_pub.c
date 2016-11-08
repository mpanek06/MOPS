#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "MOPS.h"

int main(void)
{
	connectToMOPS();

	PublishHandler pub = advertiseMOPS("node_sub");
	
	for(;;){
		usleep(300000);
		pub.publish("test", &pub);
	}
    return 0;
}
