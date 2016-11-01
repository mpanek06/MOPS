#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#include "MOPS.h"
#include "list.h"

#define INTERVAL 5000

long long sentTime = 0, recTime = 0;
long no_of_data = 0;
double average = 0;
long sum = 0;

bool endProgramSigRec = false, endPub = false, endSub = false;

pthread_t pub_thread, sub_thread;
pthread_mutex_t listLock;

my_node *received = NULL;
my_node *sent = NULL;

long long getCurrentTimeNs(void)
{
    struct timeval te; 
    gettimeofday(&te, NULL);
    return (te.tv_sec*1000LL + te.tv_usec/1000);
}

void sigHandler(int signo)
{
	endProgramSigRec = true;
}

void *SubFun()
{
    char receivedData[100];
    int receivedDataInt = 0;

	connectToMOPS();
	subscribeOnceMOPS("node_pub", 0);

	for(;;)
	{
		if(endSub)
			break;
		else
		{
			readMOPS(receivedData, 100);
			recTime = getCurrentTimeNs();
			receivedDataInt = atoi(receivedData);
			
			// insertFirst(received, receivedDataInt, recTime);

			// if(NULL != nodePtr)
			// {
			// 	//Recalculating average time
			// 	sum = ((average * no_of_data) + (recTime-sentTime));
			// 	++no_of_data;
			// 	average = (double) sum / (double)no_of_data;
				
			// 	printf("received data: %s\n", receivedData);
			// 	printf("%f %ld %lld\n", average, no_of_data, recTime-sentTime);
				
			// 	pthread_mutex_lock(&listLock);
			// 	deleteByValue(receivedDataInt);
			// 	pthread_mutex_unlock(&listLock);
			// }
			// else //no such data was sent by publisher so rec data must be corrupted
			// {
			// 	printf("corrupted data: %s\n", receivedData);
			// }
		

		} 
	}
	return 0;
}

void *PubFun()
{
	connectToMOPS();
	char buff[100];
	PublishHandler pub = advertiseMOPS("node_sub");
	int i = 0;
	
	for(;;)
	{
		if(endPub)
			break;
		else
		{
			memset(buff, 0, sizeof(buff));
			sprintf(buff, "%d", i++);

			pub.publish(buff, &pub);
			sentTime = getCurrentTimeNs();
			insertFirst(&sent, i, sentTime);
			if(sent == NULL)
			{
				printf("sent jest NULLem");
			}else
				printf("dane ostatio dodoana: %d", sent->packetData);
			usleep(INTERVAL);
		}

	}
	return 0;
}

int main(void)
{
	pthread_mutex_init(&listLock, NULL);
	
	struct sigaction act;
	act.sa_sigaction = &sigHandler;
	act.sa_flags = SA_SIGINFO;

	if (sigaction(SIGINT, &act, NULL) < 0) 
	{
		perror ("sigaction");
		return 1;
	}
	
    if(pthread_create(&pub_thread, NULL, PubFun, NULL) || pthread_create(&sub_thread, NULL, SubFun, NULL))
    {
        perror("Error - pthread_create() return code:");
        return 1;
    }

    while(1){

    	if(endProgramSigRec)
    	{
			printf("\nEnd of test\n");
			endPub = true;
			usleep(INTERVAL*40);
			endSub = true;
			// printStats();
			// 
			printList(sent);
			// printList(received);
			break;
    	}
    }
	return 0;
}
