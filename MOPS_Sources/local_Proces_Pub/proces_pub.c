#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "MOPS.h"

int main(void)
{
	connectToMOPS();
	for(;;){
		usleep(300000);
		publishMOPS("Jakis", "Pierwsza wiadomosc", 18);
	}
    return 0;
}
