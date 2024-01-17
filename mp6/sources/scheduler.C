/*
 File: scheduler.C
 
 Author: Pranav Anantharam
 Date  : 10/31/2023
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "machine.H"

extern BlockingDisk * SYSTEM_DISK;

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/


Scheduler::Scheduler()
{
	qsize = 0;
	Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield()
{
	// Disable interrupts when performing any operations on ready queue
	if( Machine::interrupts_enabled() )
	{
		Machine::disable_interrupts();
	}
	
	// Check if disk is ready and blocked threads exist in queue
	// If disk is ready, immediately dispatch to top thread in blocked queue
	if( SYSTEM_DISK->check_blocked_thread_in_queue() )
	{
		// Re-enable interrupts
		if( !Machine::interrupts_enabled() )
		{
			Machine::enable_interrupts();
		}
		
		Thread::dispatch_to(SYSTEM_DISK->get_top_thread());
	}
	
	// If disk is not ready or blocked threads do not exist in queue
	// Check the regular FIFO ready queue
	else
	{
		if( qsize == 0 )
		{
			Console::puts("Queue is empty. No threads available. \n");
		}
		else
		{
			// Remove thread from queue for CPU time
			Thread * new_thread = ready_queue.dequeue();
			
			// Decrement queue size
			qsize = qsize - 1;
			
			// Re-enable interrupts
			if( !Machine::interrupts_enabled() )
			{
				Machine::enable_interrupts();
			}
			
			// Context-switch and give CPU time to new thread
			Thread::dispatch_to(new_thread);
		}
	}	
}


void Scheduler::resume(Thread * _thread)
{
	// Disable interrupts when performing any operations on ready queue
	if( Machine::interrupts_enabled() )
	{
		Machine::disable_interrupts();
	}
	
	// Add thread to ready queue
	ready_queue.enqueue(_thread);
	
	// Increment queue size
	qsize = qsize + 1;
	
	// Re-enable interrupts
	if( !Machine::interrupts_enabled() )
	{
		Machine::enable_interrupts();
	}
}


void Scheduler::add(Thread * _thread)
{
	// Disable interrupts when performing any operations on ready queue
	if( Machine::interrupts_enabled() )
	{
		Machine::disable_interrupts();
	}
	
	// Add thread to ready queue
	ready_queue.enqueue(_thread);
	
	// Increment queue size
	qsize = qsize + 1;
	
	// Re-enable interrupts
	if( !Machine::interrupts_enabled() )
	{
		Machine::enable_interrupts();
	}
}


void Scheduler::terminate(Thread * _thread)
{
	// Disable interrupts when performing any operations on ready queue
	if( Machine::interrupts_enabled() )
	{
		Machine::disable_interrupts();
	}
	
	int index = 0;
	
	// Iterate and dequeue each thread
	// Check if thread ID matches with thread to be terminated
	// If thread ID does not match, add thread back to ready queue
	for( index = 0; index < qsize; index++ )
	{
		Thread * top = ready_queue.dequeue();
		
		if( top->ThreadId() != _thread->ThreadId() )
		{
			ready_queue.enqueue(top);
		}
		else
		{
			qsize = qsize - 1;
		}
	}
	
	// Re-enable interrupts
	if( !Machine::interrupts_enabled() )
	{
		Machine::enable_interrupts();
	}
}
