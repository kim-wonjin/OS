//
// OS 2021
// Simple FIle System
// Student Name : 김원진
// Student Number : B911040
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* optional */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
/***********/

#include "sfs_types.h"
#include "sfs_func.h"
#include "sfs_disk.h"
#include "sfs.h"

void dump_directory();

/* BIT operation Macros */
/* a=target variable, b=bit number to act upon 0-n */
#define BIT_SET(a,b) ((a) |= (1<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1<<(b)))
#define BIT_CHECK(a,b) ((a) & (1<<(b)))

static struct sfs_super spb;	// superblock
static struct sfs_dir sd_cwd = { SFS_NOINO }; // current working directory

void error_message(const char *message, const char *path, int error_code) {
	switch (error_code) {
	case -1:
		printf("%s: %s: No such file or directory\n",message, path); return;
	case -2:
		printf("%s: %s: Not a directory\n",message, path); return;
	case -3:
		printf("%s: %s: Directory full\n",message, path); return;
	case -4:
		printf("%s: %s: No block available\n",message, path); return;
	case -5:
		printf("%s: %s: Not a directory\n",message, path); return;
	case -6:
		printf("%s: %s: Already exists\n",message, path); return;
	case -7:
		printf("%s: %s: Directory not empty\n",message, path); return;
	case -8:
		printf("%s: %s: Invalid argument\n",message, path); return;
	case -9:
		printf("%s: %s: Is a directory\n",message, path); return;
	case -10:
		printf("%s: %s: Is not a file\n",message, path); return;
	default:
		printf("unknown error code\n");
		return;
	}
}

void sfs_mount(const char* path)
{
	if( sd_cwd.sfd_ino !=  SFS_NOINO )
	{
		//umount
		disk_close();
		printf("%s, unmounted\n", spb.sp_volname);
		bzero(&spb, sizeof(struct sfs_super));
		sd_cwd.sfd_ino = SFS_NOINO;
	}

	printf("Disk image: %s\n", path);

	disk_open(path);
	disk_read( &spb, SFS_SB_LOCATION );

	printf("Superblock magic: %x\n", spb.sp_magic);

	assert( spb.sp_magic == SFS_MAGIC );
	
	printf("Number of blocks: %d\n", spb.sp_nblocks);
	printf("Volume name: %s\n", spb.sp_volname);
	printf("%s, mounted\n", spb.sp_volname);
	
	sd_cwd.sfd_ino = 1;		//init at root
	sd_cwd.sfd_name[0] = '/';
	sd_cwd.sfd_name[1] = '\0';
}

void sfs_umount() {

	if( sd_cwd.sfd_ino !=  SFS_NOINO )
	{
		//umount
		disk_close();
		printf("%s, unmounted\n", spb.sp_volname);
		bzero(&spb, sizeof(struct sfs_super));
		sd_cwd.sfd_ino = SFS_NOINO;
	}
}

int find_emptyBlock()
{
	int i, j, k;
	char bitmap[SFS_BLOCKSIZE];
	int map_loc = SFS_MAP_LOCATION;

	for (i = 0; i < SFS_BITBLOCKS(spb.sp_nblocks); i++)  
	{
		disk_read(bitmap, map_loc);
		for (j = 0; j < SFS_BLOCKSIZE; j++) // 
		{
			if (bitmap[j] != (char)255) 
			{
				for (k = 0; k < 8; k++)
				{
					if (BIT_CHECK(bitmap[j], k) == 0)
					{
						BIT_SET(bitmap[j], k);
						disk_write(bitmap, map_loc);
						return ((i * SFS_BLOCKBITS) + (j * 8) + k );
					}
				}
			}	
		}
		map_loc++;
	}
	return (-1); //full
}

void sfs_touch(const char* path)
{
 	int i, j;
	struct sfs_inode cwd_inode;
	struct sfs_dir dir_entry[SFS_DENTRYPERBLOCK];
	int newbie_ino;

	disk_read(&cwd_inode, sd_cwd.sfd_ino);

	for (i = 0; i < SFS_NDIRECT; i++)
	{
		if (cwd_inode.sfi_direct[i] == 0) break;
		disk_read( dir_entry, cwd_inode.sfi_direct[i]);
		for (j = 0; j < SFS_DENTRYPERBLOCK; j++)
		{
			if (strcmp(path, dir_entry[j].sfd_name) == 0)
			{
				error_message("touch", path, -6);  //already in
				return;
			}
		}
	}
	for (i = 0; i < SFS_NDIRECT; i++)
	{
		if (cwd_inode.sfi_direct[i] == 0) break;
		disk_read( dir_entry, cwd_inode.sfi_direct[i]);
		for (j = 0; j < SFS_DENTRYPERBLOCK; j++)
		{
			if (dir_entry[j].sfd_ino == 0) //new block
			{
				newbie_ino = find_emptyBlock();
				if (newbie_ino == -1)
				{
					error_message("touch", path, -4); // disk full
					return;
				}
				dir_entry[j].sfd_ino = newbie_ino;
				strncpy( dir_entry[j].sfd_name, path, SFS_NAMELEN );
				disk_write(dir_entry, cwd_inode.sfi_direct[i]);        //update new directory entry 

				cwd_inode.sfi_size += sizeof(struct sfs_dir);
				disk_write( &cwd_inode, sd_cwd.sfd_ino );              //update cwd

				struct sfs_inode newbie;
				bzero(&newbie,SFS_BLOCKSIZE); 
				newbie.sfi_size = 0;
				newbie.sfi_type = SFS_TYPE_FILE;
				disk_write( &newbie, newbie_ino);           //update new inode
				return;
			}	
		}
	}
	error_message("touch", path, -3);  //directory full
}

void sfs_cd(const char* path)
{
	int i, j;
	struct sfs_inode cwd_inode;
	struct sfs_dir dir_entry[SFS_DENTRYPERBLOCK];

	if (!path)
	{
		sd_cwd.sfd_ino = 1;		//init at root
		sd_cwd.sfd_name[0] = '/';
		sd_cwd.sfd_name[1] = '\0';
		return;
	}

	disk_read( &cwd_inode, sd_cwd.sfd_ino);

	for (i = 0; i < SFS_NDIRECT; i++)
	{
		if (cwd_inode.sfi_direct[i] == 0) break;
		disk_read( dir_entry, cwd_inode.sfi_direct[i]);
		for (j = 0; j < SFS_DENTRYPERBLOCK; j++)
		{
			if (strcmp(path, dir_entry[j].sfd_name) == 0)
			{
				disk_read( &cwd_inode, dir_entry[j].sfd_ino);
				if (cwd_inode.sfi_type == SFS_TYPE_FILE)
				{
					error_message("cd", path, -2);  //not a directory
					return;
				}
				sd_cwd = dir_entry[j];
				return;
			}
		}
	}
	error_message("cd",path,-1);  // no such file or directory
	return;
}

void sfs_ls(const char* path)
{
	int i, j;

	struct sfs_inode cwd_inode;
	struct sfs_inode inode;
	struct sfs_dir dir_entry[SFS_DENTRYPERBLOCK];

	disk_read( &cwd_inode, sd_cwd.sfd_ino);

	if(path)
	{
		for (i = 0; i < SFS_NDIRECT; i++)
		{
			if (cwd_inode.sfi_direct[i] == 0) break;
			disk_read( dir_entry, cwd_inode.sfi_direct[i]);
			for (j = 0; j < SFS_DENTRYPERBLOCK; j++)
			{
				if (strcmp(path, dir_entry[j].sfd_name) == 0)
				{
					disk_read( &cwd_inode, dir_entry[j].sfd_ino);
					if (cwd_inode.sfi_type == SFS_TYPE_FILE)
					{
						printf("%s\n", dir_entry[j].sfd_name);
						return;
					}
					goto ls;
				}
			}
		}
		error_message("ls",path,-1);  // no such file or directory
		return;
	}
	
ls :
	for (i = 0; i < SFS_NDIRECT; i++)
	{	
		if (cwd_inode.sfi_direct[i] == 0) break;
		disk_read( dir_entry, cwd_inode.sfi_direct[i]);
		for(j=0; j < SFS_DENTRYPERBLOCK; j++) 
		{
			disk_read(&inode,dir_entry[j].sfd_ino);
			if (inode.sfi_type == SFS_TYPE_DIR)
				printf("%s/\t", dir_entry[j].sfd_name);
			else if (inode.sfi_type == SFS_TYPE_FILE) 
				printf("%s\t", dir_entry[j].sfd_name);
		}
	}
	printf("\n");
}

void sfs_mkdir(const char* org_path) 
{
	int i, j;
	struct sfs_inode cwd_inode;
	struct sfs_dir dir_entry[SFS_DENTRYPERBLOCK];
	int newbie_ino, newbie_data_ino, new_direct;

	disk_read(&cwd_inode, sd_cwd.sfd_ino);

	for (i = 0; i < SFS_NDIRECT; i++)
	{
		if (cwd_inode.sfi_direct[i] == 0) break;
		disk_read( dir_entry, cwd_inode.sfi_direct[i]);
		for (j = 0; j < SFS_DENTRYPERBLOCK; j++)
		{
			if (strcmp(org_path, dir_entry[j].sfd_name) == 0)
			{
				error_message("mkdir", org_path, -6);  //already in
				return;
			}
		}
	}
	for (i = 0; i < SFS_NDIRECT; i++)
	{
		if (cwd_inode.sfi_direct[i] == 0)
		{
			new_direct = find_emptyBlock();
			if (new_direct == -1)
				{
					error_message("mkdir", org_path, -4); // disk full
					return;
				}
			cwd_inode.sfi_direct[i] = new_direct;
			bzero(dir_entry, SFS_DENTRYPERBLOCK * sizeof(struct sfs_dir));
			disk_write(&cwd_inode,sd_cwd.sfd_ino);
			disk_write(dir_entry, cwd_inode.sfi_direct[i]);

		}
		disk_read(dir_entry, cwd_inode.sfi_direct[i]);
		for (j = 0; j < SFS_DENTRYPERBLOCK; j++)
		{
			if (dir_entry[j].sfd_ino == 0) //new block
			{
				newbie_ino = find_emptyBlock();
				if (newbie_ino == -1)
				{
					error_message("mkdir", org_path, -4); // disk full
					return;
				}
				dir_entry[j].sfd_ino = newbie_ino;
				strncpy( dir_entry[j].sfd_name, org_path, SFS_NAMELEN );
				disk_write(dir_entry, cwd_inode.sfi_direct[i]);        //update cwd directory entry 

				cwd_inode.sfi_size += sizeof(struct sfs_dir);
				disk_write( &cwd_inode, sd_cwd.sfd_ino );              //update cwd

				struct sfs_inode newbie;
				bzero(&newbie, SFS_BLOCKSIZE); 
				newbie.sfi_size = sizeof(struct sfs_dir) * 2;
				newbie.sfi_type = SFS_TYPE_DIR;
				
				newbie_data_ino = find_emptyBlock();
				if (newbie_data_ino == -1)
				{
					error_message("mkdir", org_path, -4); // disk full
					return;
				}
				newbie.sfi_direct[0] = newbie_data_ino;
				disk_write( &newbie, newbie_ino);              //update new inode
				
				struct sfs_dir newbie_entry[SFS_DENTRYPERBLOCK];
				bzero(newbie_entry,SFS_DENTRYPERBLOCK * sizeof(struct sfs_dir));
				newbie_entry[0].sfd_ino = newbie_ino;
				strncpy(newbie_entry[0].sfd_name, ".", SFS_NAMELEN);
				newbie_entry[1].sfd_ino = sd_cwd.sfd_ino;
				strncpy(newbie_entry[1].sfd_name, "..", SFS_NAMELEN);
				disk_write(newbie_entry, newbie.sfi_direct[0]);    //update new inode's directory entry
				return;
			}	
		}
	}
	error_message("mkdir", org_path, -3);  //directory full
}

void sfs_rmdir(const char* org_path) 
{
	int i, j, k;
	struct sfs_inode cwd_inode;
	struct sfs_inode rm_inode;
	struct sfs_dir dir_entry[SFS_DENTRYPERBLOCK];
	char bitmap[SFS_BLOCKSIZE];
	int bitmap_block;

	if (strcmp(org_path, "..") == 0)
	{
		error_message("rmdir", org_path, -7);  //not empty
		return;
	}
	if (strcmp(org_path, ".") == 0)
	{
		error_message("rmdir", org_path, -8);  //invalid
		return;
	}

	disk_read( &cwd_inode, sd_cwd.sfd_ino);

	for (i = 0; i < SFS_NDIRECT; i++)
	{
		if (cwd_inode.sfi_direct[i] == 0) break;
		disk_read( dir_entry, cwd_inode.sfi_direct[i]);
		for (j = 0; j < SFS_DENTRYPERBLOCK; j++)
		{
			if (strcmp(org_path, dir_entry[j].sfd_name) == 0)
			{
				disk_read( &rm_inode, dir_entry[j].sfd_ino);
				if (rm_inode.sfi_type == SFS_TYPE_FILE)
				{
					error_message("rmdir", org_path, -2);  //not a directory
					return;
				}
				if (rm_inode.sfi_size != 2 * sizeof(struct sfs_dir))
				{
					error_message("rmdir", org_path, -7);  //not empty
					return;
				}
				for (k = 0; k < SFS_NDIRECT; k++)
				{
					if (rm_inode.sfi_direct[k] != 0)
					{
						bitmap_block = rm_inode.sfi_direct[k] / SFS_BLOCKBITS;
						disk_read(bitmap, SFS_MAP_LOCATION + bitmap_block);
						BIT_CLEAR(bitmap[rm_inode.sfi_direct[k] / 8], rm_inode.sfi_direct[k] % 8);
						disk_write(bitmap, SFS_MAP_LOCATION + bitmap_block);
					}
				}

				bitmap_block = dir_entry[j].sfd_ino / SFS_BLOCKBITS;
				disk_read(bitmap, SFS_MAP_LOCATION + bitmap_block);
				BIT_CLEAR(bitmap[dir_entry[j].sfd_ino / 8], dir_entry[j].sfd_ino % 8);
				disk_write(bitmap, SFS_MAP_LOCATION + bitmap_block);
				
				cwd_inode.sfi_size -= sizeof(struct sfs_dir);
				disk_write(&cwd_inode, sd_cwd.sfd_ino);              //update cwd

				bzero(&dir_entry[j], sizeof(struct sfs_dir)); 
				disk_write( dir_entry, cwd_inode.sfi_direct[i]);   
			
				return;
			}
		}
	}
	error_message("rmdir",org_path,-1);  // no such file or directory
	return;
}

void sfs_mv(const char* src_name, const char* dst_name) 
{
	int i, j;
	int i_, j_;
	int flag = 0;
	struct sfs_inode cwd_inode;
	struct sfs_dir dir_entry[SFS_DENTRYPERBLOCK];

	if (!strcmp(".", src_name) || !strcmp("..", src_name))
	{
		error_message("mv", src_name, -8);   //invalid
		return;
	}
	if (!strcmp(".", dst_name) || !strcmp("..", dst_name))
	{
		error_message("mv", dst_name, -8);   //invalid
		return;
	}

	disk_read( &cwd_inode, sd_cwd.sfd_ino);

	for (i = 0; i < SFS_NDIRECT; i++)
	{
		if (cwd_inode.sfi_direct[i] == 0) break;
		disk_read( dir_entry, cwd_inode.sfi_direct[i]);
		for (j = 0; j < SFS_DENTRYPERBLOCK; j++)
		{	
			if (strcmp(dst_name, dir_entry[j].sfd_name) == 0)
			{
				error_message("mv", dst_name, -6);
				return;
			}
			if (flag == 0 && strcmp(src_name, dir_entry[j].sfd_name) == 0)
			{
				i_ = i;
				j_ = j;
				flag = 1;
			}
		}
	}
	if (flag == 0)
	{
		error_message("mv", src_name, -1);  // no such file or directory
		return;
	}
	disk_read(dir_entry, cwd_inode.sfi_direct[i_]);
	strcpy(dir_entry[j_].sfd_name, dst_name);
	disk_write(dir_entry, cwd_inode.sfi_direct[i_]);
}

void sfs_rm(const char* path) 
{
	int i, j, k;
	struct sfs_inode cwd_inode;
	struct sfs_inode rm_inode;
	struct sfs_dir dir_entry[SFS_DENTRYPERBLOCK];
	char bitmap[SFS_BLOCKSIZE];
	int bitmap_block;

	disk_read( &cwd_inode, sd_cwd.sfd_ino);

	for (i = 0; i < SFS_NDIRECT; i++)
	{
		if (cwd_inode.sfi_direct[i] == 0) break;
		disk_read( dir_entry, cwd_inode.sfi_direct[i]);
		for (j = 0; j < SFS_DENTRYPERBLOCK; j++)
		{
			if (strcmp(path, dir_entry[j].sfd_name) == 0)
			{
				disk_read( &rm_inode, dir_entry[j].sfd_ino);
				if (rm_inode.sfi_type == SFS_TYPE_DIR)
				{
					error_message("rm", path, -9);  //is a directory
					return;
				}
				for (k = 0; k < SFS_NDIRECT; k++)
				{
					if (rm_inode.sfi_direct[k] != 0)
					{
						bitmap_block = rm_inode.sfi_direct[k] / SFS_BLOCKBITS;
						disk_read(bitmap, SFS_MAP_LOCATION + bitmap_block);
						BIT_CLEAR(bitmap[rm_inode.sfi_direct[k] / 8], rm_inode.sfi_direct[k] % 8);
						disk_write(bitmap, SFS_MAP_LOCATION + bitmap_block);
					}
				}

				bitmap_block = dir_entry[j].sfd_ino / SFS_BLOCKBITS;
				disk_read(bitmap, SFS_MAP_LOCATION + bitmap_block);
				BIT_CLEAR(bitmap[dir_entry[j].sfd_ino / 8], dir_entry[j].sfd_ino % 8);
				disk_write(bitmap, SFS_MAP_LOCATION + bitmap_block);
				
				cwd_inode.sfi_size -= sizeof(struct sfs_dir);
				disk_write(&cwd_inode, sd_cwd.sfd_ino);              //update cwd

				bzero(&dir_entry[j], sizeof(struct sfs_dir)); 
				disk_write( dir_entry, cwd_inode.sfi_direct[i]);   
			
				return;
			}
		}
	}
	error_message("rm",path,-1);  // no such file or directory
	return;
}

void sfs_cpin(const char* local_path, const char* path) 
{
	printf("Not Implemented\n");
}

void sfs_cpout(const char* local_path, const char* path) 
{
	printf("Not Implemented\n");
}

void dump_inode(struct sfs_inode inode) {
	int i;
	struct sfs_dir dir_entry[SFS_DENTRYPERBLOCK];

	printf("size %d type %d direct ", inode.sfi_size, inode.sfi_type);
	for(i=0; i < SFS_NDIRECT; i++) {
		printf(" %d ", inode.sfi_direct[i]);
	}
	printf(" indirect %d",inode.sfi_indirect);
	printf("\n");

	if (inode.sfi_type == SFS_TYPE_DIR) {
		for(i=0; i < SFS_NDIRECT; i++) {
			if (inode.sfi_direct[i] == 0) break;
			disk_read(dir_entry, inode.sfi_direct[i]);
			dump_directory(dir_entry);
		}
	}

}

void dump_directory(struct sfs_dir dir_entry[]) {
	int i;
	struct sfs_inode inode;
	for(i=0; i < SFS_DENTRYPERBLOCK;i++) {
		printf("%d %s\n",dir_entry[i].sfd_ino, dir_entry[i].sfd_name);
		disk_read(&inode,dir_entry[i].sfd_ino);
		if (inode.sfi_type == SFS_TYPE_FILE) {
			printf("\t");
			dump_inode(inode);
		}
	}
}

void sfs_dump() {
	// dump the current directory structure
	struct sfs_inode c_inode;

	disk_read(&c_inode, sd_cwd.sfd_ino);
	printf("cwd inode %d name %s\n",sd_cwd.sfd_ino,sd_cwd.sfd_name);
	dump_inode(c_inode);
	printf("\n");

}
