#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "MOPS.h"

int main(void)
{
    int s;
    char array[100];
    char *topic[2]={"Jakis_tam_topic", "Jakis"};
    uint8_t Qos[]={1, 2};

	s = connectToMOPS();
	subscribeMOPS(topic, Qos, 2);
	for(;;){
		readMOPS(array, 100);
		printf("%s\n", array);
	}
    return 0;
}