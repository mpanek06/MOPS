#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

#include "MOPS.h"

long long sent_time = 0;
long long rec_time = 0;
long no_of_data = 0;
double average = 0;

long sum = 0;

long long get_current_time_ns(void)
{
    struct timeval te; 
    gettimeofday(&te, NULL);
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
    return milliseconds;
}

void *sub_fun()
{
	printf("Sub_fun starting:\n");

	char array[100];
    char tmp[100];
    int i = 0;

	connectToMOPS();
	subscribeOnceMOPS("node_pub", 0);

	for(;;)
	{
		memset(tmp, 0, sizeof(tmp));
		sprintf(tmp, "%d", i++);

		readMOPS(array, 100);
		rec_time = get_current_time_ns();
		printf("rec: %s\n", array);

		sum = ((average * no_of_data) + (rec_time-sent_time));

		++no_of_data;
		
		average = (double) sum / (double)no_of_data;

		printf("%f %ld %lld\n", average, no_of_data, rec_time-sent_time);
	}
	return 0;
}

void *pub_fun()
{
	printf("Pub_fun starting: \n");

	connectToMOPS();
	char buff[100];
	PublishHandler pub = advertiseMOPS("node_sub");
	int i = 0;
	
	for(;;)
	{
		memset(buff, 0, sizeof(buff));
		sprintf(buff, "%d", i++);

		pub.publish(buff, &pub);
		sent_time = get_current_time_ns();
		usleep(5000);
	}
	return 0;
}

int main(void)
{
	pthread_t pub_thread, sub_thread;
	
    if(pthread_create(&pub_thread, NULL, pub_fun, NULL) || pthread_create(&sub_thread, NULL, sub_fun, NULL))
    {
        perror("Error - pthread_create() return code:");
        exit(EXIT_FAILURE);
    }

    pthread_join(sub_thread, NULL);
    pthread_join(pub_thread, NULL);

	return 0;
}
