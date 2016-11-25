/**
 *	@file	MOPS.h
 *	@date	Mar 10, 2016
 *	@author	Michal Oleszczyk
 *	@brief	File defines, structs, enums and function headers.
 *
 *	Headers set of functions for broker-process communication
 *	and broker logic in general. Here are also stored all defines
 *	responsible for main configuration of MOPS parameters.
 */

#ifndef MOPS_H_
#define MOPS_H_

#include <stdint.h>
#include <stdio.h>
#include <semaphore.h>
#include <sys/types.h>
#include "MOPS_config.h"
#include "MQTT.h"
#include "MOPS_Linux.h"

/**
 * @struct TopicID
 * @brief Structure links topic name and its ID.
 *
 * This type is used to create 'topic list'. This struct
 * contain information about already known topics (its ID),
 * and about 'candidates' (ID=0). Moreover if field
 * 'LocalTopic' is set, topic has to be announced to
 * all RTnet participants.
 */
typedef struct TopicID {
	/** Topic name - at most MAX_TOPIC_LENGTH long string. */
	uint8_t Topic[MAX_TOPIC_LENGTH + 1];
	/** ID of this topic. */
	uint16_t ID;
	/** If this field is equal 1, this topic has to be announced in RTnet. */
	uint8_t LocalTopic;
} TopicID;

/**
 * @struct SubscriberList
 * @brief Structure creating subscriptions.
 *
 * This type is used in 'subscription list'.
 * Struct contain topic name and clients ID of
 * processes which subscribe this particular topic.
 */
typedef struct SubscriberList {
	/** Topic name - at most MAX_TOPIC_LENGTH long string. */
	uint8_t Topic[MAX_TOPIC_LENGTH + 1];
	/** Client ID of process which subscribe this topic. */
	int ClientID;
} SubscriberList;

/**
 * @struct PublishHandler
 * @brief Handler used for publishing
 *
 * This type is returned by advertising function.
 * It contains topic name and poniter to publishing function.
 */
typedef struct PublishHandler {
	/** Topic name - at most MAX_TOPIC_LENGTH long string. */
	char *TopicName;
	/** Pointer to publishing finction. */
	void (*publish)();
} PublishHandler;

typedef void (*callBackFun)(void*);

/**
 * @struct Subscription
 * @brief Keeps pairs <Topic+callBack>
 *
 * This type is used to keep topics and their callbacks in process.
 */
typedef struct Subscription {
	/** Topic name - at most MAX_TOPIC_LENGTH long string. */
	char *TopicName;
	/** Pointer to publishing finction. */
	callBackFun callBack;
} Subscription;


/**
 * @enum MOPS_STATE
 * @brief State of MOPS broker.
 *
 * Describes how MOPS broker should behavior, which MOPS protocol header
 * should build and send to RTnet.
 * */
enum MOPS_STATE {
	SEND_NOTHING = 1,/**< Usual state - process as usual, receive and send messages. */
	SEND_REQUEST,    /**< Init state - responsible for request for all known topics list. */
	SEND_TOPIC_LIST, /**< Topics response - set when some RTnet participant requested for full topic list.*/
};

// ***************   Funtions for local processes   ***************//
int connectToMOPS();
int sendToMOPS(char *buffer, uint16_t buffLen);
int recvFromMOPS(char *buffer, uint16_t buffLen);

void publishMOPShdlr(char* Message, PublishHandler *self);
PublishHandler advertiseMOPS(char *Topic);
void publishMOPS(char *Topic, char *Message, int MessageLen);
void subscribeOnceMOPS(char *TopicName, uint8_t Qos);
void subscribeMOPS2(char *TopicName, uint8_t Qos, void (*callBack)(void*));
void subscribeMOPS(char **TopicName, uint8_t *QosList, uint8_t NoOfTopics);
int readMOPS(char *buf, uint8_t length);
int readMOPS2(char *buf, uint8_t length);
int InterpretFrame(char *messageBuf, char *frameBuf, uint8_t frameLen);
int InterpretFrame2(char *messageBuf, char *topicName, char *frameBuf, uint8_t frameLen);
callBackFun getCallBackByTopicName(char *TopicName, uint16_t topicLen);
// ***************   Funtions for local processes   ***************//

// ***************   Funtions for local MOPS broker   ***************//
int StartMOPSBroker();
int StartMOPSBrokerNonBlocking();
int StopMOPSBroker(void);
void threadSendToRTnet();
void threadRecvFromRTnet();

void AddClientIDToPacket(uint8_t *buf, uint8_t ClientID, int *WrittenBytes,
		int nbytes);
void InitTopicList(TopicID list[]);
void SubListInit(SubscriberList *sublist);

uint16_t SendEmptyMessage();
uint16_t SendTopicRequestMessage();
uint16_t SendTopicList(TopicID list[]);
uint16_t SendLocalTopics(TopicID list[]);
uint8_t AddTopicToList(TopicID list[], uint8_t *topic, uint16_t topicLen,
		uint16_t id);
void AnalyzeIncomingUDP(uint8_t *Buffer, int written_bytes);
void UpdateTopicList(uint8_t *Buffer, int BufferLen);
uint8_t ApplyIDtoNewTopics();
void AddTopicCandidate(uint8_t *topic, uint16_t topicLen);
int GetIDfromTopicName(uint8_t *topic, uint16_t topicLen);
uint16_t GetTopicNameFromID(uint16_t id, uint8_t *topic);
void InitProcesConnection();
void CloseProcessConnection(int file_de);
int AddToMOPSQueue(int MOPS_Proces_fd, int Proces_MOPS_fd);
void MOPS_QueueInit(MOPS_Queue *queue);
int AddToSubscribersList(uint8_t *topic, uint16_t topicLen, int ClientID);
void PrepareFrameToSendToProcess(uint8_t *Buffer, int written_bytes);
int ReceiveFromProcess(int file_de);
int SendToProcess(uint8_t *buffer, uint16_t buffLen, int file_de);
int ServeSendingToProcesses();
int FindClientIDbyFileDesc(int file_de);
void FindClientsIDbyTopic(int *clientsID, uint8_t *topic, uint16_t topicLen);
void AnalyzeProcessMessage(uint8_t *buffer, int bytes_wrote, int ClientID);
void ServePublishMessage(uint8_t *buffer, int FrameLen);
void ServeSubscribeMessage(uint8_t *buffer, int FrameLen, int ClientID);
void AddPacketToWaitingTab(uint8_t *buffer, int FrameLen);
void AddPacketToFinalTab(uint8_t *buffer, int FrameLen, uint16_t topicID);
void MoveWaitingToFinal();
void DeleteProcessFromQueueList(int ClientID, MOPS_Queue *queue);
// ***************   Funtions for local MOPS broker   ***************//

// ***************   Tools functions   ******************************//
void u16ToMSBandLSB(uint16_t u16bit, uint8_t *MSB, uint8_t *LSB);
uint16_t MSBandLSBTou16(uint8_t MSB, uint8_t LSB);
void MOPSBrokerTargetInit(void);
uint8_t waitOnTDMASync(void);
uint8_t RTnetConnTargetDependentInit(void);
void startRandomGenrator(void);

// ***************   Tools functions   ******************************//

// *************** Global variables for local processes *************** //

// *************** Global variables for local processes *************** //

// *************** Global variables for MOPS broker *************** //
extern uint8_t MOPS_State;
uint8_t input_buffer[UDP_MAX_SIZE];				/**< Buffer for receiving data from RTnet. */

uint8_t waiting_output_buffer[UDP_MAX_SIZE]; 	/**< Buffer for incoming data from processes
											 	* (waiting for sending them to RTnet). */
uint8_t waiting_input_buffer[UDP_MAX_SIZE];  	/**< Buffer for outgoing data to processes
											 	* (waiting for sending them to processes). */

sem_t sem;

extern uint16_t input_index;			/**< Index of written bytes to #input_buffer. */
extern uint16_t output_index;			/**< Index of written bytes to #output_buffer. */
extern uint16_t waiting_output_index;	/**< Index of written bytes to #waiting_output_buffer. */
extern uint16_t waiting_input_index;	/*< Index of written bytes to #waiting_input_buffer. */

TopicID list[MAX_NUMBER_OF_TOPIC]; /**< List of all known topics with their IDs. ID=0 is for candidates.*/
SubscriberList sub_list[MAX_NUMBER_OF_SUBSCRIPTIONS]; /**< List of all subscribers ID and subscribed topics by them. */
MOPS_Queue mops_queue[MAX_PROCES_CONNECTION]; /**< List of connected processes to broker. */
Subscription procOwnSubs[MAX_NUMBER_OF_SUBSCRIPTIONS];
// *************** Global variables for MOPS broker *************** //

#endif /* MOPS_H_ */
