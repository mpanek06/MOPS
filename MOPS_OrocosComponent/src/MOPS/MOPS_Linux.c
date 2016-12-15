/**
 *	@file	MOPS_Linux.c
 *	@date	Jan 30, 2016
 *	@author	Michal Oleszczyk
 *	@brief	File containing functions responsible for
 *			communication between MOPS broker and local processes and for general borker logic.
 *
 *	Implementation for set of functions for broker-process communication
 *	and broker logic in general. Communication is based on queues mechanism.
 *	Every process is sending its process ID to queue named QUEUE_NAME (on Linux based target).
 */
#include "MOPS.h"
#include "MQTT.h"
#include "MOPS_RTnet_Con.h"

#include <sys/select.h>
#include <sys/un.h>
#include <mqueue.h>
#include <sys/mman.h>
#include <time.h>
#include <semaphore.h>

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <rtnet.h>
#include <rtmac.h>
#include <limits.h>


// *************** Global variables for MOPS broker *************** //
static MOPS_Queue proc_mops_queue;
mqd_t mq_listener;
// ***************   Funtions for local processes   ***************//

/**
 * @brief Function used in local processes to connect to the MOPS broker.
 *
 * For Linux target communication MOPS uses MQueues library but for RTnode -
 * Queue Management mechanism (user interface function).
 *
 * @return 0 - if connection succeed, 1 - if there was a problem with connection.
 */
int connectToMOPS(void) {
	uint8_t temp;
	mqd_t mq;
	struct mq_attr attr;
	char buffer[10] = { '/', 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	attr.mq_flags = 0;
	attr.mq_maxmsg = MAX_QUEUE_MESSAGE_NUMBER;
	attr.mq_msgsize = MAX_QUEUE_MESSAGE_SIZE;
	attr.mq_curmsgs = 0;
	startRandomGenrator();
	sprintf(buffer + 1, "%d", rand()%100000);

	mq = mq_open(QUEUE_NAME, O_WRONLY);
	if (!((mqd_t) -1 != mq)){
		perror("MQueue Open");
		return 1;
	}
	temp = strlen(buffer);
	buffer[temp] = 'a';
	proc_mops_queue.MOPSToProces_fd = mq_open(buffer, O_CREAT | O_RDONLY, 0644,
			&attr);
	if (!((mqd_t) -1 != proc_mops_queue.MOPSToProces_fd)){
		perror("MQueue Open MOPSToProces");
		return 1;
	}
	buffer[temp] = 'b';
	proc_mops_queue.ProcesToMOPS_fd = mq_open(buffer, O_CREAT | O_WRONLY, 0644,
			&attr);
	if (!((mqd_t) -1 != proc_mops_queue.ProcesToMOPS_fd)){
		perror("MQueue Open ProcesToMOPS");
		return 1;
	}
	buffer[temp] = 0;
	if (!(0 <= mq_send(mq, buffer, 10, 0))){
		perror("Send MQueue");
		return 1;
	}
	if (!((mqd_t) -1 != mq_close(mq))){
		perror("Close MQueue");
		return 1;
	}
	return 0;
}

/**
 * @brief Sends indicated buffer to connected MOPS broker (low level function).
 *
 * @param[in] buffer Contains data which will be sent to the connected broker.
 * @param[in] buffLen Specifies number of bytes from buffer which will be sent.
 * @return Number of bytes actually sent.
 */
int sendToMOPS(char *buffer, uint16_t buffLen) {
	return mq_send(proc_mops_queue.ProcesToMOPS_fd, (char*) buffer, buffLen, 0);
}

/**
 * @brief Receive data from MOPS broker (low level function).
 *
 * @param[out] buffer Container for data received from broker.
 * @param[in] buffLen Define number of bytes which can be stored in buffer.
 * @return Number of bytes actually written.
 */
int recvFromMOPS(char *buffer, uint16_t buffLen) {
	return mq_receive(proc_mops_queue.MOPSToProces_fd, buffer, buffLen, NULL);
}

// ***************   Funtions for local processes   ***************//

// ***************   Funtions for MOPS broker   ***************//
/**
 * @brief Adding new local process to broker communication queue.
 *
 * For communication process<->broker are used one direction queues.
 * File descriptors for that queues (2 for each process) are stored in
 * "communication list".
 *
 * @param[in] MOPS_Proces_fd File descriptor for a queue MOPS->process.
 * @param[in] Proces_MOPS_fd File descriptor for a queue process->MOPS.
 * @return Client ID, which is also index of communication list if
 * there was enough place to add new process connection.\n
 * -1 - if there was no place to add new process connection.
 * @post One place in 'communication list' less.
 */
int AddToMOPSQueue(int MOPS_Proces_fd, int Proces_MOPS_fd) {
	int i = 0;
	for (i = 0; i < MAX_PROCES_CONNECTION; i++)
		if (mops_queue[i].MOPSToProces_fd == 0
				&& mops_queue[i].ProcesToMOPS_fd == 0) {

			mops_queue[i].MOPSToProces_fd = MOPS_Proces_fd;
			mops_queue[i].ProcesToMOPS_fd = Proces_MOPS_fd;

			return i;
		}
	return -1;
}


/**
 * @brief Main function for setting processes<->broker communication.
 *
 * This is place where initial queue processes->broker is created.
 * Broker is listening on this queue and is adding new connections to his
 * 'communication list'. Functionality is based on select(). Function
 * is target sensitive.
 *
 * @post This is blocking function (never ending loop)!
 */
void InitProcesConnection() {
	mqd_t new_mq_Proces_MOPS;
	struct mq_attr attr;
	int fdmax, rv, i;
	fd_set master, read_fd; //master fd list, temp fd list for select()
	FD_ZERO(&master);
	FD_ZERO(&read_fd);

	/* initialize the queue attributes */
	attr.mq_flags = 0;
	attr.mq_maxmsg = MAX_QUEUE_MESSAGE_NUMBER;
	attr.mq_msgsize = MAX_QUEUE_MESSAGE_SIZE;
	attr.mq_curmsgs = 0;

	mq_listener = mq_open(QUEUE_NAME, O_RDONLY, 0644, &attr);

	if (-1 != mq_listener) 	//there was such QUEUE before - other broker is runnning
	{
		printf("MQueue exists! Perhaps another broker is runnning?\n");
		return;
	}

	mq_listener = mq_open(QUEUE_NAME, O_CREAT | O_RDONLY, 0644, &attr);
	
	if (-1 == mq_listener)
	{
		perror("mq_open");
		return;
	}
	
	FD_SET(mq_listener, &master);
	fdmax = mq_listener;
	for (;;) {
		read_fd = master;
		rv = select(fdmax + 1, &read_fd, NULL, NULL, NULL);
		if (rv > 0) { // there are file descriptors to serve
			for (i = 0; i <= fdmax; i++) {
				if (FD_ISSET(i, &read_fd)) {
					if (i == mq_listener) {
						new_mq_Proces_MOPS = ServeNewProcessConnection(&master,
								mq_listener);
						if (new_mq_Proces_MOPS > fdmax)
							fdmax = new_mq_Proces_MOPS;
					} else {
						ReceiveFromProcess(i);
					}
				}
			}
		}
		
		if (rv < 0) // error occurred in select()
			perror("select");
	}
}

/**
 * @brief Receiving data from connected local processes.
 *
 * This is high level function used for react when select() function
 * return that file descriptor (file_de variable) if ready to read some data.
 *
 * @param[in] file_de File descriptor of queue from which data can be read.
 * @return 0 - in every case (still TODO).
 */
int ReceiveFromProcess(int file_de) {
	int bytes_read, ClientID;
	uint8_t temp[MAX_QUEUE_MESSAGE_SIZE + 1];

	bytes_read = mq_receive(file_de, (char*) temp, MAX_QUEUE_MESSAGE_SIZE, NULL);
	if (bytes_read == -1) {
		CloseProcessConnection(file_de);
	}
	if (bytes_read >= sizeof(FixedHeader)) {
		ClientID = FindClientIDbyFileDesc(file_de);
		AnalyzeProcessMessage(temp, bytes_read, ClientID);
	}
	return 0;
}

/**
 * @brief Sending data from broker to particular file descriptor.
 *
 * Function sends buffer of given length to given file descriptor.
 * This is very low level function. It is target sensitive.
 *
 * @param[in] buffer Buffer of data to send.
 * @param[in] buffLen Buffer length.
 * @param[in] file_de File descriptor, place where data should be sent.
 * @return Number of bytes properly sent.\n
 * 0 - if queue is full
 */
int SendToProcess(uint8_t *buffer, uint16_t buffLen, int file_de) {
	struct mq_attr attr;
	attr.mq_flags = 0;
	attr.mq_maxmsg = MAX_QUEUE_MESSAGE_NUMBER;
	attr.mq_msgsize = MAX_QUEUE_MESSAGE_SIZE;
	attr.mq_curmsgs = 0;

	mq_getattr(file_de, &attr);
	if (attr.mq_curmsgs < MAX_QUEUE_MESSAGE_NUMBER){
		return mq_send(file_de, (char*) buffer, buffLen, 0);
	}
	return 0;
}

/**
 * @brief Main place where new connection from processes to broker are serve.
 *
 * Function is fired when file descriptors on which broker is listening new
 * connections is set.
 *
 * @param[in,out] set Set of file descriptors which will be extended for new one needed
 * for new process.
 * @param[in] listener_fd File descriptor on which broker is listening for new connections.
 * @return File descriptor value - when there is place in MOPSQueue array ('connection list')\n
 * 	-1 - if there is no place in MOPSQueue array or no message received from listener_fd
 */
int ServeNewProcessConnection(fd_set *set, int listener_fd) {
	uint8_t buffer[MAX_QUEUE_MESSAGE_SIZE + 1], temp;
	int new_mq_Proces_MOPS, new_mq_MOPS_Proces;

	memset(buffer, 0, MAX_QUEUE_MESSAGE_SIZE + 1);
	if (mq_receive(listener_fd, (char*) buffer, MAX_QUEUE_MESSAGE_SIZE, NULL) > 0) {
		temp = strlen((char*) buffer);
		buffer[temp] = 'b';
		new_mq_Proces_MOPS = mq_open((char*) buffer, O_RDONLY);
		if (!((mqd_t) -1 != new_mq_Proces_MOPS))
			perror("MQueue Open Proces_MOPS");

		buffer[temp] = 'a';
		new_mq_MOPS_Proces = mq_open((char*) buffer, O_WRONLY);
		if (!((mqd_t) -1 != new_mq_MOPS_Proces))
			perror("MQueue Open MOPS_Proces");

		if (AddToMOPSQueue(new_mq_MOPS_Proces, new_mq_Proces_MOPS) >= 0) {
			FD_SET(new_mq_Proces_MOPS, set);
			// printf("Nowy deskryptor: %d, nazwa kolejki: %s \n", new_mq_Proces_MOPS, buffer);
			return new_mq_Proces_MOPS;
		}
	}
	return -1;
}

/**
 * @brief Deleting not needed anymore connections from 'connection list'.
 *
 * File descriptors stored in 'connection list' are erased (set to 0) for
 * given client.
 *
 * @param[in] ClientID ID of client for which connection should be closed.
 * @param[out] queue List of communication structure where particular
 * communication was stored.
 * @post One more free space in 'communication list'.
 */
void DeleteProcessFromQueueList(int ClientID, MOPS_Queue *queue) {
	mq_close(queue[ClientID].MOPSToProces_fd);
	mq_close(queue[ClientID].ProcesToMOPS_fd);

	queue[ClientID].MOPSToProces_fd = 0;
	queue[ClientID].ProcesToMOPS_fd = 0;
}

// ***************   Tools functions   ******************************//
void MOPSBrokerTargetInit(void){
	mlockall(MCL_CURRENT | MCL_FUTURE);
}

/**
 * @brief Stops MOPS Broker 
 * Should be called when stoping brocker - unlinks global message queue.
 */
int StopMOPSBroker(void){
	mq_unlink(QUEUE_NAME);
	return 0;
}

/**
 * @brief Waits for TDMA sync signal - overlaps rt_dev_ioctl for Linux
 *
 */
uint8_t waitOnTDMASync(void){
	if(0 == rt_dev_ioctl(TDMA_Dev, RTMAC_RTIOC_WAITONCYCLE, (void*) TDMA_WAIT_ON_SYNC)){
		return 1;
	}
	return 0;
}

/**
 * @brief Initializes RTnet connection
 * Makes some target dependent initialisation - it this case opens TDMA0 device.
 * return descriptor returned by rt_dev_open function
 */
uint8_t RTnetConnTargetDependentInit(void){
	TDMA_Dev = rt_dev_open("TDMA0", O_RDWR);
	
	return TDMA_Dev;
}

/**
 * @brief Starts random number generator 
 * Starts random number generator and initializes it with microseconds part of surrent time.
 */
void startRandomGenrator(void){
	struct timeval tv;
	gettimeofday(&tv,NULL);
	
	srand(tv.tv_usec);
}
// ***************   Tools functions   ******************************//