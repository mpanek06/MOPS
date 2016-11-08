/**
 *	@file	MOPS_Linux.h
 *	@date	Mar 10, 2016
 *	@author	Michal Oleszczyk
 *	@brief	File defines, structs, enums and function headers.
 *
 *	Headers set of functions for broker-process communication
 *	and broker logic in general. Here are also stored all defines
 *	responsible for main configuration of MOPS parameters.
 */

#ifndef MOPS_LINUX_H_
#define MOPS_LINUX_H_

#include <mqueue.h>
#include <string.h>
/** Name of general queue (processes->broker). */
#define QUEUE_NAME "/MOPS_path1e11"

/**
 * @struct MOPS_Queue
 * @brief Structure for connecting two file descriptors
 * responsible for broker<->process communication.
 *
 * Each new local process which wants to connect to MOPS
 * broker, create to queues with format: \{proces_id}a,
 * \{proces_id}b. First one (a) is for broker->process, second
 * one (b) is process->broker. This structure is used to build
 * 'communication list'.
 * */
typedef struct MOPS_Queue {
	/** File descriptor for transmission process->broker*/
	mqd_t ProcesToMOPS_fd;
	/** File descriptor for transmission broker->process*/
	mqd_t MOPSToProces_fd;
} MOPS_Queue;

uint8_t output_buffer[UDP_MAX_SIZE]; 	/**< Buffer for sending data to RTnet. */

int ServeNewProcessConnection(fd_set *set, int listener_fd);

pthread_mutex_t output_lock; 			/**< mutex for blocking access to #output_buffer. */
pthread_mutex_t input_lock; 			/**< mutex for blocking access to #input_buffer. */
pthread_mutex_t waiting_output_lock;	/**< mutex for blocking access to #waiting_output_buffer. */
pthread_mutex_t	waiting_input_lock;		/**< mutex for blocking access to #waiting_input_buffer. */

extern int TDMA_Dev;
extern mqd_t mq_listener;
#endif /* MOPS_LINUX_H_ */
