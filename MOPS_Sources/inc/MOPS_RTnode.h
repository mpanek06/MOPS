/**
 *	@file	MOPS_RTnode.h
 *	@date	Mar 10, 2016
 *	@author	Michal Oleszczyk
 *	@brief	File defines, structs, enums and function headers.
 *
 *	Headers set of functions for broker-process communication
 *	and broker logic in general. Here are also stored all defines
 *	responsible for main configuration of MOPS parameters.
 */

#ifndef MOPS_RTNODE_H_
#define MOPS_RTNODE_H_

#include "FreeRTOS.h"
#include "timers.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "rtnet.h"
#include "rtnet_inet.h"

QueueHandle_t GlobalProcesMopsQueue;

typedef struct MOPS_Queue {
	QueueHandle_t ProcesToMOPS_fd;
	QueueHandle_t MOPSToProces_fd;
} MOPS_Queue;

QueueHandle_t ServeNewProcessConnection();
 uint8_t *output_buffer;				 		 	/**< Buffer for sending data to RTnet. */

 SemaphoreHandle_t output_lock;			/**< mutex for blocking access to #output_buffer. */
 SemaphoreHandle_t input_lock;			/**< mutex for blocking access to #input_buffer. */
 SemaphoreHandle_t waiting_output_lock;	/**< mutex for blocking access to #waiting_output_buffer. */
 SemaphoreHandle_t waiting_input_lock;	/**< mutex for blocking access to #waiting_input_buffer. */

#endif /* MOPS_RTNODE_H_ */
