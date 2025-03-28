#include "akua_config.h"

akuaQueue* createQueue()
{
	akuaQueue* queue = (akuaQueue*)malloc(sizeof(akuaQueue));
	queue->front = queue->rear = NULL;
	return queue;
}

void enqueue(akuaQueue* queue, void* data)
{
	akuaNode* newNode = (akuaNode*)malloc(sizeof(akuaNode));
	newNode->data = data;
	newNode->next = NULL;

	if (queue->rear == NULL)
	{
		queue->front = queue->rear = newNode;
		return;
	}

	queue->rear->next = newNode;
	queue->rear = newNode;
}

void* dequeue(akuaQueue* queue)
{
	if (queue->front == NULL)
	{
		return NULL;
	}

	akuaNode* temp = queue->front;
	void* data = temp->data;
	queue->front = queue->front->next;

	if (queue->front == NULL)
	{
		queue->rear = NULL;
	}

	free(temp);
	return data;
}

void freeQueue(akuaQueue* queue)
{
	akuaNode* current = queue->front;
	akuaNode* next;

	while (current != NULL)
	{
		next = current->next;
		free(current);
		current = next;
	}

	free(queue);
}
