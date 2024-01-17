/*
     File        : blocking_disk.c

     Author      : Pranav Anantharam
     Modified    : 11/17/2023

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "blocking_disk.H"
#include "scheduler.H"
#include "thread.H"
#include "machine.H"

extern Scheduler * SYSTEM_SCHEDULER;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size)
{
	blocked_queue = new Queue();
	blocked_queue_size = 0;
}

/*--------------------------------------------------------------------------*/
/* BLOCKED DISK OPERATIONS */
/*--------------------------------------------------------------------------*/

Thread * BlockingDisk::get_top_thread()
{
	// Get the thread at the top of the blocked queue and return
	Thread *top = this->blocked_queue->dequeue();
	this->blocked_queue_size--;
	return top;
}


void BlockingDisk::wait_until_ready()
{
	// Add current thread to end of blocked threads queue
	this->blocked_queue->enqueue(Thread::CurrentThread());
	this->blocked_queue_size++;
	SYSTEM_SCHEDULER->yield();
}

bool BlockingDisk::check_blocked_thread_in_queue()
{
	// Check if disk is ready to transfer data and threads exist in blocked state
	return ( SimpleDisk::is_ready() && (this->blocked_queue_size > 0) );
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf)
{	
	SimpleDisk::read(_block_no, _buf);
}


void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf)
{
	SimpleDisk::write(_block_no, _buf);
}
