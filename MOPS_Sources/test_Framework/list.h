#include <stdbool.h>

/**
 * @brief Adds element to linked list.
 *
 * @param[in] packetData First field of node.
 * @param[in] timestamp Second field of node.
 */
typedef struct node 
{
   int packetData;
   long long timestamp;
   struct node *next;
} my_node;


/**
 * @brief Adds element to linked list.
 *
 * @param[in] packetData First field of node.
 * @param[in] timestamp Second field of node.
 */
void insertFirst(my_node **_head, int packetData, long long timestamp) 
{
   my_node *ptr = (my_node*) malloc(sizeof(my_node));

   ptr->packetData = packetData;
   ptr->timestamp = timestamp;

   ptr->next = *_head;

   *_head = ptr;
}

/**
 * @brief Adds element to linked list.
 *
 * @param[in] packetData First field of node.
 * @param[in] timestamp Second field of node.
 */
bool isEmpty(my_node **head) 
{
   return *head == NULL;
}

/**
 * @brief Adds element to linked list.
 *
 * @param[in] packetData First field of node.
 * @param[in] timestamp Second field of node.
 */
my_node* findByValue(my_node **head, int packetData) 
{

   my_node* current = *head;

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
my_node* deleteByValue(my_node **head, int packetData)
{

   my_node* current = *head;
   my_node* previous = NULL;
	
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

void printList(my_node **head) {
   my_node *ptr = *head;
   printf("\n[ ");
	
   while(NULL != ptr)
   {
      printf("(%d,%lld) ", ptr->packetData, ptr->timestamp);
      ptr = ptr->next;
   }
	
   printf(" ]\n");
}