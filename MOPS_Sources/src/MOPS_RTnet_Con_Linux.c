/**
 *	@brief	File containing function responsible for
 *			communication between MOPS brokers in RTnet.
 *
 *	Implementation for set of function for broker-broker communication.
 *	Communication is based on UDP transfer. Every broker is sending its
 *	UDP packet to broadcast address and to yourself on port 1883.
 *
 *	@file	MOPS_RTnet_Con.c
 *	@date	Jan 30, 2016
 *	@author	Michal Oleszczyk
 */
#include "MOPS.h"
#include "MOPS_RTnet_Con.h"

#if TARGET_DEVICE == Linux
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/mman.h>
#endif //TARGET_DEVICE == Linux

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#include <pthread.h>
#include <rtnet.h>
#include <rtmac.h>
#include <unistd.h>
#include <limits.h>


static struct sockaddr_in rec_addr; /**< Struct containing socket address for receiving. */
static struct sockaddr_in sd_addr_b;/**< Struct containing socket address for sending broadcast. */
int get_sock; /**< Socket for receiving packet from RTnet. */
int bcast_sock; /**< Socket for broadcasting packets to RTnet. */

/**
 *	@brief	Setting all required variable for connection.
 *
 *	Function sets global variables responsible for Ethernet communication
 *	(rec_addr, sd_addr_b, sd_addr_l). Moreover here two sockets are created:
 *	get_sock, bcast_sock. First one is for listening incoming pockets, second
 *	one is used for outgoing transfer.
 *
 *	@pre	Nothing.
 *	@post	Changing values of globla variables: rec_addr, sd_addr_b, sd_addr_l, get_sock, bcast_sock.
 */
void connectToRTnet(){
    if ((bcast_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        perror("socket");
    if ((get_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
            perror("socket");

    memset(&rec_addr, 0, sizeof(rec_addr));
    memset(&sd_addr_b, 0, sizeof(sd_addr_b));

    rec_addr.sin_family = AF_INET;
    rec_addr.sin_port = htons(MOPS_PORT);
    rec_addr.sin_addr.s_addr =  htonl(INADDR_ANY);//inet_addr("10.0.0.0");

    sd_addr_b.sin_family = AF_INET;
    sd_addr_b.sin_port = htons(MOPS_PORT);
    sd_addr_b.sin_addr.s_addr =  inet_addr(IPADDR);

	if (bind(get_sock, (struct sockaddr*)&rec_addr, sizeof(rec_addr))==-1)
		perror("bind");
}

/**
 * @brief	Sending buffer to RTnet.
 * @param buf	This a buffer containing data which should be send to other MOPS brokers.
 * @param buflen	Length of buffer in bytes for sending.
 *
 * @pre	Nothing.
 * @post Data are send as a broadcast UDP frame into RTnet and also to myself.
 */
void sendToRTnet(uint8_t *buf, int buflen){
	int write = 0;
	socklen_t len = sizeof(sd_addr_b);
    if((write = sendto(bcast_sock, buf, buflen, 0, (struct sockaddr*)&sd_addr_b, len)) < 0)
        perror("sendto");
}

/**
 * @brief	Receiving data from RTnet.
 * @param buf Destination where received data will be stored.
 * @param buflen Maximum length of buffer (in bytes) which can be overridden.
 * @return Actual number of overridden bytes in buffer (amount of written bytes).
 *
 * @post buf variable has been filled with incoming data.
 */
int receiveFromRTnet(uint8_t *buf, int buflen){
	int written = 0;
	socklen_t len = sizeof(rec_addr);
	written = recvfrom(get_sock, buf, buflen, 0, (struct sockaddr*)&rec_addr, &len);
	return written;
}

/**
 * @brief Enable starting of some function in separated thread.
 * @param start_routine Pointer to a function which will be  started as a new thread.
 * @param arg Pointer to a struct containing arguments for function which will be started.
 * @return ID of new thread.
 *
 * @post New thread has been started.
 */
pthread_t startNewThread(void *(*start_routine) (void *), void *arg){
	int err;
	pthread_t thread_id;
    err = pthread_create(&thread_id, NULL, start_routine, arg);
    if (err != 0)
        printf("\ncan't create thread :[%s]", strerror(err));
    return thread_id;
}

/**
 * @brief Initiation o mutex.
 * @param lock Pointer to mutex we want to be initiated.
 * @return 1 if initiation failed, 0 if everything goes fine.
 *
 * @post Pointed mutex if ready to use.
 */
uint8_t mutex_init(pthread_mutex_t *lock){
    if (pthread_mutex_init(lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
    return 0;
}

/**
 * @brief Locking mutex.
 * @param lock Pointer to mutex we want to locked.
 *
 * @post Pointed mutex is locked.
 */
void lock_mutex(pthread_mutex_t *lock){
	pthread_mutex_lock(lock);
}

/**
 * @brief Unlocking mutex.
 * @param lock Pointer to mutex we want to unlock.
 *
 * @post Pointed mutex is unlocked and ready for reuse.
 */
void unlock_mutex(pthread_mutex_t *lock){
	pthread_mutex_unlock(lock);
}

//*********************************************************************
