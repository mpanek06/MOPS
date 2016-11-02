#include <stdbool.h>

/**
 * @brief Struct used in linked list
 *
 * Struct used in linked list
 */
typedef struct node 
{
   int packetData;
   long long timestamp;
   struct node *next;
} listNode;


/**
 * @brief Adds element to linked list.
 *
 * @param[in] packetData First field of node.
 * @param[in] timestamp Second field of node.
 */
void insertFirst(listNode **_head, int packetData, long long timestamp) 
{
   listNode *ptr = (listNode*) malloc(sizeof(listNode));

   ptr->packetData = packetData;
   ptr->timestamp = timestamp;

   ptr->next = *_head;

   *_head = ptr;
}

/**
 * @brief Checks if list is empty.
 *
 * @param[in] packetData First field of node.
 * @param[in] timestamp Second field of node.
 */
bool isEmpty(listNode **head) 
{
   return *head == NULL;
}

/**
 * @brief Adds element to linked list.
 *
 * @param[in] packetData First field of node.
 * @param[in] timestamp Second field of node.
 */
listNode* findByValue(listNode **head, int packetData) 
{

   listNode* current = *head;

   if(NULL == *head) 
   {
      return NULL;
   }

   while(current->packetData != packetData) 
   {
	
      if(NULL == current->next)
      {
         return NULL;
      } 
      else
      {
         current = current->next;
      }
   }      
	
   return current;
}

/**
 * @brief Adds element to linked list.
 *
 * @param[in] packetData First field of node.
 * @param[in] timestamp Second field of node.
 */
listNode* deleteByValue(listNode **head, int packetData)
{

   listNode* current = *head;
   listNode* previous = NULL;
	
   if(NULL == *head) 
   {
      return NULL;
   }

   while(current->packetData != packetData)
   {

      if(NULL == current->next)
      {
         return NULL;
      } 
      else 
      {
         previous = current;
         current = current->next;
      }
   }

   if(current == *head)
   {
      *head = (*head)->next;
   } 
   else
   {
      previous->next = current->next;
   }    
	
   return current;
}

void printList(listNode **head) {
   listNode *ptr = *head;
   printf("\n[ ");
	
   while(NULL != ptr)
   {
      printf("(%d,%lld) ", ptr->packetData, ptr->timestamp);
      ptr = ptr->next;
   }
	
   printf(" ]\n");
}