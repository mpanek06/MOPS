#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "MOPS.h"


void clback1(void* msg){
	char *data = (char*) msg;
	printf("MSG dla tematu node_sub: %s\n", data);
}

void clback2(void* msg){
	char *data = (char*) msg;
	printf("MSG dla tematu testpub: %s\n", data);
}

int main(void)
{
    char array[100];

	connectToMOPS();

	subscribeMOPS2("testpub", 0, clback2);
	subscribeMOPS2("node_sub", 0, clback1);
	
	for(;;){
		readMOPS2(array, 100);
	}
	
    return 0;
}