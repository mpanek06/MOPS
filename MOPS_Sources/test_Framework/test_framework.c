/**
 * @file test_framework.c
 * @author  Marcin Panek
 * @brief   File containing implemantation of simple test framework for MOPS protocole.
 */

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

/**
 * @brief Calculates average RTT (Round Trip Time)and prints outcome on std.
 */
void printStats(void);

/**
 * @brief Returns current time in miliseconds.
 * Function returns current time in miliseconds. It is used to mesure RTT.
 * @return time in miliseconds.
 */
long long getCurrentTimeMs(void);

/**
 * @brief Signal handler for SIGINT
 * 
 * Signal is captured in order to properly kill threads and print stats.
 *
 * @param[in] sig signal number
 * @param[in] si siginfo_t structure - according to POSIX standard
 * @param[in] unused unused according to POSIX standard
 */
void sigHandler(int sig, siginfo_t *si, void *unused);

/**
 * @brief Subscribing function
 * Subscribes data- running in a separate thread.
 */
void *SubFun();

/**
 * @brief Callback function called when test message is received
 * Saves data and time of packet arrival.
 * @param[in] msg incoming data
 */
void clbFun(void* msg);

/**
 * @brief Publish function
 * Function publishng data - running in a separate thread.
 * Sends data in a fixed formar - eight digits and saves.
 */
void *PubFun();

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

		else if(argv[1][0] == '-' && argv[1][1] == 'h')
		
		{
			printf("Simple test framework for MOPS protocole\n");
			printf("\n");
			printf("\n");
			printf("Usage:  ./TestFramework.out -v no_of_data_to_sent interval\n");
			printf("\n");
			printf("-v                   verbose mode\n");
			printf("no_of_data_to_sent   number of packets to be sent\n");
			printf("interval             interval between packets\n");
			printf("\n");
			printf("Example  ./TestFramework.out -v 200 5000\n");
			return 0;
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

	if(pthread_create(&pub_thread, NULL, PubFun, NULL) || pthread_create(&sub_thread, NULL, SubFun, NULL))
    {
       	perror("Error - pthread_create() return code:");
       	return 1;
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

			break;
    	}
    }
	return 0;
}


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
      		++noOfCorruptedData;
      		deleteByValue(&sent, tmpPacketData);
      	}

      	sent_ptr = sent_ptr->next;
   	}

   	printf("Average time: %f, Sent data: %ld, Lost data: %ld Lost data(percent): %.2f%% \n",average, noOfSentData, noOfCorruptedData-1, 100*((double)(noOfCorruptedData-1)/(double)noOfSentData) );
   	fprintf(stderr, "Average time: %f, Sent data: %ld, Lost data: %ld Lost data(percent): %.2f%% \n",average, noOfSentData, noOfCorruptedData-1, 100*((double)(noOfCorruptedData-1)/(double)noOfSentData) );	
}


long long getCurrentTimeMs(void)
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
	connectToMOPS();
	subscribeMOPS("node_pub", 0, clbFun);

	for(;;)
	{
		if(endSub)
			break;
		else
		{
			spinMOPS();
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
			sentTime = getCurrentTimeMs();
			insertFirst(&sent, i, sentTime);
			++noOfSentData;
			usleep(interval);
		}

	}
	return 0;
}

void clbFun(void* msg)
{
	int receivedDataInt = 0;
	long recTime = 0;

	recTime = getCurrentTimeMs();
	receivedDataInt = atoi((char*)msg);
	insertFirst(&received, receivedDataInt, recTime);
}