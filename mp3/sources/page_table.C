#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;



void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
   PageTable::kernel_mem_pool = _kernel_mem_pool;
   PageTable::process_mem_pool = _process_mem_pool;
   PageTable::shared_size = _shared_size;
   
   Console::puts("Initialized Paging System\n");
}


PageTable::PageTable()
{   
   unsigned int index = 0;
   unsigned long address = 0;
   
   // Paging is disabled at first
   paging_enabled = 0;
   
   // Calculate number of frames shared - 4 MB / 4 KB = 1024
   unsigned long num_shared_frames = ( PageTable::shared_size / PAGE_SIZE );
   
   // Initialize page directory
   page_directory = (unsigned long *)( kernel_mem_pool->get_frames(1) * PAGE_SIZE );
   
   // Initialize page table
   unsigned long * page_table = (unsigned long *)( kernel_mem_pool->get_frames(1) * PAGE_SIZE );
   
   // Initializing page directory entries (PDEs) - Mark 1st PDE as valid
   page_directory[0] = ( (unsigned long)page_table | 0b11 );			// Supervisor level, R/W and Present bits are set
   
   // Remaining PDEs are marked invalid
   for( index = 1; index < num_shared_frames; index++ )
   {
	   page_directory[index] = ( page_directory[index] | 0b10 );		// Supervisor level and R/W bits are set, Present bit is not set
   }
   
   // Mapping first 4 MB of memory for page table - All pages marked as valid
   for( index = 0; index < num_shared_frames; index++ )
   {
	   page_table[index] = (address | 0b11);							// Supervisor level, R/W and Present bits are set   
	   address = address + PAGE_SIZE;
   }
   
   Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
   current_page_table = this;
   
   // Write page directory address into CR3 register
   write_cr3((unsigned long)(current_page_table->page_directory));
   
   Console::puts("Loaded page table\n");
}


void PageTable::enable_paging()
{
   write_cr0( read_cr0() | 0x80000000 );	// Set paging bit - 32nd bit
   paging_enabled = 1;						// Set paging_enabled variable
   
   Console::puts("Enabled paging\n");
}


void PageTable::handle_fault(REGS * _r)
{
	unsigned int index = 0;
	
	unsigned long error_code = _r->err_code;

	// Get the page fault address from CR2 register
	unsigned long fault_address = read_cr2();

	// Get the page directory address from CR3 register
	unsigned long * page_dir = (unsigned long *)read_cr3();

	// Extract page directory index - first 10 bits
	unsigned long page_dir_index = (fault_address >> 22);

	// Extract page table index using mask - next 10 bits
	unsigned long page_table_index = ( (fault_address >> 12) & 0x3FF );		// 0x3FF = 001111111111 - retain only last 10 bits

	unsigned long * new_page_table = NULL;

	// If page not present fault occurs
	if ((error_code & 1) == 0 )
	{
		// Check where page fault occured
		if ((page_dir[page_dir_index] & 1 ) == 0)
		{
			// Page fault occured in page directory - PDE is invalid
			
			// Allocate a frame from kernel pool for page directory and mark flags - supervisor, R/W, Present bits
			page_dir[page_dir_index] = (unsigned long)(kernel_mem_pool->get_frames(1)*PAGE_SIZE | 0b11);		// PDE marked valid
			
			// Clear last 12 bits to erase all flags using mask
			new_page_table = (unsigned long *)(page_dir[page_dir_index] & 0xFFFFF000);

			// Set flags for each page - PTEs marked invalid
			for(index=0; index < 1024; index++)
			{
				// Set user level flag bit
				new_page_table[index] = 0b100;
			}
		}

		else
		{
			// Page fault occured in page table page - PDE is present, but PTE is invalid
			
			// Clear last 12 bits to erase all flags using mask
			new_page_table = (unsigned long *)(page_dir[page_dir_index] & 0xFFFFF000);
			
			// Allocate a frame from process pool and mark flags - supervisor, R/W, Present bits
			new_page_table[page_table_index] =  PageTable::process_mem_pool->get_frames(1)*PAGE_SIZE | 0b11;	// PTE marked valid
		}
	}

	Console::puts("handled page fault\n");
}

