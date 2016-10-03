#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "MOPS.h"

int main(void)
{
    char array[100];
    uint8_t Qos[]={1, 2};

	connectToMOPS();
	for(;;){
		usleep(300000);
		publishMOPS("Jakis", "Pierwsza wiadomosc", 10);
	}
    return 0;
}
