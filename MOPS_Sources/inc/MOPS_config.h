/**
 *	@file	MOPS_config.h
 *	@date	Mar 10, 2016
 *	@author	Michal Oleszczyk
 *	@brief	File defines, structs, enums and function headers.
 *
 *	General settings of MOPS protocole.
 */

#ifndef MOPSCONFIG_H_
#define MOPSCONFIG_H_

//***************** General Settings *********************
/** Maximal number of connected local processes to broker. */
#define MAX_PROCES_CONNECTION 100
/** Maximal length of message broker<->process. */
#define MAX_QUEUE_MESSAGE_SIZE 100
/** Maximal amount of messages stored in queues broker<->process. */
#define MAX_QUEUE_MESSAGE_NUMBER 10
//***************** General Settings *********************

//***************MOPS - RTnet Settings********************
/** MOPS protocol port. */
#define MOPS_PORT 1525
/** Size of send/receive buffers. */
#define UDP_MAX_SIZE 512

/** Broadcast address. */
#define IPADDR     "10.255.255.255"
/** Maximal length of MOPS topic name (max is 2^16-1).*/
#define MAX_TOPIC_LENGTH             30
/** Maximal length of MOPS message (max is 2^16-1).*/
#define MAX_MESSAGE_LENGTH			 100
/** Maximal number of different topic names (max is 2^16-1).*/
#define MAX_NUMBER_OF_TOPIC          100
/** Maximal number of different subscriptions (max is 2^16-1).*/
#define MAX_NUMBER_OF_SUBSCRIPTIONS  100
//***************MOPS - RTnet Settings********************

#endif /* MOPSCONFIG_H_ */
