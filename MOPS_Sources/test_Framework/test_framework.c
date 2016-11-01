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
long noOfData = 0, noOfCorruptedData = 0, noOfSentData = 0;
double average = 0;
long sum = 0;

bool endProgramSigRec = false, endPub = false, endSub = false;

pthread_t pub_thread, sub_thread;
pthread_mutex_t listLock;

my_node *received = NULL;
my_node *sent = NULL;

void printStats()
{
	int tmpPacketData = 0;
	my_node *sent_ptr = sent;
	my_node *rec_ptr;

	while(NULL != sent_ptr)
	{
		tmpPacketData = sent_ptr->packetData;
		sentTime = sent_ptr->timestamp;
		rec_ptr = findByValue(&received, tmpPacketData);

		if(NULL != rec_ptr)
		{
			recTime = rec_ptr->timestamp;
			sum += (recTime-sentTime);
			++noOfData;
			average = (double) sum / (double)noOfData;

			printf("suma: %d, srednia: %f \n", sum, average );

			deleteByValue(&received, tmpPacketData);
			deleteByValue(&sent, tmpPacketData);

		}
      	else
      	{
      		// nie ma takiego w odbernaych wiec zgubiona paczka
      		++noOfCorruptedData;
      		printf("corrupted: %d, noOfCorruptedData\n", tmpPacketData, noOfCorruptedData);
      		deleteByValue(&sent, tmpPacketData);
      	}

      	sent_ptr = sent_ptr->next;
   	}

   	printf("Average time: %f, Sent data: %d, Lost data: %d Lost data[%]: %f% \n",average, noOfSentData, noOfCorruptedData, 100*((double)noOfCorruptedData/(double)noOfSentData) );
		
}


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
			
			insertFirst(&received, receivedDataInt, recTime);
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
			printf("dodane: %lld \n", sent->timestamp );
			++noOfSentData;
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
			printStats();
			// 
			// printList(&sent);
			// printList(&received);
			break;
    	}
    }
	return 0;
}
