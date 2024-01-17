/*
 File: queue.H
 
 Author: Pranav Anantharam
 Date  : 11/17/2023
 
 */
#ifndef QUEUE_H
#define QUEUE_H

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "thread.H"

/*--------------------------------------------------------------------------*/
/* QUEUE DATA STRUCTURE */
/*--------------------------------------------------------------------------*/

class Queue
{
	private:
	
	Thread* thread;					// Pointer to thread at the top of queue
	Queue* next;					// Pointer to next thread in queue. 
		
	public:
	
	// Constructor for initial setup
	Queue()
	{
		thread = nullptr;
		next   = nullptr;
	}
	
	// Constructor for new threads to enter queue
	Queue(Thread* new_thread)
	{
		thread = new_thread;
		next = nullptr;
	}
	
	// Add thread at end of queue
	void enqueue(Thread* new_thread)
	{
		// First thread added to queue
		if ( thread == nullptr )
		{
			thread = new_thread;
		}
		else
		{			
			// Traverse the queue
			if( next != nullptr )
			{
				next->enqueue(new_thread);
			}
			else
			{
				// Reached end of queue - add new thread at the end of queue
				next =  new Queue(new_thread);
			}
		}
	}
	
	// Remove thread at head position and point to next thread in queue
	Thread* dequeue()
	{
		// Queue is empty
		if( thread == nullptr )
		{
			return nullptr; 
		}
		
		// Get top of queue element
		Thread *top = thread;
		
		// Only one queue element present
		if( next == nullptr )
		{
			thread = nullptr;
		}
		else
		{
			// Update head element of queue
			thread = next->thread;
			
			// Delete current node
			Queue * del_node = next;
			
			// Update next pointer
			next = next->next;
			
			delete del_node;
		}
		
		return top;
	}
};

#endif
