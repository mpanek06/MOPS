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

#include "task.h"

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

static xRTnetSockAddr_t rec_addr; /**< Struct containing socket address for receiving. */
static xRTnetSockAddr_t sd_addr_b;/**< Struct containing socket address for sending broadcast. */
xRTnetSocket_t get_sock; /**< Socket for receiving packet from RTnet. */
xRTnetSocket_t bcast_sock; /**< Socket for broadcasting packets to RTnet. */

/**
 * @brief Enable starting of some function in separated thread.
 * @param start_routine Pointer to a function which will be  started as a new thread.
 * @param arg Pointer to a struct containing arguments for function which will be started.
 * @return ID of new thread.
 *
 * @post New thread has been started.
 */
void startNewThread(void *(*start_routine) (void *), void *arg){
	TaskHandle_t xHandle = NULL;
	xTaskCreate( (*start_routine), NULL, 400, arg, 3, &xHandle );
}

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
    uint8_t  macBroadcast[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    uint32_t ip;
    xRTnetSockAddr_t  bindAddr;

	while(xRTnetWaitRedy(portMAX_DELAY) == pdFAIL){;}

	 /* network byte order ip */
    ip  = ulRTnetGetIpAddr();
    /* Add broadcast address */
    ip |= rtnet_htonl(RTNET_NETMASK_BROADCAST);
    xRTnetRouteAdd(macBroadcast, ip);

    if ((bcast_sock = xRTnetSocket(RTNET_AF_INET, RTNET_SOCK_DGRAM, RTNET_IPPROTO_UDP)) == NULL)
        vTaskSuspend(NULL);
    if ((get_sock = xRTnetSocket(RTNET_AF_INET, RTNET_SOCK_DGRAM, RTNET_IPPROTO_UDP)) == NULL)
        vTaskSuspend(NULL);

    xRTnetSetsockopt(bcast_sock, 0, RTNET_SO_TXTIMEOUT, (void  *) 1, 0);
    rec_addr.sin_port = rtnet_htons(MOPS_PORT);
    rec_addr.sin_addr =  rtnet_htonl(0x00000000UL);//inet_addr("10.0.0.0");


    sd_addr_b.sin_port = rtnet_htons(MOPS_PORT);
    sd_addr_b.sin_addr = ip;

    bindAddr.sin_port = rtnet_htons(2001);
    if(xRTnetBind(bcast_sock, &bindAddr, sizeof(&bindAddr)) != 0)
		vTaskSuspend(NULL);

	if (xRTnetBind(get_sock, &rec_addr, sizeof(rec_addr)) != 0)
		vTaskSuspend(NULL);
	rtprintf("Connection with RTnet established\r\n");
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
	uint32_t len = sizeof(sd_addr_b);

    if( (write = lRTnetSendto(bcast_sock, buf, buflen, 0, &sd_addr_b, len)) <= 0 )
        rtprintf("error: sendto = %d \r\n", write);
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
	uint32_t len = sizeof(rec_addr);
	uint8_t          *data;

	written = lRTnetRecvfrom(get_sock, &data, (size_t) buflen, RTNET_ZERO_COPY, &rec_addr,  &len);
	memcpy(buf, data, written);
	vRTnetReleaseUdpDataBuffer(data);
	//rtprintf("Odebrane, niby\r\n");
	return written;
}

/**
 * @brief Initiation o mutex.
 * @param lock Pointer to mutex we want to be initiated.
 * @return 1 if initiation failed, 0 if everything goes fine.
 *
 * @post Pointed mutex if ready to use.
 */
uint8_t mutex_init(SemaphoreHandle_t *lock){
	*lock = xSemaphoreCreateMutex();
	if( *lock == NULL )
		return 1;
    return 0;
}

/**
 * @brief Locking mutex.
 * @param lock Pointer to mutex we want to locked.
 *
 * @post Pointed mutex is locked.
 */
void lock_mutex(SemaphoreHandle_t *lock){
	while( xSemaphoreTake( *lock, ( TickType_t ) 30 ) != pdTRUE )
	{
		//rtprintf("Zablokowany...\r\n");
	}
}

/**
 * @brief Unlocking mutex.
 * @param lock Pointer to mutex we want to unlock.
 *
 * @post Pointed mutex is unlocked and ready for reuse.
 */
void unlock_mutex(SemaphoreHandle_t *lock){
	xSemaphoreGive( *lock );
}


/**
 * @brief Initialization of semaphore.
 * @param sem Pointer to semaphore that should be initialized.
 * @return -1 if Initialization failed, 0 if everything goes fine.
 *
 */
uint8_t semaphore_init(SemaphoreHandle_t *sem){
    if (*sem = xSemaphoreCreateBinary())
    {
        return -1;
    }
    return 0;
}

/**
 * @brief Initialization of semaphore.
 * @param sem Pointer to semaphore that should be initialized.
 * @return -1 if Initialization failed, 0 if everything goes fine.
 *
 */
void semaphore_give(SemaphoreHandle_t *sem){
    xSemaphoreGive(*sem);
}

/**
 * @brief Initialization of semaphore.
 * @param sem Pointer to semaphore that should be initialized.
 * @return -1 if Initialization failed, 0 if everything goes fine.
 *
 */
uint8_t semaphore_take(SemaphoreHandle_t *sem){
    while( xSemaphoreTake( *sem, ( TickType_t ) 30 ) != pdTRUE ){}
    return 0;
}