#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

#include "MOPS.h"
#include "list.h"

#define INTERVAL 5000

int verbose = 0;
int i = 0;
long noOfData = 0, noOfCorruptedData = 0, noOfSentData = 0;
double average = 0;
long sum = 0;

long interval = 0;

bool endProgramSigRec = false, endPub = false, endSub = false;

pthread_t pub_thread, sub_thread;
pthread_mutex_t listLock;

listNode *received = NULL;
listNode *sent = NULL;

void printStats()
{
	long long sentTime = 0, recTime = 0;
	int tmpPacketData = 0;
	listNode *sent_ptr = sent;
	listNode *rec_ptr;

	while(NULL != sent_ptr)
	{
		tmpPacketData = sent_ptr->packetData;
		sentTime = sent_ptr->timestamp;
		rec_ptr = findByValue(&received, tmpPacketData);

		if(NULL != rec_ptr)
		{
			recTime = rec_ptr->timestamp;
			sum += (recTime-sentTime);

			if(verbose)
			{
				printf("%lld %lld %lld \n", sentTime, recTime, recTime-sentTime);
			}


			++noOfData;
			average = (double) sum / (double)noOfData;

			deleteByValue(&received, tmpPacketData);
			deleteByValue(&sent, tmpPacketData);
		}
      	else
      	{
      		// nie ma takiego w odbernaych wiec zgubiona paczka
      		++noOfCorruptedData;
      		deleteByValue(&sent, tmpPacketData);
      	}

      	sent_ptr = sent_ptr->next;
   	}

   	printf("Average time: %f, Sent data: %ld, Lost data: %ld Lost data(percent): %.2f%% \n",average, noOfSentData, noOfCorruptedData-1, 100*((double)(noOfCorruptedData-1)/(double)noOfSentData) );
   	fprintf(stderr, "Average time: %f, Sent data: %ld, Lost data: %ld Lost data(percent): %.2f%% \n",average, noOfSentData, noOfCorruptedData-1, 100*((double)(noOfCorruptedData-1)/(double)noOfSentData) );	
}


long long getCurrentTimeNs(void)
{
    struct timeval te; 
    gettimeofday(&te, NULL);
    return (te.tv_sec*1000LL + te.tv_usec/1000);
}

void sigHandler(int sig, siginfo_t *si, void *unused)
{
	if(SIGINT == sig)
	{
		endProgramSigRec = true;
	}
}

void *SubFun()
{
    char receivedData[100];
    int receivedDataInt = 0;
	long recTime = 0;

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
	char buff[100];
	i = 0;
	long sentTime = 0;

	PublishHandler pub = advertiseMOPS("node_sub");
	connectToMOPS();
	
	for(;;)
	{
		if(endPub)
			break;
		else
		{
			memset(buff, 0, sizeof(buff));
			sprintf(buff, "%08d", i++);
			
			if(verbose)
			{
				printf("%s\n", buff);
			}

			pub.publish(buff, &pub);
			sentTime = getCurrentTimeNs();
			insertFirst(&sent, i, sentTime);
			// printf("dodane: %lld \n", sent->timestamp );
			++noOfSentData;
			usleep(interval);
		}

	}
	return 0;
}

void *Pub2Fun()
{
	char buff[100];
	i = 0;
	long sentTime = 0;

	connectToMOPS();
	
	for(;;)
	{
		if(endPub)
			break;
		else
		{
			memset(buff, 0, sizeof(buff));
			sprintf(buff, "%08d", i++);
			
			if(verbose)
			{
				printf("%s\n", buff);
			}
			
			publishMOPS(buff, buff, strlen(buff));
			publishMOPS(buff, buff, strlen(buff));
			
			sentTime = getCurrentTimeNs();
			insertFirst(&sent, i, sentTime);
			// printf("dodane: %lld \n", sent->timestamp );
			++noOfSentData;
			usleep(interval);
		}

	}
	return 0;
}

int main(int argc, char **argv)
{

	int no_of_data_to_sent;

	if(argc > 1)
	{
		if(argv[1][0] == '-' && argv[1][1] == 'v')
		{
			verbose = 1;
			printf("verbose mode on!\n");
		}
	}

	no_of_data_to_sent = atoi(argv[2]);
	interval = atoi(argv[3]);

	pthread_mutex_init(&listLock, NULL);
	
	struct sigaction act;
	act.sa_sigaction = sigHandler;
	act.sa_flags = SA_SIGINFO;

	if (sigaction(SIGINT, &act, NULL) < 0) 
	{
		perror ("sigaction");
		return 1;
	}


	if(argc > 4 && argv[4][0] == 'p')
	{
		printf("New topic each time mode active!\n");
		if(pthread_create(&pub_thread, NULL, Pub2Fun, NULL) || pthread_create(&sub_thread, NULL, SubFun, NULL))
    	{
        	perror("Error - pthread_create() return code:");
        	return 1;
    	}

	}
	else 
	{
		if(pthread_create(&pub_thread, NULL, PubFun, NULL) || pthread_create(&sub_thread, NULL, SubFun, NULL))
    	{
        	perror("Error - pthread_create() return code:");
        	return 1;
    	}
	}

    while(1){

    	if(i >= no_of_data_to_sent)
    	{
    		endProgramSigRec = true;
    	}

    	if(endProgramSigRec)
    	{
			printf("\nEnd of test\n");
			endPub = true;
			usleep(INTERVAL*80);
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
