/*
 * local_broker.c
 *
 *  Created on: Jan 20, 2016
 *      Author: rudy
 */
#include <sys/select.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include "MOPS.h"
#include "MQTT.h"
#include "MOPS_RTnet_Con.h"


static uint8_t MOPS_State = SEND_REQUEST;
uint8_t input_buffer[UDP_MAX_SIZE], output_buffer[UDP_MAX_SIZE], waiting_output_buffer[UDP_MAX_SIZE];
uint16_t writtenBytes = 0, output_index = 0, waiting_output_index = 0;
TopicID list[MAX_NUMBER_OF_TOPIC];
SubscriberList SubList[MAX_NUMBER_OF_SUBSCRIPTIONS];
MOPS_Queue mops_queue[MAX_PROCES_CONNECTION];

#if TARGET_DEVICE == Linux
pthread_mutex_t output_lock, input_lock, waiting_output_lock;
#endif
#if TARGET_DEVICE == RTnode
SemaphoreHandle_t output_lock, input_lock, waiting_output_lock;
#endif


int main(void)
{
	int RTsocket = 0;
	mutex_init(&input_lock);
	mutex_init(&output_lock);

	InitTopicList(list);
	MOPS_QueueInit(mops_queue);

	RTsocket = connectToRTnet();

    startNewThread(&threadSendToRTnet, (void *)RTsocket);
    startNewThread(&threadRecvFromRTnet, (void *)RTsocket);

	InitProcesConnection();
}

void MOPS_QueueInit(MOPS_Queue *queue){
	int i = 0;
	for(i=0; i<MAX_PROCES_CONNECTION; i++)
	{
		queue->MOPSToProces_fd = 0;
		queue->ProcesToMOPS_fd = 0;
	}
}

void threadRecvFromRTnet(int RTsocket){
    for(;;){
    	lock_mutex(&input_lock);
		receiveFromRTnet(RTsocket, input_buffer, UDP_MAX_SIZE);
		AnalizeIncomingUDP(input_buffer, UDP_MAX_SIZE);
		unlock_mutex(&input_lock);
    }
}

void threadSendToRTnet(int RTsocket){
	for(;;){
		sleep(2);  // slot czasowy
		lock_mutex(&output_lock);

		switch(MOPS_State){
		case SEND_NOTHING:
			//check if there are local topic to announce
			//if yes, then add them to head of message and update TopicList - reset LocalTopic flag
			//else, send 'nothing'
			if( ApplyIDtoNewTopics() )
				output_index += SendLocalTopics(output_buffer, UDP_MAX_SIZE, list);
			else
				output_index += buildEmptyMessage(output_buffer, UDP_MAX_SIZE);
			break;
		case SEND_REQUEST:
			output_index += buildTopicRequestMessage(output_buffer, UDP_MAX_SIZE);
			break;
		case SEND_TOPIC_LIST:
			ApplyIDtoNewTopics();
			output_index += SendTopicList(output_buffer, UDP_MAX_SIZE, list);
			break;
		}

		if (output_index > 0){
			sendToRTnet(RTsocket, output_buffer, output_index);
			MOPS_State = SEND_NOTHING;
			memset(output_buffer, 0, UDP_MAX_SIZE);
			output_index = 0;
		}
		unlock_mutex(&output_lock);
	}
}

/*
 * Sending all available (not candidate) topics to RTnet,
 * after that local topics become global.
 */
uint16_t SendTopicList(uint8_t *Buffer, int BufferLen, TopicID list[]){
	int i = 0, counter = 0;
	uint8_t *tempTopicList[MAX_NUMBER_OF_TOPIC];
	uint16_t tempTopicIDs[MAX_NUMBER_OF_TOPIC];
	uint16_t writtenBytes;

	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		if (list[i].ID != 0){
			tempTopicList[counter] = &list[i].Topic;
			tempTopicIDs[counter] = list[i].ID;
			if(list[i].LocalTopic == 1)
				list[i].LocalTopic = 0;
			counter++;
		}
	}
	writtenBytes = buildNewTopicMessage(Buffer, BufferLen, tempTopicList, tempTopicIDs, counter);
	return writtenBytes;
}

/*
 * Sending only local topics to RTnet,
 * after that local topics become global.
 */
uint16_t SendLocalTopics(uint8_t *Buffer, int BufferLen, TopicID list[]){
	int i = 0, counter = 0;
	uint8_t *tempTopicList[MAX_NUMBER_OF_TOPIC];
	uint16_t tempTopicIDs[MAX_NUMBER_OF_TOPIC];
	uint16_t writtenBytes;

	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		if (list[i].ID != 0 && list[i].LocalTopic==1){
			tempTopicList[counter] = &list[i].Topic;
			tempTopicIDs[counter] = list[i].ID;
			list[i].LocalTopic = 0;
			counter++;
		}
	}
	writtenBytes = buildNewTopicMessage(Buffer, BufferLen, tempTopicList, tempTopicIDs, counter);
	return writtenBytes;
}


uint8_t AddTopicToList(TopicID list[], uint8_t *topic, uint16_t topicLen, uint16_t id){
	int i = 0;

	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		//if candidate, apply ID
		if( strncmp(list[i].Topic, topic, topicLen)==0 && list[i].Topic[0]!=0 && list[i].ID==0 ){
			list[i].ID = id;
			//printf("Dodalem ID kandydatowi: %s \n", list[i].Topic);
			return 0;
		}
		// if exists such topic (or at least ID) available, do not do anything
		if ( (list[i].ID == id) || (strncmp(list[i].Topic, topic, topicLen)==0 && list[i].Topic[0]!=0) ){
			//printf("Nie dodam bo jest: %s \n", list[i].Topic);
			return 2;
		}
	}

	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		//else add new topic in the first empty place
		if ( list[i].ID==0 && strlen(list[i].Topic)==0 ){
			memcpy(list[i].Topic, topic, topicLen);
			//printf("Dodany: %s \n", list[i].Topic);
			list[i].ID = id;
			return 0;
		}
	}
	//there is no place in TopicList
	return 1;
}


uint8_t ApplyIDtoNewTopics(){
	int i;
	uint8_t localTopicFlag = 0;
	uint16_t max = 0;

	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		if(list[i].ID > max)
			max = list[i].ID;
	}
	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		if ( list[i].ID==0 && strlen(list[i].Topic)!=0 ){
			list[i].ID = max+1;
			list[i].LocalTopic = 1;
			max++;
			localTopicFlag = 1;
		}
	}
	return localTopicFlag;
}

void AddTopicCandidate(uint8_t *topic, uint16_t topicLen){
	int i;
	if(GetIDfromTopicName(topic, topicLen) == -1)
		for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
			if ( list[i].ID==0 && strlen(list[i].Topic)==0 ){
				memcpy(list[i].Topic, topic, topicLen);
				return;
			}
		}
}

/*
 * return:
 *  ID (uint16_t value) if topic exist already in TopicList and is available
 *  0					if topic is candidate in TopicList
 *  -1					if topic is not available, and not candidate
 */
int GetIDfromTopicName(uint8_t *topic, uint16_t topicLen){
	int i;
	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		if (strncmp(list[i].Topic, topic, topicLen)==0 && list[i].Topic[0]!=0)  //when  are the same
				return list[i].ID;
	}
	return -1;
}

/*
 * POST: variable 'topic' is set as Topic with id 'id',
 * if there is not a topic in TopicList with that id
 * variable 'topic' is set to \0.
 */
void GetTopicNameFromID(uint16_t id, uint8_t *topic, uint16_t topicLen){
	int i;
	memset(topic, 0, topicLen);
	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		if (list[i].ID == id)  //when  are the same
			memcpy(topic, &list[i].Topic, topicLen);
	}
}

void InitTopicList(TopicID list[]){
	int i = 0;
	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		list[i].ID = 0;
		list[i].LocalTopic = 0;
		memset(&list[i].Topic, 0, MAX_TOPIC_LENGTH+1);
	}
}

void PrintfList(TopicID list[]){
	int i;
	printf("Lista{\n");
	for(i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		printf("    Topic: %s, ID: %d \n", list[i].Topic, list[i].ID);
	}
	printf("};\n");
}

void AddClientIDToPacket(uint8_t *buf, uint8_t ClientID, int *WrittenBytes, int nbytes){
	memmove(buf + sizeof(ClientID), buf, nbytes);
	memcpy(buf, &ClientID, sizeof(ClientID));
	(*WrittenBytes) += sizeof(ClientID);
}

void AnalizeIncomingUDP(uint8_t *Buffer, uint8_t BufferLen){
	MOPSHeader MHeader;
	memcpy(&MHeader, Buffer, sizeof(MHeader));

	switch(MHeader.MOPSMessageType){
	case TOPIC_REQUEST:
		lock_mutex(&output_lock);
		MOPS_State = SEND_TOPIC_LIST;
		unlock_mutex(&output_lock);
		break;
	case NEW_TOPICS:
		lock_mutex(&output_lock);
		UpdateTopicList(Buffer, BufferLen);
		unlock_mutex(&output_lock);
		break;
	case NOTHING:
		//do not change state
		break;
	}
}

void UpdateTopicList(uint8_t *Buffer, uint8_t BufferLen){
	uint16_t index = 0, messageLength = 0;
	uint16_t tempTopicLength = 0, tempTopicID = 0;
	uint8_t tempTopic[MAX_TOPIC_LENGTH], err;

	messageLength = MSBandLSBTou16(Buffer[1], Buffer[2]) + 3;
	index += 3;
	for(; index<messageLength; ){
		tempTopicID = MSBandLSBTou16(Buffer[index], Buffer[index+1]);
		tempTopicLength = MSBandLSBTou16(Buffer[index+2], Buffer[index+3]);
		index += 4;
		memcpy(tempTopic, Buffer+index, tempTopicLength);
		err = AddTopicToList(list, tempTopic, tempTopicLength, tempTopicID);
		index += tempTopicLength;
		if(err == 1)
			printf("Brak miejsca na liscie! \n");
		if(err == 0)
			printf("Dodalem, id: %d \n", tempTopicID);
		if(err == 2)
			printf("Topic, id: %d, juz istnieje. \n", tempTopicID);
	}
}

int AddToMOPSQueue(int MOPS_Proces_fd, int Proces_MOPS_fd){
	int i = 0;
	for(i=0; i<MAX_PROCES_CONNECTION; i++)
		if(mops_queue[i].MOPSToProces_fd==0 && mops_queue[i].ProcesToMOPS_fd==0){
			mops_queue[i].MOPSToProces_fd = MOPS_Proces_fd;
			mops_queue[i].ProcesToMOPS_fd = Proces_MOPS_fd;
			return i;
		}
	return -1;
}

#if TARGET_DEVICE == Linux
void InitProcesConnection(){
    mqd_t mq_listener, new_mq_Proces_MOPS;
    struct mq_attr attr;
    struct timeval tv;
    int bytes_read, fdmax, rv, i;
    fd_set master, read_fd;  //master fd list, temp fd list for select()
	FD_ZERO(&master);
	FD_ZERO(&read_fd);

    /* initialize the queue attributes */
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_QUEUE_SIZE;
    attr.mq_curmsgs = 0;

	mq_listener = mq_open(QUEUE_NAME, O_CREAT | O_RDONLY, 0644, &attr);
    if( !((mqd_t)-1 != mq_listener) )
    	perror("MQueue Open listener");

    FD_SET(mq_listener, &master);
    fdmax = mq_listener;
    for (;;){
    	tv.tv_sec = 1;
    	tv.tv_usec = 500000;
    	read_fd = master;
    	rv = select(fdmax+1, &read_fd, NULL, NULL, &tv);
    	if(rv > 0){		// there are file descriptors to serve
    		for(i = 0; i <=fdmax; i++){
    			if (FD_ISSET(i, &read_fd)){
					if(i == mq_listener){
						new_mq_Proces_MOPS = ServeNewProcessConnection(&master, mq_listener);
						if(new_mq_Proces_MOPS > fdmax)
							fdmax = new_mq_Proces_MOPS;
					}
					else{
						ReceiveFromProcess(i);
					}
    			}
    		}
    	}
    	if(rv < 0)		// error occurred in select()
    	    perror("select");
    	if(rv == 0)		// timeout, we can do our things
    		SendToProcesses();
    }
}

int ReceiveFromProcess(int file_de){
	int bytes_read;
    uint8_t temp[MAX_QUEUE_SIZE+1];

	bytes_read = mq_receive(file_de, temp, MAX_QUEUE_SIZE, NULL);
	AnalizeProcessMessage(temp, bytes_read);
	return 0;
}

//TODO
int SendToProcesses(){
	printf("Tutaj sobie baki zbijam \n");
	return 0;
}

/*
 * Return:
 * 	file descriptor (int) - when there is place in MOPSQueue array
 * 	-1 					  - if there is not place in MOPSQueue array or no message received from listener_fd
 */
int ServeNewProcessConnection(fd_set *set, int listener_fd){
    struct mq_attr attr;
    uint8_t buffer[MAX_QUEUE_SIZE+1];
    int new_mq_Proces_MOPS, new_mq_MOPS_Proces;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_QUEUE_SIZE;
    attr.mq_curmsgs = 0;
    memset(buffer, 0, MAX_QUEUE_SIZE+1);

    if(mq_receive(listener_fd, buffer, MAX_QUEUE_SIZE, NULL) > 0){
		new_mq_Proces_MOPS = mq_open(buffer, O_CREAT | O_RDONLY, 0644, &attr);
		if( !((mqd_t)-1 != new_mq_Proces_MOPS) )
			perror("MQueue Open Proces_MOPS");
		new_mq_MOPS_Proces = mq_open(buffer, O_WRONLY);
		if( !((mqd_t)-1 != new_mq_MOPS_Proces) )
			perror("MQueue Open MOPS_Proces");

		if (AddToMOPSQueue(new_mq_MOPS_Proces, new_mq_Proces_MOPS) >= 0){
			FD_SET(new_mq_Proces_MOPS, set);
			printf("Nowy deskryptor: %d, nazwa kolejki: %s \n", new_mq_Proces_MOPS, buffer);
			return new_mq_Proces_MOPS;
		}
    }
    return -1;
}
#endif //TARGET_DEVICE == Linux


//TODO
#if TARGET_DEVICE == RTnode
void InitProcesConnection(){

	for(;;){}
}
#endif //TARGET_DEVICE == RTnode


void AnalizeProcessMessage(uint8_t *buffer, int bytes_wrote){
	FixedHeader FHeader;
	memcpy(&FHeader, buffer, sizeof(FHeader));

	switch(FHeader.MessageType){
	case PUBLISH:
		ServePublishMessage(FHeader, buffer, bytes_wrote);
		break;
	case SUBSCRIBE:
		break;
	}
}

void ServePublishMessage(FixedHeader FHeader, uint8_t *buffer, int bytes_wrote){
	uint8_t topicTemp[MAX_TOPIC_LENGTH+1], messageTemp[MAX_MESSAGE_LENGTH+1], QoS;
	uint16_t FrameLen, TopicLen, MessageLen, index = 0;
	int topicID;
	memset(topicTemp, 0, MAX_TOPIC_LENGTH+1);
	memset(messageTemp, 0, MAX_MESSAGE_LENGTH+1);

	FrameLen = MSBandLSBTou16(FHeader.RemainingLengthMSB, FHeader.RemainingLengthLSB);
	QoS = (FHeader.Flags & 6) >> 1;
	index+=3;
	TopicLen = MSBandLSBTou16(buffer[index], buffer[index+1]);
	index+=2;
	memcpy(topicTemp, buffer+index, TopicLen);
	index+=TopicLen;

	if(QoS==0){
		MessageLen = MSBandLSBTou16(buffer[index], buffer[index+1]);
		index+=2;
		memcpy(messageTemp, buffer+index, MessageLen);
	}
	else{
		index+=2;
		MessageLen = MSBandLSBTou16(buffer[index], buffer[index+1]);
		index+=2;
		memcpy(messageTemp, buffer+index, MessageLen);
	}
	printf("Topic: %s, Message: %s\n",topicTemp, messageTemp);
	topicID = GetIDfromTopicName(topicTemp, TopicLen);
	switch(topicID){
	case -1:
		AddTopicCandidate(topicTemp, TopicLen);
		PrintfList(list);
		//AddPacketToWaitingTab();
		break;
	case 0:
		//AddPacketToWaitingTab();
		break;
	default:
		AddPacketToFinalTab(, );
		break;
	}
}

void u16ToMSBandLSB(uint16_t u16bit, uint8_t *MSB, uint8_t *LSB){
	uint16_t temp;
	*LSB = (uint8_t) u16bit;
	temp = u16bit>>8;
	*MSB = (uint8_t) temp;
}

uint16_t MSBandLSBTou16(uint8_t MSB, uint8_t LSB){
	uint16_t temp;
	temp = MSB;
	temp = temp<<8;
	temp += LSB;
	return temp;
}
