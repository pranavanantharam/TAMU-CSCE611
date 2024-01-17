/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define INODE_BLOCK_NO		0
#define FREELIST_BLOCK_NO	1
#define DISK_BLOCK_SIZE		512
#define END_INDICATOR		0xFFFFFFFF		// To denote -1

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CLASS Inode */
/*--------------------------------------------------------------------------*/

/* You may need to add a few functions, for example to help read and store 
   inodes from and to disk. */

/*--------------------------------------------------------------------------*/
/* CLASS FileSystem */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem()
{
	Console::puts("In file system constructor.\n");
	inodes = new Inode [DISK_BLOCK_SIZE];
	free_blocks = new unsigned char [DISK_BLOCK_SIZE]; 
}

FileSystem::~FileSystem()
{
	Console::puts("unmounting file system\n");
	/* Make sure that the inode list and the free list are saved. */
	
	write_inode_block_to_disk();
	write_freelist_block_to_disk();
	
	delete []inodes;
	delete []free_blocks;
}


/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/


int FileSystem::GetFreeBlock()
{
    unsigned int index = 0;
	for(index = 0; index < DISK_BLOCK_SIZE; index++)
    {
    	if( free_blocks[index] == 0 )
		{
			// Return free block number
			return index;
		}                
    }
    
	// Free block not available, return -1 
	return END_INDICATOR;         
}

short FileSystem::GetFreeInode()
{
	unsigned int index = 0;
	for(index = 0; index < MAX_INODES; index++)
	{
		if( inodes[index].id == END_INDICATOR )
		{
			// Return Free Inode
			return index;
		}
	}
	
	// Free Inode not available, return -1
	return END_INDICATOR;
}


bool FileSystem::Mount(SimpleDisk * _disk) {
    Console::puts("mounting file system from disk \n");

    /* Here you read the inode list and the free list into memory */
	disk = _disk;
	
	// Load Inode Block
	read_inode_block_from_disk();
	
	// Load FreeList Block
	read_freelist_block_from_disk();
	
	// Check if first 2 blocks in disk are used
	if( (free_blocks[0] == 1) && (free_blocks[1] == 1) )
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) { // static!
    Console::puts("formatting disk \n");
    /* Here you populate the disk with an initialized (probably empty) inode list
       and a free list. Make sure that blocks used for the inodes and for the free list
       are marked as used, otherwise they may get overwritten. */
	
	unsigned int index = 0;
	unsigned char buffer[DISK_BLOCK_SIZE];
	
	// Initialize inode block to be empty
	for( index = 0; index < DISK_BLOCK_SIZE; index++ )
	{
		buffer[index] = END_INDICATOR;
	}
	
	_disk->write(INODE_BLOCK_NO, buffer);
	
	// Initialize free list to be empty
	for( index = 0; index < DISK_BLOCK_SIZE; index++ )
	{
		buffer[index] = 0x00;
	}
	
	// Set Inode block as used
	buffer[INODE_BLOCK_NO] = 1;
	
	// Set Free List block as used
	buffer[FREELIST_BLOCK_NO] = 1;
	
	_disk->write(FREELIST_BLOCK_NO, buffer);
	
	return true;
}

Inode * FileSystem::LookupFile(int _file_id)
{
    Console::puts("looking up file with id = "); Console::puti(_file_id); Console::puts("\n");
    /* Here you go through the inode list to find the file. */
	
	unsigned int index = 0;
	for(index = 0; index < MAX_INODES; index++)
    {
    	if( inodes[index].id == _file_id )
		{
			return &inodes[index];
		}
    }
	
    Console::puts("LookupFile: file does not exist \n");
    return NULL;
}

bool FileSystem::CreateFile(int _file_id)
{
    Console::puts("creating file with id:"); Console::puti(_file_id); Console::puts("\n");
    /* Here you check if the file exists already. If so, throw an error.
       Then get yourself a free inode and initialize all the data needed for the
       new file. After this function there will be a new file on disk. */
	
	short free_block_no = 0;
	int free_inode_idx = 0;
	
	if( LookupFile(_file_id) != NULL )
    {
		Console::puts("CreateFile: file exists already, cannot create file \n");
		return false;
    }
	
    free_block_no = GetFreeBlock();
    if( free_block_no == END_INDICATOR )
    {
		Console::puts("CreateFile: free blocks not available \n");
		return false;	
    }
    
	free_inode_idx = GetFreeInode();
    if( free_inode_idx == END_INDICATOR )
    {
		Console::puts("CreateFile: free inodes not available \n");
		return false;	
    }
	
    free_blocks[free_block_no] = 1;
	
	inodes[free_inode_idx].size = 0;
	inodes[free_inode_idx].id = _file_id;
    inodes[free_inode_idx].block_no = free_block_no;
    inodes[free_inode_idx].fs = this;
	
    write_inode_block_to_disk();
	write_freelist_block_to_disk();
	
	Console::puts("CreateFile: created file having id: ");
	Console::puti(_file_id);
	Console::puts("\n");
	
    return true;
}

bool FileSystem::DeleteFile(int _file_id)
{
	Console::puts("deleting file with id:"); Console::puti(_file_id); Console::puts("\n");
	/* First, check if the file exists. If not, throw an error. 
		Then free all blocks that belong to the file and delete/invalidate 
		(depending on your implementation of the inode list) the inode. */
	
	// Check if file exists
	Inode * inode = LookupFile( _file_id );
	if( inode != NULL )
	{
		free_blocks[inode->block_no] = 0;
		
		inode->id = END_INDICATOR;
		inode->size = END_INDICATOR;
		inode->block_no = END_INDICATOR;
		
		write_inode_block_to_disk();
		write_freelist_block_to_disk();
		
		return true;
	}
	else
	{
		return false;
	}
	
}


void FileSystem::read_inode_block_from_disk()
{
	disk->read( INODE_BLOCK_NO, (unsigned char *)inodes );
}


void FileSystem::write_inode_block_to_disk()
{
	disk->write( INODE_BLOCK_NO, (unsigned char *)inodes );
}


void FileSystem::read_freelist_block_from_disk()
{
	disk->read( FREELIST_BLOCK_NO, free_blocks );
}


void FileSystem::write_freelist_block_to_disk()
{
	disk->write( FREELIST_BLOCK_NO, free_blocks );
}


void FileSystem::write_block_to_disk( unsigned long block_number, unsigned char * buffer )
{
	disk->write( block_number, buffer );
}


void FileSystem::read_block_from_disk( unsigned long block_number, unsigned char * buffer )
{
	disk->read( block_number, buffer );
}
