#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "MOPS.h"

int main(void)
{
    char array[100];

	connectToMOPS();
	subscribeOnceMOPS("node_pub", 0);
	
	for(;;){
		readMOPS(array, 100);
		printf("%s\n", array);
	}
    return 0;
}