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


//***************** MOPS - MOPS communication protocol ********************
/**
 * @brief Function for creating MOPS protocol "Topic Request" header.
 * "Topic Request" header shape looks like:
 * <pre>
 * +-+-+-+-+-+-+-+-+
 * |0|0|0|0|0|0|0|1|  <- Topic Request type: 1
 * +-+-+-+-+-+-+-+-+
 * |0|0|0|0|0|0|0|0|  <- Remaining header length most significant byte: 0
 * +-+-+-+-+-+-+-+-+
 * |0|0|0|0|0|0|0|0|  <- Remaining header length least significant byte: 0
 * +-+-+-+-+-+-+-+-+
 * </pre>
 * @param Buffer Buffer where header will be stored.
 * @param BufferLen Maximal buffer length which can be overridden.
 * @return Length of buffer (in bytes) which has been overridden - number of written bytes.
 *
 * @post Only if BufferLen is bigger or equal to header length, Buffer contains written header.
 */
uint16_t buildTopicRequestMessage(uint8_t *Buffer, int BufferLen){
	MOPSHeader MHeader;
	uint8_t index = 0;

	MHeader.MOPSMessageType = TOPIC_REQUEST;
	MHeader.RemainingLengthLSB = 0;
	MHeader.RemainingLengthMSB = 0;
	if(BufferLen >= sizeof(MHeader)){
		memcpy(Buffer, &MHeader, sizeof(MHeader));
		index += sizeof(MHeader);
	}
	return index;
}

/**
 * @brief Function for creating MOPS protocol "New Topic" header.
 * "New Topic" header shape looks like:
 * <pre>
 * +-+-+-+-+-+-+-+-+
 * |0|0|0|0|0|0|1|0|  <- Topic Request type: 2
 * +-+-+-+-+-+-+-+-+
 * |X|X|X|X|X|X|X|X|  <- Remaining header length most significant byte.
 * +-+-+-+-+-+-+-+-+
 * |X|X|X|X|X|X|X|X|  <- Remaining header length least significant byte.
 * +-+-+-+-+-+-+-+-+
 * | Topic ID  MSB |  <- Topic identification number most significant byte.
 * +-+-+-+-+-+-+-+-+
 * | Topic ID  LSB |  <- Topic identification number least significant byte.
 * +-+-+-+-+-+-+-+-+
 * | Topic Len MSB |  <- Topic length most significant byte.
 * +-+-+-+-+-+-+-+-+
 * | Topic Len LSB |  <- Topic length least significant byte.
 * +-+-+-+-+-+-+-+-+
 * |X|X|X|X|X|X|X|X|  <- First character of topic string.
 * +-+-+-+-+-+-+-+-+
 *      ......
 * +-+-+-+-+-+-+-+-+
 * |X|X|X|X|X|X|X|X|  <- Last character of topic string..
 * +-+-+-+-+-+-+-+-+
 * </pre>
 * @param Buffer Buffer where header will be stored.
 * @param BufferLen Maximal buffer length which can be overridden.
 * @param Topics Pointer to array containing topics in string format.
 * @param IDs Pointer to array containing topics IDs.
 * @param TopicNo Number of topics in array that we should send as new topic in RTnet.
 * @return Length of buffer (in bytes) which has been overridden - number of written bytes.
 *
 * @post Only if BufferLen is bigger or equal to header length, Buffer contains written header.
 */
uint16_t buildNewTopicMessage(uint8_t *Buffer, int BufferLen, uint8_t **Topics, uint16_t *IDs, int TopicNo){
	MOPSHeader MHeader;
	uint8_t MSB_temp, LSB_temp;
	int index = 0, tempLen = 0, i;
	index = sizeof(MHeader);

	//**** Checking if we have enough space ****//
	tempLen += sizeof(MHeader);
	for (i=0; i<TopicNo; i++)
		tempLen += strlen((char*)(Topics[i])) + 2 + 2;
	if(BufferLen <= tempLen)
		return 0;
	tempLen = 0;
	//**** Checking if we have enough space ****//

	//**** Payload part *****//
	for (i=0; i<TopicNo; i++){
		u16ToMSBandLSB(IDs[i], &MSB_temp, &LSB_temp);
		Buffer[index] = MSB_temp;
		Buffer[index+1] = LSB_temp;
		tempLen = strlen((char*)(Topics[i]));
		u16ToMSBandLSB(tempLen, &MSB_temp, &LSB_temp);
		Buffer[index+2] = MSB_temp;
		Buffer[index+3] = LSB_temp;
		index += 4;
		memcpy(Buffer + index, Topics[i], tempLen);
		index += tempLen;
	}
	//**** Payload part *****//

	u16ToMSBandLSB(index-sizeof(MHeader), &MSB_temp, &LSB_temp);
	MHeader.MOPSMessageType = NEW_TOPICS;
	MHeader.RemainingLengthMSB = MSB_temp;
	MHeader.RemainingLengthLSB = LSB_temp;
	memcpy(Buffer, &MHeader, sizeof(MHeader));

	return (uint16_t)index;
}

/**
 * @brief Function for creating MOPS protocol "Nothing" header.
 * "Nothing" header shape looks like:
 * <pre>
 * +-+-+-+-+-+-+-+-+
 * |0|0|0|0|0|0|1|1|  <- Topic Request type: 3
 * +-+-+-+-+-+-+-+-+
 * |0|0|0|0|0|0|0|0|  <- Remaining header length most significant byte: 0
 * +-+-+-+-+-+-+-+-+
 * |0|0|0|0|0|0|0|0|  <- Remaining header length least significant byte: 0
 * +-+-+-+-+-+-+-+-+
 * </pre>
 * @param Buffer Buffer where header will be stored.
 * @param BufferLen Maximal buffer length which can be overridden.
 * @return Length of buffer (in bytes) which has been overridden - number of written bytes.
 *
 * @post Only if BufferLen is bigger or equal to header length, Buffer contains written header.
 */
uint16_t buildEmptyMessage(uint8_t *Buffer, int BufferLen){
	MOPSHeader MHeader;
	uint8_t index = 0;

	MHeader.MOPSMessageType = NOTHING;
	MHeader.RemainingLengthLSB = 0;
	MHeader.RemainingLengthMSB = 0;
	if(BufferLen >= sizeof(MHeader)){
		memcpy(Buffer, &MHeader, sizeof(MHeader));
		index += sizeof(MHeader);
	}
	return index;
}
//*********************************************************************
