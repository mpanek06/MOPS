#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "MOPS.h"


void clback1(char* msg){
	printf("MSG dla tematu node_sub%s\n", msg);
}

void clback2(char* msg){
	printf("MSG dla tematu testpub: %s\n", msg);
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