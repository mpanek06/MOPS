/**
 *	@brief	Header file containing for communication between
 *	MOPS brokers in RTnet.
 *
 *	Headers of function and structures definitions for broker-broker
 *	communication. Communication is based on UDP transfer. Every
 *	broker is sending its UDP packet to broadcast address and to
 *	yourself on port 1883.
 *
 *	@file	MOPS_RTnet_Con.h
 *	@date	Jan 30, 2016
 *	@author	Michal Oleszczyk
 */

#ifndef MOPS_RTNET_CON_LINUX_H_
#define MOPS_RTNET_CON_LINUX_H_

pthread_t startNewThread(void *(*start_routine) (void *), void *arg);
uint8_t mutex_init(pthread_mutex_t *lock);
void lock_mutex(pthread_mutex_t *lock);
void unlock_mutex(pthread_mutex_t *lock);

#endif /* MOPS_RTNET_CON_LINUX_H_ */
