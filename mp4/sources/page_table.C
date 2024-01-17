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
VMPool * PageTable::vm_pool_head = NULL;


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
	page_directory[num_shared_frames-1] = ( (unsigned long)page_directory | 0b11 );
	
	// Initialize page table
	unsigned long * page_table = (unsigned long *)( process_mem_pool->get_frames(1) * PAGE_SIZE );
	
	// Initializing page directory entries (PDEs) - Mark 1st PDE as valid
	page_directory[0] = ( (unsigned long)page_table | 0b11 );

	// Remaining PDEs are marked invalid
	for( index = 1; index < num_shared_frames-1; index++ )
	{
		// Supervisor level and R/W bits are set, Present bit is not set
		page_directory[index] = ( page_directory[index] | 0b10 );
	}
	
	// Mapping first 4 MB of memory for page table - All pages marked as valid
	for( index = 0; index < num_shared_frames; index++ )
	{
		// Supervisor level, R/W and Present bits are set  
		page_table[index] = (address | 0b11); 
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
	unsigned long error_code = _r->err_code;
	
	// If page not present fault occurs
	if( (error_code & 1) == 0 )
	{
		// Get the page fault address from CR2 register
		unsigned long fault_address = read_cr2();

		// Get the page directory address from CR3 register
		unsigned long * page_dir = (unsigned long *)read_cr3();

		// Extract page directory index - first 10 bits
		unsigned long page_dir_index = (fault_address >> 22);

		// Extract page table index using mask - next 10 bits 
		// 0x3FF = 001111111111 - retain only last 10 bits
		unsigned long page_table_index = ( (fault_address & (0x03FF << 12) ) >> 12 );
		
		unsigned long *new_page_table = NULL; 
		unsigned long *new_pde = NULL;
		
		// Check if logical address is valid and legitimate
		unsigned int present_flag = 0;
		
		// Iterate through VM pool regions
		VMPool * temp = PageTable::vm_pool_head;
		
		for( ; temp != NULL; temp = temp->vm_pool_next )
		{
			if( temp->is_legitimate(fault_address) == true )
			{
				present_flag = 1;
				break;
			}
		}
		
		if( (temp != NULL) && (present_flag == 0) )
		{
		  Console::puts("Not a legitimate address.\n");
		  assert(false);	  	
		}
		
		// Check where page fault occured
		if ( (page_dir[page_dir_index] & 1 ) == 0 )
		{
			// Page fault occured in page directory - PDE is invalid
			
			int index = 0;
			
			new_page_table = (unsigned long *)(process_mem_pool->get_frames(1) * PAGE_SIZE);
			
			// PDE Address = 1023 | 1023 | Offset
			unsigned long * new_pde = (unsigned long *)( 0xFFFFF << 12 );               
			new_pde[page_dir_index] = ( (unsigned long)(new_page_table)| 0b11 );
			
			// Set flags for each page - PTEs marked invalid
			for( index = 0; index < 1024; index++ )
			{
				// Set user level flag bit
				new_page_table[index] = 0b100;
			}
			
			// To avoid raising another page fault, handle invalid PTE case as well
			new_pde = (unsigned long *) (process_mem_pool->get_frames(1) * PAGE_SIZE);
			
			// PTE Address = 1023 | PDE | Offset
			unsigned long * page_entry = (unsigned long *)( (0x3FF << 22) | (page_dir_index << 12) );
			
			// Mark PTE valid
			page_entry[page_table_index] = ( (unsigned long)(new_pde) | 0b11 );
		}

		else
		{
			// Page fault occured in page table page - PDE is present, but PTE is invalid
			new_pde = (unsigned long *) (process_mem_pool->get_frames(1) * PAGE_SIZE);
			
			// PTE Address = 1023 | PDE | Offset
			unsigned long * page_entry = (unsigned long *)( (0x3FF << 22)| (page_dir_index << 12) );
			
			page_entry[page_table_index] = ( (unsigned long)(new_pde) | 0b11 );
		}
	}

	Console::puts("handled page fault\n");
}


void PageTable::register_pool(VMPool * _vm_pool)
{	
	// Register the initial virtual memory pool
	if( PageTable::vm_pool_head == NULL )
	{
		PageTable::vm_pool_head = _vm_pool;
	}
	
	// Register subsequent virtual memory pools
	else
	{
		VMPool * temp = PageTable::vm_pool_head;
		for( ; temp->vm_pool_next != NULL; temp = temp->vm_pool_next );
		
		// Add pool to end of linked list
		temp->vm_pool_next = _vm_pool;
	}
	
    Console::puts("registered VM pool\n");
}


void PageTable::free_page(unsigned long _page_no)
{
	// Extract page directory index - first 10 bits
	unsigned long page_dir_index = ( _page_no & 0xFFC00000) >> 22;
	
	// Extract page table index using mask - next 10 bits
	unsigned long page_table_index = (_page_no & 0x003FF000 ) >> 12;
	
	// PTE Address = 1023 | PDE | Offset
	unsigned long * page_table = (unsigned long *) ( (0x000003FF << 22) | (page_dir_index << 12) );
	
	// Obtain frame number to release
	unsigned long frame_no = ( (page_table[page_table_index] & 0xFFFFF000) / PAGE_SIZE );
	
	// Release frame from process pool
	process_mem_pool->release_frames(frame_no);
	
	// Mark PTE as invalid
	page_table[page_table_index] = page_table[page_table_index] | 0b10;
	
	// Flush TLB by reloading page table
	load();
	
	Console::puts("freed page\n");
}
