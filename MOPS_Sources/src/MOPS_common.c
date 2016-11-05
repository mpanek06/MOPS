/**
 *	@file	MOPS_common.c
 *	@date	Jan 30, 2016
 *	@author	Michal Oleszczyk
 *	@brief	File containing functions responsible for
 *			communication between MOPS broker and local processes and for general borker logic.
 *
 *	Implementation for set of functions for broker-process communication
 *	and broker logic in general. Communication is based on queues mechanism.
 *	Every process is sending its process ID to queue named QUEUE_NAME (on Linux based target).
 */
#include "MQTT.h"
#include "MOPS_RTnet_Con.h"

#include "MOPS.h"

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

int TDMA_Dev = 0;
uint8_t MOPS_State = SEND_REQUEST;
uint16_t input_index = 0;			/**< Index of written bytes to #input_buffer. */
uint16_t output_index = 0;			/**< Index of written bytes to #output_buffer. */
uint16_t waiting_output_index = 0;	/**< Index of written bytes to #waiting_output_buffer. */
uint16_t waiting_input_index = 0;	/*< Index of written bytes to #waiting_input_buffer. */

// ***************   Funtions for local processes   ***************//

/**
 * @brief Advertising specified topic (user interface function). Returns handler for publishing.
 *
 * @param[in] Topic Topic name (as a string).
 */

PublishHandler advertiseMOPS(char *Topic){
	PublishHandler publishHdlr;
	publishHdlr.TopicName = Topic;
	publishHdlr.publish = publishMOPShdlr;
	return publishHdlr;
}

void publishMOPShdlr(char* Message, PublishHandler *self){
	publishMOPS(self->TopicName, Message, sizeof(Message));
}

/**
 * @brief Publishing specified message under specified topic (user interface function).
 *
 * @param[in] Topic Message topic name (as a string).
 * @param[in] Message Message payload.
 * @param[in] MessageLen Length of message in bytes.
 */
void publishMOPS(char *Topic, char *Message, int MessageLen) {
	char buffer[MAX_QUEUE_MESSAGE_SIZE+1];
	memset(buffer, 0, MAX_QUEUE_MESSAGE_SIZE+1);
	uint16_t packetID, written;
	written = BuildClientPublishMessage((uint8_t*) buffer, sizeof(buffer),
			(uint8_t*) Topic, (uint8_t*) Message, (uint16_t) MessageLen,  0, 0, &packetID);
	if (sendToMOPS(buffer, written) == -1) {
		perror("send");
	}
}

/**
 * @brief Sends to broker information with subscription of specified list of topics (user interface function).
 *
 * @param[in] TopicName Topics name to subscribe (string).
 * @param[in] QosList List of required Quality of Service (for now only 0 available).
 * @param[in] NoOfTopics Length of topics list.
 */
void subscribeMOPS(char **TopicList, uint8_t *QosList, uint8_t NoOfTopics) {
	char buffer[MAX_QUEUE_MESSAGE_SIZE+1];
	memset(buffer, 0, MAX_QUEUE_MESSAGE_SIZE+1);
	uint16_t packetID, written;
	written = BuildSubscribeMessage((uint8_t*) buffer, sizeof(buffer),
			(uint8_t**) TopicList, QosList, NoOfTopics, &packetID);

	if (sendToMOPS(buffer, written) == -1) {
		perror("send");
	}
}

/**
 * @brief Sends to broker information with subscription of one specified topic (user interface function).
 *
 * @param[in] TopicName Topics name to subscribe (string).
 * @param[in] Qos Required Quality of Service (for now only 0 available).
 */
void subscribeOnceMOPS(char *TopicName, uint8_t Qos) {
	char buffer[MAX_QUEUE_MESSAGE_SIZE+1];
	memset(buffer, 0, MAX_QUEUE_MESSAGE_SIZE+1);
	uint16_t packetID, written;

	uint8_t QosList[1];
	QosList[0] = Qos;

	char *TopicList[1];
	TopicList[0] = TopicName;

	written = BuildSubscribeMessage((uint8_t*) buffer, sizeof(buffer),
			(uint8_t**) TopicList, QosList, 1, &packetID);

	if (sendToMOPS(buffer, written) == -1) {
		perror("send");
	}
}

/**
 * @brief Receive data from MOPS broker (user interface function).
 *
 * @param[out] buf Container for data received from broker.
 * @param[in] length Define number of bytes which can be stored in buffer.
 * @return Number of bytes actually written.
 */
int readMOPS(char *buf, uint8_t length) {
	char temp[MAX_QUEUE_MESSAGE_SIZE+1];
	int t;
	memset(temp, 0, MAX_QUEUE_MESSAGE_SIZE+1);
	memset(buf, 0, length);

	if ((t = recvFromMOPS(temp, MAX_QUEUE_MESSAGE_SIZE)) > 0) {
		return InterpretFrame(buf, temp, t);
	} else {
		if (t < 0)
			perror("recv");
		else
			printf("Server closed connection\n");
	}
	return t;
}

/**
 * @brief Function interprets received from MOPS broker frame and
 * extracts pure message from that frame.
 *
 * @param[out] messageBuf Container for extracted message.
 * @param[in] frameBuf Raw frame received from broker.
 * @param[in] frameLen Length of raw frame.
 */
int InterpretFrame(char *messageBuf, char *frameBuf, uint8_t frameLen) {
	FixedHeader FHeader;
	uint8_t Qos, topicLen, messsageLen;
	uint16_t headLen = 0, index = 3;

	headLen = sizeof(FHeader);
	memcpy(&FHeader, frameBuf, headLen);
	Qos = (FHeader.Flags & 6) >> 1;

	topicLen = MSBandLSBTou16(frameBuf[index], frameBuf[index + 1]);
	index += (2 + topicLen);
	if (Qos > 0)
		index += 2;
	messsageLen = MSBandLSBTou16(frameBuf[index], frameBuf[index + 1]);
	index += 2;
	if ((index + messsageLen) <= frameLen) {
		memcpy(messageBuf, frameBuf + index, messsageLen);
		return messsageLen;
	}
	return 0;
}
// ***************   Funtions for local processes   ***************//

// ***************   Funtions for MOPS broker   ***************//
/**
 * @brief Function which starts MOPS broker functionalities (user interface function).
 *
 * If user want to use MOPS as his publish/subscribe protocol, this function
 * should be started firstly! This function is blocking itself in infinite loop,
 * so function has to be opened in other thread (on FreeRTOS) or in separated process
 * (on Linux devices).
 *
 * @return 0 - in case of end of main thread. This situation should never happened.
 */
int StartMOPSBroker(void) {
	lockMemoryInit();
	
	mutex_init(&input_lock);
	mutex_init(&output_lock);
	mutex_init(&waiting_output_lock);
	mutex_init(&waiting_input_lock);

	InitTopicList(list);
	MOPS_QueueInit(mops_queue);
	SubListInit(sub_list);
	connectToRTnet();
	startNewThread((void*) &threadSendToRTnet, NULL);
	startNewThread((void*) &threadRecvFromRTnet, NULL);
	InitProcesConnection();
	return 0;
}

/**
 * @brief Function which starts MOPS broker functionalities (user interface function) for Orocos need.
 *
 * If user want to use MOPS as his publish/subscribe protocol, this function
 * should be started firstly! This function is blocking itself in infinite loop,
 * so function has to be opened in other thread (on FreeRTOS) or in separated process
 * (on Linux devices). This function is prepared for Orocos purposes. All main task are started in separated threads so Orocos main thread is not blocked.
 *
 * @return 0 - in case of end of main thread.
 */
int StartMOPSBrokerNonBlocking(void) {
	lockMemoryInit();

	mutex_init(&input_lock);
	mutex_init(&output_lock);
	mutex_init(&waiting_output_lock);
	mutex_init(&waiting_input_lock);

	InitTopicList(list);
	MOPS_QueueInit(mops_queue);
	SubListInit(sub_list);
	connectToRTnet();
	startNewThread((void*) &threadSendToRTnet, NULL);
	startNewThread((void*) &threadRecvFromRTnet, NULL);
	startNewThread((void*) &InitProcesConnection, NULL);

	return 0;
}

/**
 * @brief Initialization of MOPS_Queue struct.
 *
 * List of MOPS_Queue structures is initialized when both queues fields
 * of every MOPS_Queue are set to 0.
 *
 * @param[in,out] queue MOPS_Queue list which should be initialized.
 * @post queue list is cleaned.
 */
void MOPS_QueueInit(MOPS_Queue *queue) {
	int i = 0;
	for (i = 0; i < MAX_PROCES_CONNECTION; i++) {
		queue[i].MOPSToProces_fd = 0;
		queue[i].ProcesToMOPS_fd = 0;
	}
}

/**
 * @brief Initialization of subscribers list.
 *
 * List containing all subscribers is initialized when every
 * filed with client id is set to -1 and topic connected to
 * that client id is erased.
 *
 * @param[in,out] sublist List containing struct SubscriberList which will be initialized.
 * @post Whole list is cleaned.
 */
void SubListInit(SubscriberList *sublist) {
	int i;
	for (i = 0; i < MAX_NUMBER_OF_SUBSCRIPTIONS; i++) {
		sublist[i].ClientID = -1;
		memset(sublist[i].Topic, 0, MAX_TOPIC_LENGTH + 1);
	}
}

/**
 * @brief Deleting a particular client ID from subscribers list.
 *
 * ClientID is an index from a global list #mops_queue represented particular process
 * connected to MOPS broker.
 *
 * @param ClientID ID of process connected to MOPS broker.
 * @param sublist Subscriber list from which client should be erased.
 * @post Process with ID ClientID do not subscribe any topic anymore.
 */
void DeleteProcessFromSubList(int ClientID, SubscriberList *sublist) {
	int i;
	for (i = 0; i < MAX_NUMBER_OF_SUBSCRIPTIONS; i++)
		if (i == ClientID) {
			sublist[i].ClientID = -1;
			memset(sublist[i].Topic, 0, MAX_TOPIC_LENGTH + 1);
		}
}

/**
 * @brief Function representing receiving from RTnet functionality.
 *
 * Receiving frames is closed in infinite loop so this particular
 * function should be started in separated thread.
 *
 * @pre Here are made changes of #input_buffer so also mutex #input_lock
 * is locked. That means #input_lock has to be unlock to start receiving.
 */
void threadRecvFromRTnet() {
	uint8_t temp[UDP_MAX_SIZE];
	uint16_t index_temp = 0;
	for (;;) {
		index_temp = receiveFromRTnet(temp, UDP_MAX_SIZE);
		lock_mutex(&input_lock);
		memcpy(input_buffer, temp, index_temp);
		input_index = index_temp;
		AnalyzeIncomingUDP(input_buffer, input_index);
		memset(input_buffer, 0, UDP_MAX_SIZE);
		input_index = 0;
		unlock_mutex(&input_lock);
	}
}

/**
 * @brief Function for sending frames to RTnet.
 *
 * Sending frames is closed in infinite loop so this particular
 * function should be started in separated thread.
 *
 */
void threadSendToRTnet() {
	uint8_t are_local_topics = 0;
	uint8_t TDMASyncOK = 0;

	if (0 == targetDependentInit()){
 		return;
	}

	for (;;) {
	    TDMASyncOK = waitOnTDMASync();

		if (!TDMASyncOK){
			printf("waitOnTDMASync error\n\r");
			continue;
		}
			switch (MOPS_State) {
			case SEND_NOTHING:
				//check if there are local topic to announce
				are_local_topics = ApplyIDtoNewTopics();
				MoveWaitingToFinal();
				if (are_local_topics)
					SendLocalTopics(list);
				else
					SendEmptyMessage();
				break;
			case SEND_REQUEST:
				SendTopicRequestMessage();
				break;
			case SEND_TOPIC_LIST:
				ApplyIDtoNewTopics();
				MoveWaitingToFinal();
				SendTopicList(list);
				break;
			}

			lock_mutex(&output_lock);
			if ((output_index > sizeof(MOPSHeader))	|| (output_buffer[0] == TOPIC_REQUEST)) {
				/* loop-back mechanism */
				lock_mutex(&input_lock);
				memcpy(input_buffer, output_buffer, output_index);
				input_index = output_index;
				AnalyzeIncomingUDP(input_buffer, input_index);
				memset(input_buffer, 0, UDP_MAX_SIZE);
				input_index = 0;
				unlock_mutex(&input_lock);
				/* loop-back mechanism end */
				sendToRTnet(output_buffer, output_index);
			}
			MOPS_State = SEND_NOTHING;
			memset(output_buffer, 0, UDP_MAX_SIZE);
			output_index = 0;
			unlock_mutex(&output_lock);
	}
}


/**
 * @brief Preparing "Nothing" MOPS protocol header and putting it
 * on the very beginning of #output_buffer.
 *
 * This kind of header has usually 3 bytes of length.
 *
 * @return Number of bytes added to a #output_buffer.
 * @pre #output_buffer has to has some empty space to write "Nothing" header.
 * Otherwise last frame in buffer will be malformed.
 */
uint16_t SendEmptyMessage() {
	uint8_t tempLen = 0;
	uint16_t writtenBytes = 0;
	tempLen += sizeof(MOPSHeader);
	if (tempLen > (UDP_MAX_SIZE - output_index))
		printf("Not enough space to send Empty Header\n");

	lock_mutex(&output_lock);
	memmove(output_buffer + tempLen, output_buffer, output_index); //Move all existing data
	writtenBytes = buildEmptyMessage(output_buffer,
			UDP_MAX_SIZE - output_index);
	output_index += writtenBytes;
	unlock_mutex(&output_lock);
	return writtenBytes;
}

/**
 * @brief Preparing "Topic Request" MOPS protocol header and putting it
 * on the very beginning of #output_buffer.
 *
 * This kind of header has usually 3 bytes of length.
 *
 * @return Number of bytes added to a #output_buffer.
 * @pre #output_buffer has to has some empty space to write "Topic Request" header.
 * Otherwise last frame in buffer will be malformed.
 */
uint16_t SendTopicRequestMessage() {
	uint8_t tempLen = 0;
	uint16_t writtenBytes = 0;
	tempLen += sizeof(MOPSHeader);
	if (tempLen > (UDP_MAX_SIZE - output_index))
		printf("Not enough space to send Topic Request\n");

	lock_mutex(&output_lock);
	memmove(output_buffer + tempLen, output_buffer, output_index); //Move all existing data
	writtenBytes = buildTopicRequestMessage(output_buffer,
			UDP_MAX_SIZE - output_index);
	output_index += writtenBytes;
	unlock_mutex(&output_lock);
	return writtenBytes;
}

/**
 * @brief Preparing "New Topic" MOPS protocol header and putting it
 * on the very beginning of #output_buffer.
 *
 * This kind of header containing all available for that broker topics (global and local)
 * with their ID. Headers length depends on number of all topics registered in broker.
 *
 * @param[in] list List containing structures linking topics name with its ID.
 * @return Number of bytes added to a #output_buffer.
 * @pre #output_buffer has to has some empty space to write "Topic Request" header.
 * Otherwise last frame in buffer will be malformed.
 */
uint16_t SendTopicList(TopicID list[]) {
	int i = 0, counter = 0, tempLen;
	uint8_t *tempTopicList[MAX_NUMBER_OF_TOPIC];
	uint16_t tempTopicIDs[MAX_NUMBER_OF_TOPIC];
	uint16_t writtenBytes;

	for (i = 0; i < MAX_NUMBER_OF_TOPIC; i++) {
		if (list[i].ID != 0) { //ID == 0 means that topics is a candidate.
			tempTopicList[counter] = (uint8_t*) (&list[i].Topic);
			tempTopicIDs[counter] = list[i].ID;
			if (list[i].LocalTopic == 1)
				list[i].LocalTopic = 0;
			counter++;
		}
	}
	tempLen = sizeof(MOPSHeader);
	for (i = 0; i < counter; i++)
		tempLen += 2 + 2 + strlen((char*) tempTopicList[i]); //2 for ID msb, ID lsb, 2 for length msb, length lsb.
	if (tempLen > (UDP_MAX_SIZE - output_index))
		printf("Not enough space to send all Topics from list\n");

	lock_mutex(&output_lock);
	memmove(output_buffer + tempLen, output_buffer, output_index); //Move all existing data
	writtenBytes = buildNewTopicMessage(output_buffer,
			UDP_MAX_SIZE - output_index, tempTopicList, tempTopicIDs, counter);
	output_index += writtenBytes;
	unlock_mutex(&output_lock);
	return writtenBytes;
}


/**
 * @brief Preparing "New Topic" MOPS protocol header and putting it
 * on the very beginning of #output_buffer.
 *
 * This kind of header containing all available for that broker (only local)
 * with their ID. After that local topics became global topics.
 * Headers length depends on number of all local topics registered in broker.
 *
 * @param[in] list List containing structures linking topics name with its ID.
 * @return Number of bytes added to a #output_buffer.
 * @pre #output_buffer has to has some empty space to write "Topic Request" header.
 * Otherwise last frame in buffer will be malformed.
 */
uint16_t SendLocalTopics(TopicID list[]) {
	int i = 0, counter = 0, tempLen;
	uint8_t *(tempTopicList[MAX_NUMBER_OF_TOPIC]);
	uint16_t tempTopicIDs[MAX_NUMBER_OF_TOPIC];
	uint16_t writtenBytes;

	for (i = 0; i < MAX_NUMBER_OF_TOPIC; i++) {
		if (list[i].ID != 0 && list[i].LocalTopic == 1) {
			tempTopicList[counter] = (uint8_t*) (&list[i].Topic);
			tempTopicIDs[counter] = list[i].ID;
			list[i].LocalTopic = 0;
			counter++;
		}
	}

	tempLen = sizeof(MOPSHeader);
	for (i = 0; i < counter; i++)
		tempLen += 2 + 2 + strlen((char*) tempTopicList[i]); //2 for ID msb, ID lsb, 2 for length msb, length lsb.
	if (tempLen > (UDP_MAX_SIZE - output_index))
		printf("Not enough space to send local Topics from list\n");

	lock_mutex(&output_lock);
	memmove(output_buffer + tempLen, output_buffer, output_index); //Move all existing data
	writtenBytes = buildNewTopicMessage(output_buffer,
			UDP_MAX_SIZE - output_index, tempTopicList, tempTopicIDs, counter);
	output_index += writtenBytes;
	unlock_mutex(&output_lock);
	return writtenBytes;
}
/**
 * @brief Main function fired when MOPS protocol header "New Topic" arrived.
 *
 * There are two forms in which topic can be added to a list in that case: \n
 * 1) Topic is heard first time. There is no that topic in our list so we
 * are adding it in the first empty place (.ID=0, empty .Topic field).\n
 * 2) Topic is already candidate (field .Topic is full filled, field
 * .ID stays 0), so we apply given ID to it.\n
 * If given ID i already in our list we ignore action.
 *
 * @param list[in] List of topic to which new one will be added.
 * @param topic[in] Topic name to add (in string).
 * @param topicLen[in] Length of topic name.
 * @param id[in] ID of topic.
 * @return 0 - if topic is added (or candidate is changed to real topic),\n
 * 2 - if give ID already exists so no action is needed,\n 1 - if there
 * was not space in list of topics.
 */
uint8_t AddTopicToList(TopicID list[], uint8_t *topic, uint16_t topicLen,
		uint16_t id) {
	int i = 0;
	uint16_t tempTopicLength;
	tempTopicLength =
			(topicLen < MAX_TOPIC_LENGTH) ? topicLen : MAX_TOPIC_LENGTH;

	for (i = 0; i < MAX_NUMBER_OF_TOPIC; i++) {
		//if candidate, apply ID
		if (strncmp((char*) list[i].Topic, (char*) topic, tempTopicLength) == 0
				&& list[i].Topic[0] != 0 && list[i].ID == 0) {
			list[i].ID = id;
			//printf("Dodalem ID kandydatowi: %s \n", list[i].Topic);
			return 0;
		}
		// if exists such topic (or at least ID) available, do not do anything
		if ((list[i].ID == id)
				|| (strncmp((char*) list[i].Topic, (char*) topic,
						tempTopicLength) == 0 && list[i].Topic[0] != 0)) {
			//printf("Nie dodam bo jest: %s \n", list[i].Topic);
			return 2;
		}
	}

	for (i = 0; i < MAX_NUMBER_OF_TOPIC; i++) {
		//else add new topic in the first empty place
		if (list[i].ID == 0 && strlen((char*) list[i].Topic) == 0) {
			memcpy(list[i].Topic, topic, tempTopicLength);
			//printf("Dodany: %s \n", list[i].Topic);
			list[i].ID = id;
			return 0;
		}
	}
	//there is no place in TopicList
	return 1;
}

/**
 * @brief Function called directly after opening RTnet time slot to apply IDs for all
 * 'candidate' topics (the ones with id=0).
 *
 * If publisher announced a topic that has not been published yet, it goes to topic list
 * as a 'candidate'. When broker has permission to send his frame to RTnet it gets right to
 * apply new IDs to all candidates. At that time such topics become 'local' (local
 * flag is set to 1). Now broker has to announce all local topics to other brokers in
 * RTnet sending "New Topic" header containing all local topics - after that topics
 * become 'global'.
 *
 * @return 1 - if there was at least one candidate changed into local topic,\n
 * 0 - if there was not any candidates.
 * @post Topics which were candidates will get IDs.
 */
uint8_t ApplyIDtoNewTopics() {
	int i;
	uint8_t localTopicFlag = 0;
	uint16_t max = 0;

	lock_mutex(&output_lock);
	for (i = 0; i < MAX_NUMBER_OF_TOPIC; i++) {
		if (list[i].ID > max)
			max = list[i].ID;
	}
	for (i = 0; i < MAX_NUMBER_OF_TOPIC; i++) {
		if (list[i].ID == 0 && strlen((char*) list[i].Topic) != 0) {
			list[i].ID = max + 1;
			list[i].LocalTopic = 1;
			max++;
			localTopicFlag = 1;
		}
	}
	unlock_mutex(&output_lock);
	return localTopicFlag;
}

/**
 * @brief Adding new topic as a 'candidate' (topic id=0).
 *
 * If publisher announce a topic which has not been published yet, it goes to topic list
 * as a 'candidate' and wait to applying real ID to it.
 *
 * @param[in] topic Topic name as a string.
 * @param[in] topicLen Length of a topic string.
 * @post One more topic added to topic list as a candidate.
 */
void AddTopicCandidate(uint8_t *topic, uint16_t topicLen) {
	int i;
	uint16_t tempTopicLength;

	tempTopicLength =
			(topicLen < MAX_TOPIC_LENGTH) ? topicLen : MAX_TOPIC_LENGTH;
	if (GetIDfromTopicName(topic, tempTopicLength) == -1)
		for (i = 0; i < MAX_NUMBER_OF_TOPIC; i++) {
			if (list[i].ID == 0 && strlen((char*) list[i].Topic) == 0) {
				memcpy(list[i].Topic, topic, tempTopicLength);
				return;
			}
		}
}

/**
 * @brief Extraction of topic ID from topic list by its name.
 * @param[in] topic Name (string) of topic which we want to get a ID.
 * @param[in] topicLen Length of topic name.
 * @return: ID of topic (uint16_t value) if topic exist already in topic list and is available\n
 *  0 - if topic is candidate in TopicList,\n
 *  -1 - if topic is not available, and not candidate.
 */
int GetIDfromTopicName(uint8_t *topic, uint16_t topicLen) {
	int i;
	uint16_t tempTopicLength;

	tempTopicLength =
			(topicLen < MAX_TOPIC_LENGTH) ? topicLen : MAX_TOPIC_LENGTH;
	for (i = 0; i < MAX_NUMBER_OF_TOPIC; i++) {
		if (strncmp((char*) list[i].Topic, (char*) topic, tempTopicLength) == 0
				&& list[i].Topic[0] != 0) //when  are the same
			return list[i].ID;
	}
	return -1;
}


/**
 * @brief Extraction of topic name from topic list by its ID.
 * @param[in] id ID of topic which we want to know a name.
 * @param[out] topic Buffer in which name will be written.
 * @return Length of a topic name.
 * @post Variable 'topic' is filled with topic name,
 * if there is not such topic ID in topic list
 * variable 'topic' is set to \0.
 */
uint16_t GetTopicNameFromID(uint16_t id, uint8_t *topic){
	int i;
	uint16_t len = 0;

	memset(topic, 0, MAX_TOPIC_LENGTH + 1);
	for (i = 0; i < MAX_NUMBER_OF_TOPIC; i++) {
		if (list[i].ID == id) { //when  are the same
			len = strlen((char*) list[i].Topic);
			memcpy(topic, &list[i].Topic, len);
			return len;
		}
	}
	return 0;
}

/**
 * @brief Initialization of topic list.
 *
 * Every element of topic list is erased: fields 'ID' and 'LocalTopic' are set to 0,
 * array 'Topic' is fulfilled with \0.
 *
 * @param[in,out] list List of topic which will be initialized.
 * @post All fields of variable list have been erased.
 */
void InitTopicList(TopicID list[]) {
	int i = 0;
	for (i = 0; i < MAX_NUMBER_OF_TOPIC; i++) {
		list[i].ID = 0;
		list[i].LocalTopic = 0;
		memset(&list[i].Topic, 0, MAX_TOPIC_LENGTH + 1);
	}
}

/**
 * @brief Function for printing topic list elements.
 *
 * This was very helpful while debugging.
 *
 * @param[in] list Topic list to print.
 */
void PrintfList(TopicID list[]) {
	int i;
	printf("Lista{\n");
	for (i = 0; i < MAX_NUMBER_OF_TOPIC; i++) {
		printf("    Topic: %s, ID: %d \n", list[i].Topic, list[i].ID);
	}
	printf("};\n");
}

/**
 * @brief Function for printing subscribers list elements.
 *
 * This was very helpful while debugging.
 *
 * @param[in] list Subscribers list to print.
 */
void PrintfSubList(SubscriberList sublist[]) {
	int i;
	printf("SubList{\n");
	for (i = 0; i < MAX_NUMBER_OF_SUBSCRIPTIONS; i++) {
		printf("    Topic: %s, SubscriberID: %d \n", sublist[i].Topic,
				sublist[i].ClientID);
	}
	printf("};\n");
}

/**
 * @brief First part of analyzing frames from RTnet.
 *
 * This is first stage responsible for recognition of MOPS protocol headers.
 * If header is "Nothing" then there is nothing to do. If came request for topic list,
 * broker has to change its state to send all available topics in the next possible slot.
 * When "New Topic" arrived broker has to analyze incoming topics.
 *
 * @param[in] Buffer Buffer containing received frame.
 * @param[in] written_bytes Length of written bytes into a buffer.
 */
void AnalyzeIncomingUDP(uint8_t *Buffer, int written_bytes) {
	MOPSHeader MHeader;
	uint16_t MOPSMessageLen;
	uint8_t HeadLen = sizeof(MHeader);

	memcpy(&MHeader, Buffer, HeadLen);
	MOPSMessageLen = MSBandLSBTou16(MHeader.RemainingLengthMSB,
			MHeader.RemainingLengthLSB) + HeadLen;

	switch (MHeader.MOPSMessageType) {
	case TOPIC_REQUEST:
		//lock_mutex(&output_lock);
		MOPS_State = SEND_TOPIC_LIST;
		//unlock_mutex(&output_lock);
		break;
	case NEW_TOPICS:
		//lock_mutex(&output_lock);
		UpdateTopicList(Buffer, written_bytes);
		//unlock_mutex(&output_lock);
		break;
	case NOTHING:
		//do not change state
		break;
	}
	//Move remaining data to buffer beginning
	lock_mutex(&waiting_input_lock);
	if ((UDP_MAX_SIZE - waiting_input_index)
			>= (written_bytes - MOPSMessageLen)) { //If we have enough space
		memmove(waiting_input_buffer + waiting_input_index,
				Buffer + MOPSMessageLen, written_bytes - MOPSMessageLen);
		waiting_input_index += (written_bytes - MOPSMessageLen);
	}
	unlock_mutex(&waiting_input_lock);
}

/**
 * @brief Analyzing frame in case of new topics.
 *
 * Function processed in case of "New Topic" MOPS header arrived.
 * Frame is given as a whole.
 *
 * @param[in] Buffer Frame containing "New Topic" header.
 * @param[in] BufferLen Length of whole frame.
 * @post This function is firing AddTopicList function
 * which can change number of all known topics.
 */
void UpdateTopicList(uint8_t *Buffer, int BufferLen) {
	uint16_t index = 0, messageLength = 0;
	uint16_t tempTopicLength = 0, tempTopicID = 0;

	messageLength = MSBandLSBTou16(Buffer[1], Buffer[2]) + 3;
	index += 3;
	for (; index < messageLength;) {
		tempTopicID = MSBandLSBTou16(Buffer[index], Buffer[index + 1]);
		tempTopicLength = MSBandLSBTou16(Buffer[index + 2], Buffer[index + 3]);
		index += 4;

		AddTopicToList(list, Buffer + index, tempTopicLength, tempTopicID);
		index += tempTopicLength;
	}
}

/**
 * @brief Closing connection between broker and particular process.
 * @param[in] file_de File descriptor which should be closed.
 * @post Process which used this file descriptor will be not
 * possible to communicate with broker with this session anymore.
 */
void CloseProcessConnection(int file_de) {
	int ClientID;
	printf("Proces ubijam!\n");
	ClientID = FindClientIDbyFileDesc(file_de);
	DeleteProcessFromQueueList(ClientID, mops_queue);
	DeleteProcessFromSubList(ClientID, sub_list);
}

/**
 * @brief High level function for triggered every time when
 * data should be send to local processes.
 *
 * In time window when there is no need to receive data from
 * local processes, broker is able to analyze all frames received from
 * RTnet, stored in #waiting_input_buffer waiting for sending them to
 * particular processes.
 *
 * @return 0 - always (should be corrected: TODO).
 */
int ServeSendingToProcesses() {
	uint8_t tempBuffer[UDP_MAX_SIZE], HeadLen;
	uint16_t FrameLen = 0, OldFrameLen = 0, written_bytes = 0;
	FixedHeader FHeader;
	memset(tempBuffer, 0, UDP_MAX_SIZE);

	lock_mutex(&waiting_input_lock);
	if (waiting_input_index > 0) {
		written_bytes = waiting_input_index;
		memcpy(tempBuffer, waiting_input_buffer, waiting_input_index);
		memset(waiting_input_buffer, 0, UDP_MAX_SIZE);
		waiting_input_index = 0;
	}
	unlock_mutex(&waiting_input_lock);

	if (written_bytes > 0) {
		HeadLen = sizeof(FHeader);
		memcpy(&FHeader, tempBuffer + FrameLen, HeadLen);
		FrameLen += MSBandLSBTou16(FHeader.RemainingLengthMSB,
				FHeader.RemainingLengthLSB) + HeadLen;

		while (FHeader.MessageType != 0 && FrameLen <= written_bytes)
		{
			PrepareFrameToSendToProcess(tempBuffer + OldFrameLen, FrameLen - OldFrameLen);
			memcpy(&FHeader, tempBuffer + FrameLen, HeadLen);
			OldFrameLen = FrameLen;
			FrameLen += MSBandLSBTou16(FHeader.RemainingLengthMSB,
					FHeader.RemainingLengthLSB) + HeadLen;
		}
	}
	return 0;
}

/**
 * @brief Preparing (and sending after that) MQTT frames
 * extracted from whole frame received from RTnet.
 *
 * Whole frame (without MOPS header) is divided to set of MQTT frames and
 * transmit to particular local process.
 *
 * @param[in] Buffer Buffer containing all raw MQTT frames.
 * @param[in] written_bytes Number of bytes which Buffer contains.
 * @post After preparing nice MQTT frames, this function is firing
 * sending them to particular process.
 */
void PrepareFrameToSendToProcess(uint8_t *Buffer, int written_bytes) {
	uint16_t topicID, topicLen, index = 0;
	uint8_t tempBuffer[MAX_QUEUE_MESSAGE_SIZE], HeaderLen;
	uint8_t tempTopic[MAX_TOPIC_LENGTH + 1], tempMSB = 0, tempLSB = 0;
	FixedHeader FHeader;
	int clientID[MAX_PROCES_CONNECTION], i;

	memset(tempBuffer, 0, MAX_QUEUE_MESSAGE_SIZE);
	memcpy(tempBuffer, Buffer, written_bytes);
	HeaderLen = sizeof(FHeader);

	topicID = MSBandLSBTou16(tempBuffer[HeaderLen], tempBuffer[HeaderLen + 1]);
	topicLen = GetTopicNameFromID(topicID, tempTopic);
	FindClientsIDbyTopic(clientID, tempTopic, topicLen);
	u16ToMSBandLSB(topicLen, &tempMSB, &tempLSB);

	tempBuffer[HeaderLen] = tempMSB;
	tempBuffer[HeaderLen + 1] = tempLSB;
	index = HeaderLen + 2;
	memmove(tempBuffer + index + topicLen, tempBuffer + index,
			written_bytes - index);
	memcpy(tempBuffer + index, tempTopic, topicLen);

	for (i = 0; i < MAX_PROCES_CONNECTION; i++)
		if (clientID[i] != -1)
			SendToProcess(tempBuffer, written_bytes + topicLen,
					(int) mops_queue[clientID[i]].MOPSToProces_fd);
}

/**
 * @brief Recognizing what processes subscribes given topic name.
 *
 * As MOPS can integrate many clients and many different topic, there is
 * possibility to many clients subscribe same topic. If we want to send them correct
 * MQTT frame we need to know list of subscribers.
 *
 * @param[out] clientsID List of clients ID which subscribe topic. It has to have
 * at least MAX_PROCES_CONNECTION length.
 * @param[in] topic Array of chars containing topic name (as a string).
 * @param[in] topicLen Length of topic name.
 * @post ClientsID variable has to be at least MAX_PROCES_CONNECTION, and it will be
 * erased firstly (all fields set to -1).
 */
void FindClientsIDbyTopic(int *clientsID, uint8_t *topic, uint16_t topicLen) {
	int i;
	int counter = 0;
	for (i = 0; i < MAX_PROCES_CONNECTION; i++)
		clientsID[i] = -1;

	for (i = 0; i < MAX_NUMBER_OF_SUBSCRIPTIONS; i++) {
		if (strncmp((char*) sub_list[i].Topic, (char*) topic, topicLen) == 0) {
			clientsID[counter] = sub_list[i].ClientID;
			counter++;
		}
	}
}

/**
 * @brief Taking particular client ID by file descriptor from 'connection list'.
 *
 * When we received some request (for example subscription request) we need to
 * connect file description from which we read those MQTT frame to one client ID.
 * That information is stored in 'communication list'.
 *
 * @param[in] file_de File descriptor for which we want to obtain client ID.
 * @return Client ID which is using given file descriptor.
 */
int FindClientIDbyFileDesc(int file_de) {
	int i = 0;
	for (i = 0; i < MAX_NUMBER_OF_SUBSCRIPTIONS; i++)
		if ((int)mops_queue[i].MOPSToProces_fd == file_de
				|| (int)mops_queue[i].ProcesToMOPS_fd == file_de)
			return i;
	return -1;
}

/**
 * @brief Mechanism for recognizing if MQTT frames in buffer are
 * a 'publish' or 'subscribe' requests.
 *
 * Function is taking all not read yet MQTT frames from process and decide
 * if they should be interpreted as 'subscribe' or 'publish' packets. Then
 * pass them separately to particular function responsible for serving them.
 *
 * @param[in] buffer Buffer containing read MQTT packets.
 * @param[in] bytes_wrote Number of bytes which was written to buffer.
 * @param[in] ClientID Client ID of process from which that MQTT frames were read.
 */
void AnalyzeProcessMessage(uint8_t *buffer, int bytes_wrote, int ClientID) {
	FixedHeader FHeader;
	uint8_t HeadLen = 0;
	uint16_t FrameLen = 0, OldFrameLen = 0;
	HeadLen = sizeof(FHeader);

	memcpy(&FHeader, buffer + FrameLen, HeadLen);
	FrameLen += MSBandLSBTou16(FHeader.RemainingLengthMSB,
			FHeader.RemainingLengthLSB) + HeadLen;
	while (FHeader.MessageType != 0 && FrameLen <= bytes_wrote) {
		switch (FHeader.MessageType) {
		case PUBLISH:
			ServePublishMessage(buffer + OldFrameLen, FrameLen - OldFrameLen);
			break;
		case SUBSCRIBE:
			ServeSubscribeMessage(buffer + OldFrameLen, FrameLen - OldFrameLen,
					ClientID);
			break;
		}
		memcpy(&FHeader, buffer + FrameLen, HeadLen);
		OldFrameLen = FrameLen;
		FrameLen += MSBandLSBTou16(FHeader.RemainingLengthMSB,
				FHeader.RemainingLengthLSB) + HeadLen;
	}
}

/**
 * @brief Serving one single pure MQTT 'publish' packet.
 *
 * Here single MQTT 'publish' frame is divided into parts and analyzed.
 *
 * @param[in] buffer Buffer containing one single MQTT frame.
 * @param[in] FrameLen Frame length (in bytes).
 * @post Frame is copied into #output_buffer
 * (if topic has been already known) or to #waiting_output_buffer
 * (if we do now know yet what topic ID is).
 */
void ServePublishMessage(uint8_t *buffer, int FrameLen) {
	uint8_t topicTemp[MAX_TOPIC_LENGTH + 1];
	uint16_t TopicLen, index = 0, tempTopicLength;
	int topicID;
	memset(topicTemp, 0, MAX_TOPIC_LENGTH + 1);

	index += 3;
	TopicLen = MSBandLSBTou16(buffer[index], buffer[index + 1]);
	index += 2;
	tempTopicLength =
			(TopicLen < MAX_TOPIC_LENGTH) ? TopicLen : MAX_TOPIC_LENGTH;
	memcpy(topicTemp, buffer + index, tempTopicLength);
	index += TopicLen;
	topicID = GetIDfromTopicName(topicTemp, TopicLen);
	switch (topicID) {
	case -1:
		AddTopicCandidate(topicTemp, TopicLen);
		AddPacketToWaitingTab(buffer, FrameLen);
		break;
	case 0:
		AddPacketToWaitingTab(buffer, FrameLen);
		break;
	default:
		AddPacketToFinalTab(buffer, FrameLen, topicID);
		break;
	}
}

/**
 * @brief Serving one single pure MQTT 'subscribe' packet.
 *
 * Here single MQTT 'subscribe' frame is divided into parts and analyzed.
 *
 * @param[in] buffer Buffer containing one single MQTT frame.
 * @param[in] FrameLen Length of frame (in bytes).
 * @param[in] ClientID ID of client which sent 'subscribe' request.
 * @post New client's ID is added to 'subscription list'
 * (if there is enough place)co 
 */
void ServeSubscribeMessage(uint8_t *buffer, int FrameLen, int ClientID) {
	uint16_t TopicLen, index = 0;

	index += 5;
	do {
		TopicLen = MSBandLSBTou16(buffer[index], buffer[index + 1]);
		index += 2;
		AddToSubscribersList(buffer + index, TopicLen, ClientID);
		index += (TopicLen + 1);
	} while (index < FrameLen);
}

/**
 * @brief Apply new subscription: connect given client ID with given topic name.
 * @param[in] topic Topic name (as a string).
 * @param[in] topicLen Topic name length (in bytes).
 * @param[in] ClientID ID of client that should be added to 'subscription list'.
 * @return -1 - if subscription already exists.\n
 * 0 - if there is no place in 'subscription list' to store new one.\n
 * >0 - if subscription has been added successfully.\n
 */
int AddToSubscribersList(uint8_t *topic, uint16_t topicLen, int ClientID) {
	int i = 0;
	uint16_t tempTopicLen;

	for (i = 0; i < MAX_NUMBER_OF_SUBSCRIPTIONS; i++) {
		if (sub_list[i].ClientID == ClientID
				&& strncmp((char*) sub_list[i].Topic, (char*) topic, topicLen)
						== 0 && sub_list[i].Topic[0] != 0) {
			return -1; //This subscription for that client already exists
		}
	}
	tempTopicLen = (topicLen < MAX_TOPIC_LENGTH) ? topicLen : MAX_TOPIC_LENGTH;
	for (i = 0; i < MAX_NUMBER_OF_SUBSCRIPTIONS; i++) {
		if (sub_list[i].ClientID == -1) {
			memcpy(sub_list[i].Topic, topic, tempTopicLen);
			sub_list[i].ClientID = ClientID;
			return i; //Subscription has been added successfully
		}
	}
	return 0; //There is no place to store subscription!
}


/**
 * @brief Push buffer content to a #waiting_output_buffer.
 *
 * This is used when topic is only 'candidate' (does not have any ID
 * yet) so frame has to be stored in 'waiting tab' and waiting
 * for ID applying to that topic.
 *
 * @param[in] buffer Buffer containing frame to push it to #waiting_output_buffer.
 * @param[in] FrameLen Number of bytes that should be copied into waiting tab.
 * @post Frame is added to #waiting_output_buffer this is why for that operation
 * mutex #waiting_output_lock is blocked.
 */
void AddPacketToWaitingTab(uint8_t *buffer, int FrameLen) {
	lock_mutex(&waiting_output_lock);
	if (waiting_output_index <= (uint16_t) (UDP_MAX_SIZE * 9) / 10) {
		memcpy(waiting_output_buffer + waiting_output_index, buffer, FrameLen);
		waiting_output_index += FrameLen;
	}
	unlock_mutex(&waiting_output_lock);
}

/**
 * @brief Push buffer content to a #output_buffer.
 *
 * This is used when topic as its ID so frame has to be stored
 * in array ready to send to RTnet.
 *
 * @param[in] buffer Buffer containing frame to push it to #output_buffer.
 * @param[in] FrameLen Number of bytes that should be copied into final tab.
 * @post Frame is added to #output_buffer this is why for that operation
 * mutex #output_lock is blocked.
 */
void AddPacketToFinalTab(uint8_t *buffer, int FrameLen, uint16_t topicID) {
	uint8_t tempBuff[MAX_QUEUE_MESSAGE_SIZE];
	uint8_t MSBtemp, LSBtemp, headLen, index = 0;
	uint16_t TopicLen, MessageLen;
	memset(tempBuff, 0, MAX_QUEUE_MESSAGE_SIZE);

	headLen = sizeof(FixedHeader);
	u16ToMSBandLSB(topicID, &MSBtemp, &LSBtemp);
	memcpy(tempBuff, buffer, headLen);
	MessageLen = MSBandLSBTou16(buffer[1], buffer[2]);

	tempBuff[headLen] = MSBtemp;
	tempBuff[headLen + 1] = LSBtemp;
	index = headLen + 2;

	TopicLen = MSBandLSBTou16(buffer[headLen], buffer[headLen + 1]);
	MessageLen = MessageLen - TopicLen;
	u16ToMSBandLSB(MessageLen, &MSBtemp, &LSBtemp);
	tempBuff[1] = MSBtemp; //New message len MSB
	tempBuff[2] = LSBtemp; //New message len LSB

	memcpy(tempBuff + index, buffer + index + TopicLen,
			FrameLen - (index + TopicLen));

	lock_mutex(&output_lock);
	if (output_index <= (uint16_t) (UDP_MAX_SIZE * 9) / 10) {
		memcpy(output_buffer + output_index, tempBuff, FrameLen - TopicLen);
		output_index += (FrameLen - TopicLen);
	}
	unlock_mutex(&output_lock);
}

/**
 * @brief Reinterpreting #waiting_output_buffer content.
 *
 * Just before sending new data into RTnet, broker applies new IDs
 * to 'candidate' topics. That is why some of those topics messages
 * can wait in #waiting_output_buffer for sending them. This function
 * once again interprets them and moves into final buffer esuch frames
 * for which topics are already known.
 *
 * @post This function copies all content of #waiting_output_buffer into
 * temporary buffer. That is why #waiting_output_buffer is erased.
 */
void MoveWaitingToFinal() {
	uint8_t tempTab[UDP_MAX_SIZE];
	uint16_t tempIndex = 0;

	lock_mutex(&waiting_output_lock);
	memcpy(tempTab, waiting_output_buffer, waiting_output_index);
	memset(waiting_output_buffer, 0, UDP_MAX_SIZE);
	tempIndex = waiting_output_index;
	waiting_output_index = 0;
	unlock_mutex(&waiting_output_lock);

	AnalyzeProcessMessage(tempTab, tempIndex, -1);
}
// ***************   Funtions for MOPS broker   ***************//

// ***************   Tools functions   ******************************//
/**
 * @brief Conversion uint16_t into two uint8_t values.
 *
 * @param[in] u16bit Value which has to be converted into most significant byte
 * and least significant byte.
 * @param[out] MSB Most significant byte of given value.
 * @param[out] LSB Least significant byte of given value.
 */
void u16ToMSBandLSB(uint16_t u16bit, uint8_t *MSB, uint8_t *LSB) {
	uint16_t temp;
	*LSB = (uint8_t) u16bit;
	temp = u16bit >> 8;
	*MSB = (uint8_t) temp;
}

/**
 * @brief Conversion two uint8_t values into uint16_t.
 *
 * @param[in] MSB Most significant byte of return value.
 * @param[in] LSB Least significant byte of return value.
 * @return 16bit value which result of conversion MSB and LSB.
 */
uint16_t MSBandLSBTou16(uint8_t MSB, uint8_t LSB) {
	uint16_t temp;
	temp = MSB;
	temp = temp << 8;
	temp += LSB;
	return temp;
}
// ***************   Tools functions   ******************************//