/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define DISK_BLOCK_SIZE		512

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File(FileSystem *_fs, int _id)
{
    Console::puts("Opening file.\n");
	fs = _fs;
	inode = fs->LookupFile(_id);
	current_position = 0;
	fs->read_block_from_disk( inode->block_no, block_cache );
}

File::~File()
{
    Console::puts("Closing file.\n");
    /* Make sure that you write any cached data to disk. */
    /* Also make sure that the inode in the inode list is updated. */
	fs->write_block_to_disk( inode->block_no, block_cache );
	fs->write_inode_block_to_disk();
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char *_buf)
{
	Console::puts("reading from file\n");
	
	unsigned long read_count = 0;
	
	while( (read_count < _n) && (EoF() == false) )
	{
		_buf[read_count] = block_cache[current_position];
		read_count = read_count + 1;
		current_position = current_position + 1;
	}
	
	return read_count;
}

int File::Write(unsigned int _n, const char *_buf)
{
    Console::puts("writing to file\n");
	
	unsigned int write_count = 0;
	unsigned int updated_inode_size = current_position + _n;
	
	// Updating inode size if required
	if( updated_inode_size > inode->size )
	{
		inode->size = updated_inode_size;
	}
	
	// Since disk block size = 512 bytes
	if( inode->size > DISK_BLOCK_SIZE )
	{
		inode->size = DISK_BLOCK_SIZE;
	}
	
	while( !EoF() )
	{
		block_cache[current_position] = _buf[write_count];
		write_count = write_count + 1;
		current_position = current_position + 1;
	}
	
	return write_count;
}

void File::Reset()
{
    Console::puts("resetting file\n");
	current_position = 0;
}

bool File::EoF()
{
    Console::puts("checking for EoF\n");
	if( current_position == inode->size )
	{
		return true;
	}
	else
	{
		return false;
	}
}
