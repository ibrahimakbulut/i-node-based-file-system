#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <time.h>

using namespace std;

struct super_block{


	int number_of_inodes;

     //in KB
	int size_of_a_block;
	int inode_blocks_position_starts;
	int data_blocks_position_starts;

	//in Bytes
	int size_of_inode;

	int block_index_of_root;
};

	
struct i_node{

	char mode;
	int size=-1;		// mode= d mean directory , mode = f mean just file

	struct tm time;

	int block_pointers[13]={-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};


};

struct directory{
	short i_node_number;

	char filename[14];
};

FILE* create_super_block(FILE *fp,struct super_block super_block_of_file_system){


	fseek(fp,0,SEEK_SET);

	fwrite(&super_block_of_file_system, sizeof(struct super_block) , 1 , fp );

   return fp;

}

 FILE* create_root_dir(FILE *fp,int index_of_root_block,int begin_of_inodes,int block_size){


 	time_t tm=time(NULL);
 	

 	struct i_node root_inode;

 	root_inode.mode='d';
 	root_inode.size=block_size*1024;
 	struct tm *timepointer=localtime(&tm);
 	root_inode.time=*timepointer;
 	root_inode.block_pointers[0]=index_of_root_block;
 	root_inode.block_pointers[1]=-1;  // it says my block pointers end here

 	fseek(fp,1024*begin_of_inodes*block_size,SEEK_SET);

 	fwrite(&root_inode, sizeof(struct i_node) , 1 , fp );


 	struct directory root_directory;

 	root_directory.i_node_number=1;
 	memset(root_directory.filename,'\0',sizeof(root_directory.filename));

 	strcpy(root_directory.filename,".");


 	fseek(fp,1024*(index_of_root_block-1)*block_size,SEEK_SET);

 	fwrite(&root_directory,sizeof(struct directory),1,fp);


	root_directory.i_node_number=-1;
 	memset(root_directory.filename,'\0',sizeof(root_directory.filename));

 	fwrite(&root_directory,sizeof(struct directory),1,fp);

 	return fp;

}  

FILE* create_free_space_management(FILE *fp,int block_size,int number_of_total_inodes,int *blocks_for_space_management,int *datablocks_begins, int *inodeblocks_begins, int *root_directory_block){

   
   int block_size_as_bytes=block_size*1024;

   int number_of_blocks_total= 1024/ block_size;

   fseek(fp,block_size_as_bytes,SEEK_SET);


   char dump0='0';
   fwrite(&dump0 , sizeof(char) , 1 , fp );


	char is_avaible_inode='1';

	for(int i=1; i< number_of_total_inodes;++i)
   	fwrite(&is_avaible_inode , sizeof(char) , 1 , fp );


   double block_used_to_keep_i_nodes_double= (number_of_total_inodes*sizeof(struct i_node) ) / (double)block_size_as_bytes ;

   int block_used_to_keep_i_nodes= number_of_total_inodes*sizeof(struct i_node)  / block_size_as_bytes ;

   int block_used_to_keep_i_nodes_exactly;

   if((double)block_used_to_keep_i_nodes==block_used_to_keep_i_nodes_double){

   		block_used_to_keep_i_nodes_exactly=block_used_to_keep_i_nodes;

   }
   else{
   		block_used_to_keep_i_nodes_exactly=block_used_to_keep_i_nodes+1;
   }



   int b=(number_of_blocks_total+number_of_total_inodes)%block_size_as_bytes;

   int blocks_used_to_keep_free_space_management;

   if(b==0)

   	blocks_used_to_keep_free_space_management=(number_of_blocks_total+number_of_total_inodes)/block_size_as_bytes;
   else
   	blocks_used_to_keep_free_space_management=(number_of_blocks_total+number_of_total_inodes)/block_size_as_bytes + 1;


   	int total_number_of_blocks_for_filesystem=blocks_used_to_keep_free_space_management+block_used_to_keep_i_nodes_exactly+1 ;

   	int g=sizeof(struct i_node);

   	// total_number_of_blocks_for_filesystem is also index of where data blocks begin. 

   	*datablocks_begins=total_number_of_blocks_for_filesystem-1;


   	char is_avaible_block='0';

   	for(int i=0;i<total_number_of_blocks_for_filesystem+1;++i){         //root falan
   		fwrite(&is_avaible_block , sizeof(char) , 1 , fp );
   	}


   	//index of root directory block
   	*root_directory_block=total_number_of_blocks_for_filesystem+1;

   	is_avaible_block='1';

   	//where 256 is that filesystem can have maksimum block = total_file_system_size/block_size

   	for(int i=total_number_of_blocks_for_filesystem+1;i<number_of_blocks_total;++i){
   		fwrite(&is_avaible_block , sizeof(char) , 1 , fp );
   	}


   	// if total size of  bit maps of free i-nodes and free-blocs is not enough to fill blocks of free space management 
   	char dump='0';
   	for(int i=number_of_blocks_total+number_of_total_inodes;i<blocks_used_to_keep_free_space_management*block_size_as_bytes;++i){
   		fwrite(&dump , sizeof(char) , 1 , fp );
   	}

   	*blocks_for_space_management= blocks_used_to_keep_free_space_management;

   	//where superblock+ free space management is begin of i-nodes
   	*inodeblocks_begins= blocks_used_to_keep_free_space_management+1;

   	return fp;
}


// fill filesystem file as 1 MB

FILE* fill_file_system(FILE* fp){

	char initial_array[] = "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";

    for(int i=0; i< 1048;++i)
   		fwrite(initial_array , sizeof(char) , 1000 , fp );

   	char dump='0';

   	for(int i=0; i< 576;++i)
   		fwrite(&dump , sizeof(char) , 1 , fp );
   	return fp;
}

int is_power_of_two(int num){

	if(num==1)
		return 1;

	if(num%2!=0)
		return -1;

	while(num!=0){

		num=num/2;
		return is_power_of_two(num);
	}

	return 1;
}


int main(int argc,char** argv){


	if(argc!=4){	// user entered missin parameter

		printf("Missing parameter to makeFileSystem\n");
		return -1;
	}

	int size_of_a_block=atoi(argv[1]);
	int number_of_inodes_in_filesystem=atoi(argv[2]);

	if(size_of_a_block<1){
		printf("Blocks size in KB must be greater or equal 1 \n");
		return -1;
	}

	if(is_power_of_two(size_of_a_block)==-1){
		printf("Blocks size must be power of 2\n");
		return -1;
	}

   	struct super_block super_block_of_file_system;

   	struct directory root_directory;

   	FILE *fp;

   	fp = fopen(argv[3] , "wb" );

   	fp=fill_file_system(fp);

   	int blocks_used_space_management;

   	int i_nodes_begins;

   	int data_blocks_begins;

   	int root_directory_block_index;
   	fp=create_free_space_management(fp,size_of_a_block,number_of_inodes_in_filesystem,&blocks_used_space_management,&data_blocks_begins,&i_nodes_begins,&root_directory_block_index);


   	super_block_of_file_system.number_of_inodes=number_of_inodes_in_filesystem;
	super_block_of_file_system.size_of_a_block=size_of_a_block;

	super_block_of_file_system.inode_blocks_position_starts=i_nodes_begins;
	super_block_of_file_system.data_blocks_position_starts=data_blocks_begins;

	super_block_of_file_system.size_of_inode=sizeof(struct i_node);

	super_block_of_file_system.block_index_of_root=root_directory_block_index;

	fp=create_super_block(fp,super_block_of_file_system);

	fseek(fp,8,SEEK_SET);

   	fwrite(&i_nodes_begins,sizeof(int),1,fp);

   	fwrite(&data_blocks_begins,sizeof(int),1,fp);  

   	fp=create_root_dir(fp,root_directory_block_index,i_nodes_begins,size_of_a_block); 
   	fclose(fp);

	return 0;
}
