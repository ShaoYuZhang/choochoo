#ifndef _PROCESS_PRIORITY_H_
#define _PROCESS_PRIORITY_H_

/* Process Priority. The bigger the number is, the lower the priority is*/

enum Process_Priority
{ HIGHEST,
  HIGH,
  MEDIUM,
  LOW,
  LOWEST,
  BLOCKED_MEMORY,
  BLOCKED_MESSAGE,
  I_PROCESS_QUEUE,
  NUM_OF_ALL_QUEUES_USED_BY_SYSTEM
};

typedef enum Process_Priority Queue_Priority;
typedef enum Process_Priority Process_Priority;

#define NUM_OF_PRIORITIES  LOWEST + 1
#define NUM_OF_QUEUES BLOCKED_MESSAGE + 1

#define I_PROCESS_PRIORITY -1


#endif /* _PROCESS_PRIORITY_H_ */
