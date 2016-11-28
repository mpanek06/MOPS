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

#ifndef MOPS_RTNET_CON_RTNODE_H_
#define MOPS_RTNET_CON_RTNODE_H_

void startNewThread(void *(*start_routine) (void *), void *arg);
uint8_t mutex_init(SemaphoreHandle_t *lock);
void lock_mutex(SemaphoreHandle_t *lock);
void unlock_mutex(SemaphoreHandle_t *lock);

uint8_t semaphore_init(SemaphoreHandle_t *sem);
void semaphore_give(SemaphoreHandle_t *sem);
uint8_t semaphore_take(SemaphoreHandle_t *sem);
#endif /* MOPS_RTNET_CON_RTNODE_H_ */
