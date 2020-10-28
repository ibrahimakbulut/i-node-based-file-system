#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <time.h>
#include <vector> 


struct i_node{

	char mode;
	int size=-1;

	struct tm time;

	int block_pointers[13]={-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};


};

struct directory{
	short i_node_number;

	char filename[14];
};

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

using namespace std;

vector<struct directory> read_directory_block(FILE *fp,int block_index,int block_size_KB,int data_blocks_start_position){

	vector<struct directory> directory_block_entry;

	int block_size_in_bytes=1024*block_size_KB;

	fseek(fp,block_size_in_bytes*(block_index-1),SEEK_SET);

	int total_size=0;

	struct directory temp;

	fread(&temp,sizeof(struct directory),1,fp);

	total_size+=sizeof(struct directory);

	while(total_size!=block_size_in_bytes && temp.i_node_number!=-1){

		directory_block_entry.push_back(temp);
		fread(&temp,sizeof(struct directory),1,fp);
		total_size+=sizeof(struct directory);
	}
	return directory_block_entry;
}
int search_file_in_block(struct i_node *inodes,char *filename,int block_index,int *mode,FILE* fp,int size_of_block_in_KB,int data_blocks_start_position){

	*mode=-1; // not clear yet that file is a directory or just file.

	vector<struct directory> block_entiries= read_directory_block(fp,block_index,size_of_block_in_KB,data_blocks_start_position);

	int a=block_entiries.size();

	for(int i=0;i<a;++i){

		if(strcmp(block_entiries.at(i).filename,filename)==0){

			if( inodes[block_entiries.at(i).i_node_number-1].mode=='d'){
				*mode=0;
				return block_entiries.at(i).i_node_number;
			}
			else{
				*mode=1;
				return block_entiries.at(i).i_node_number;
			}
		}
	}
	return -1;
}

int search_file_in_directory(struct i_node current_inode,struct i_node *inodes,char *filename,int *mode,FILE *fp,int size_of_block_in_KB,int data_blocks_start_position){

	int result= search_file_in_block(inodes,filename,current_inode.block_pointers[0],mode,fp,size_of_block_in_KB,data_blocks_start_position);

	if(result!=-1){

		return result;
	}

	for(int i=1;i<10 && current_inode.block_pointers[i]!=-1 ;++i){
		result=search_file_in_block(inodes,filename,current_inode.block_pointers[i],mode,fp,size_of_block_in_KB,data_blocks_start_position);
		
		if(result!=-1)
			return result;
	}
	return -1;
}

int find_empty_index_in_directory_block(FILE *fp, int block_index,int size_of_block_in_KB){

	fseek(fp, (block_index-1)*1024*size_of_block_in_KB,SEEK_SET);

	int number_of_directory_entries=(1024*size_of_block_in_KB/sizeof(struct directory));

	int total_size= sizeof(struct directory);

	struct directory *directory_block= (struct directory*)malloc(sizeof(struct directory)*number_of_directory_entries);

	fread(directory_block,sizeof(struct directory),number_of_directory_entries,fp);

	for(int i=0;i<number_of_directory_entries;++i){

		if(directory_block[i].i_node_number==-1){
			free(directory_block);
			return i;
		}
	}
	free(directory_block);
	return -1;
}

int find_place_to_locate_block_index(struct i_node current_inode,FILE *fp,int size_of_block_in_KB,int *block_entry_index,int *inode_pointer){

	int directory_index;

	for(int i=0;i<10;++i){

		if(current_inode.block_pointers[i]==-1){
			*inode_pointer=i;
			return -2;
		}
		directory_index=find_empty_index_in_directory_block(fp,current_inode.block_pointers[i],size_of_block_in_KB);
		if(directory_index!=-1){
			*block_entry_index=directory_index;
			*inode_pointer=i;
			return current_inode.block_pointers[i];
		}
	}
	return -1;
}

int mkdir_command(char *directory_name,FILE *fp,int begining_inode_blocks,int begining_of_data_blocks, int size_block_in_KB,int total_number_of_inodes){

	int total_number_of_blocks= 1024/size_block_in_KB;

	fseek(fp,1024*size_block_in_KB,SEEK_SET);

	char *free_inode_list=(char *)malloc(sizeof(char)*total_number_of_inodes);
	char *free_block_list=(char *)malloc(sizeof(char)*total_number_of_blocks);

	fread(free_inode_list,sizeof(char),total_number_of_inodes,fp);

	fread(free_block_list,sizeof(char),total_number_of_blocks,fp);

	int size_of_path=strlen(directory_name);

	char *backup_directory_name=(char *)malloc(sizeof(char)*(size_of_path+1));

	strcpy(backup_directory_name,directory_name);

	char **path_as_list;

	int names_in_path=0;

	 char * token = strtok(directory_name, "/");
	 char * current=NULL;

   while( token != NULL ) {
      current=token;
      token = strtok(NULL, "/");
      names_in_path+=1;
   }

   path_as_list=(char **)malloc(sizeof(char *)*names_in_path);

	 token = strtok(backup_directory_name, "/");
	 int i=0;
   while( token != NULL ) {
      path_as_list[i]=token;
      token = strtok(NULL, "/");
      ++i;
   }

   fseek(fp,1024*begining_inode_blocks*size_block_in_KB,SEEK_SET);

   struct i_node *inodes=(struct i_node*)malloc(sizeof(struct i_node)*total_number_of_inodes);

   fread(inodes,sizeof(struct i_node),total_number_of_inodes,fp);

   struct i_node current_inode;

   current_inode=inodes[0];  // read 1 th i node. root i_node is in the begining of the inode blocks
   int exist_file=0;

   for(int i=0;i<names_in_path-1;++i){

   	int mode=-1;

   	exist_file=search_file_in_directory(current_inode,inodes,path_as_list[i],&mode,fp,size_block_in_KB,begining_of_data_blocks);

   	if(exist_file==-1){
   		free(free_inode_list);
   		free(free_block_list);
   		free(inodes);
   		free(backup_directory_name);
   		printf("No such a file or directory\n");
   		return 0;
   	}
   	else{

   		if(mode==1){
   			free(free_inode_list);
   			free(free_block_list);
   			free(inodes);
   			free(backup_directory_name);
   			printf("This is not directory\n");
   			return 0;
   		}
   	}

   	current_inode=inodes[exist_file-1];

   }

   int current_inode_number=exist_file;

   int block_index_locate_directory;
   int block_entry_index;
   int dumpmode;

   exist_file=search_file_in_directory(current_inode,inodes,path_as_list[names_in_path-1],&dumpmode,fp,size_block_in_KB,begining_of_data_blocks);

   if(exist_file!=-1){
   	free(free_inode_list);
   	free(free_block_list);
   	free(inodes);
   	free(backup_directory_name);
   	printf("can not create directory : files exist\n");

   	return 0;
   }

   int block_pointer_index;

   block_index_locate_directory=find_place_to_locate_block_index(current_inode,fp,size_block_in_KB,&block_entry_index,&block_pointer_index);

   		int new_directory_inode_number;
   		int new_directory_block_number;

   		i=0;

   		while(free_inode_list[i]=='0' && i<total_number_of_inodes){
   			++i;
   		}

   		if(total_number_of_inodes==i){

   			free(free_inode_list);
   			free(free_block_list);
   			free(inodes);
   			free(backup_directory_name);   			
   			printf("can not create directory: there is no space for inodes\n");
   			return 0;
   		}
   		new_directory_inode_number=i+1;
   		free_inode_list[i]='0';

   		i=0;

   		while(free_block_list[i]=='0' && i<total_number_of_blocks){
   			++i;
   		}

   		if(total_number_of_blocks==i){
   			free(free_inode_list);
   			free(free_block_list);
   			free(inodes);
   			free(backup_directory_name);
   			printf("can not create directory: there is no space for blocks\n");
   			return 0;
   		}
   		free_block_list[i]='0';

   		new_directory_block_number=i+1;

   		struct directory temp_directory;
   		temp_directory.i_node_number=new_directory_inode_number;
   		strcpy(temp_directory.filename,path_as_list[names_in_path-1]);

   		struct directory temp_directory2;
   		temp_directory2.i_node_number=-1;

   		time_t tm= time(NULL);
   		struct i_node temp_inode;
   		temp_inode.mode='d';
 		temp_inode.size=size_block_in_KB*1024;
 		struct tm *timepointer=localtime(&tm);
 		temp_inode.time=*timepointer;
 		temp_inode.block_pointers[0]=new_directory_block_number;
 		temp_inode.block_pointers[1]=-1;

 		struct directory temp_directory3;
 		temp_directory3.i_node_number=new_directory_inode_number;
 		strcpy(temp_directory3.filename,".");

	if(block_index_locate_directory==-1){

   		free(free_inode_list);
   		free(free_block_list);
   		free(inodes);
   		free(backup_directory_name);		
		printf("You can not create this much file under this directory\n");
		return 0;
	}
	else if(block_index_locate_directory==-2){

		i=0;
		while(free_block_list[i]=='0' && i<total_number_of_blocks){      //yeni block ayır i nodun gostermesi icin
   			++i;
   		}

   		if(total_number_of_blocks==i){

   			free(free_inode_list);
   			free(free_block_list);
   			free(inodes);
   			free(backup_directory_name);   			
   			printf("can not create directory: there is no space for new blocks\n");
   			return 0;
   		}

   		free_block_list[i]='0';

   		int new_block_pointer_to_block=i+1;

   		current_inode.time=*timepointer;

   		current_inode.block_pointers[block_pointer_index]=new_block_pointer_to_block;

		if(current_inode_number==0)
			fseek(fp,begining_inode_blocks*1024*size_block_in_KB,SEEK_SET);
		else
			fseek(fp,begining_inode_blocks*1024*size_block_in_KB+ sizeof(struct i_node)*(current_inode_number-1),SEEK_SET);
		      
   		fwrite(&current_inode,sizeof(struct i_node),1,fp);

   		fseek(fp,(new_block_pointer_to_block-1)*1024*size_block_in_KB ,SEEK_SET);
   		struct directory temp3_directory;

   		temp3_directory.i_node_number=current_inode_number;
   		strcpy(temp3_directory.filename,".");

   		fwrite(&temp3_directory,sizeof(struct directory),1,fp);

   		fwrite(&temp_directory,sizeof(struct directory),1,fp);
   		fwrite(&temp_directory2,sizeof(struct directory),1,fp);

   		fseek(fp,(new_directory_block_number-1)*1024*size_block_in_KB,SEEK_SET);  // yeni data block ayrildi yeni klasor icin
 		fwrite(&temp_directory3,sizeof(struct directory),1,fp);
 		temp_directory3.i_node_number=-1;
 		fwrite(&temp_directory3,sizeof(struct directory),1,fp);

 		fseek(fp,begining_inode_blocks*1024*size_block_in_KB+sizeof(struct i_node)*(new_directory_inode_number-1),SEEK_SET);
 		fwrite(&temp_inode,sizeof(struct i_node),1,fp);

 		fseek(fp,1024*size_block_in_KB,SEEK_SET);
		fwrite(free_inode_list,sizeof(char),total_number_of_inodes,fp);
		fwrite(free_block_list,sizeof(char),total_number_of_blocks,fp);

   		free(free_inode_list);
   		free(free_block_list);
   		free(inodes);
   		free(backup_directory_name);

 		return 0;
	}

	else{

		current_inode.time=*timepointer;

		if(current_inode_number==0)
			fseek(fp,begining_inode_blocks*1024*size_block_in_KB,SEEK_SET);
		else
			fseek(fp,begining_inode_blocks*1024*size_block_in_KB+ sizeof(struct i_node)*(current_inode_number-1),SEEK_SET);
		      
		//change modification time of root directory
   		fwrite(&current_inode,sizeof(struct i_node),1,fp);

		fseek(fp,(block_index_locate_directory-1)*1024*size_block_in_KB+(block_entry_index)*sizeof(struct directory),SEEK_SET);
		fwrite(&temp_directory,sizeof(struct directory),1,fp);

		if((1024*size_block_in_KB)/sizeof(struct directory) !=block_entry_index){

			temp_directory.i_node_number=-1;
			fwrite(&temp_directory,sizeof(struct directory),1,fp);
		}

   		fseek(fp,(new_directory_block_number-1)*1024*size_block_in_KB,SEEK_SET);  // yeni data block ayrildi yeni klasor icin
 		fwrite(&temp_directory3,sizeof(struct directory),1,fp);
 		temp_directory3.i_node_number=-1;
 		fwrite(&temp_directory3,sizeof(struct directory),1,fp);

 		fseek(fp,begining_inode_blocks*1024*size_block_in_KB+sizeof(struct i_node)*(new_directory_inode_number-1),SEEK_SET);  //burada
 		fwrite(&temp_inode,sizeof(struct i_node),1,fp);

		fseek(fp,1024*size_block_in_KB,SEEK_SET);
		fwrite(free_inode_list,sizeof(char),total_number_of_inodes,fp);
		fwrite(free_block_list,sizeof(char),total_number_of_blocks,fp);

   		free(free_inode_list);
   		free(free_block_list);
   		free(inodes);
   		free(backup_directory_name);

		return 0;
	}

   return 0;
}

int list(char *file_name,FILE *fp,int begining_inode_blocks,int begining_of_data_blocks, int size_block_in_KB,int total_number_of_inodes){

	int total_number_of_blocks= 1024/size_block_in_KB;

	int size_of_path=strlen(file_name);

	char *backup_directory_name=(char *)malloc(sizeof(char)*(size_of_path+1));
	strcpy(backup_directory_name,file_name);

	char **path_as_list;

	int names_in_path=0;
	char * token = strtok(file_name, "/");

	char * current=NULL;
  	while( token != NULL ) {
     	current=token;
     	token = strtok(NULL, "/");
      	names_in_path+=1;
   	}

   	path_as_list=(char **)malloc(sizeof(char *)*names_in_path);

	token = strtok(backup_directory_name, "/");

	int i=0;
   	while( token != NULL ) {
    	path_as_list[i]=token;
      	token = strtok(NULL, "/");
      	++i;
   	}

   	fseek(fp,1024*begining_inode_blocks*size_block_in_KB,SEEK_SET);

   	struct i_node *inodes=(struct i_node*)malloc(sizeof(struct i_node)*total_number_of_inodes);

   	fread(inodes,sizeof(struct i_node),total_number_of_inodes,fp);

   	struct i_node current_inode;

   	current_inode=inodes[0];  // read 1 th i node. root i_node is in the begining of the inode blocks
   	int exist_file=0;

	if(names_in_path==0){

   	for(int i=0;i<10 && current_inode.block_pointers[i]!=-1;++i){

		vector<struct directory> directory_entiries= read_directory_block(fp,current_inode.block_pointers[i],size_block_in_KB,begining_of_data_blocks);

		int number_of_directory_entries= directory_entiries.size();

		int j=1;

		while(j<number_of_directory_entries){

			struct i_node current_inode2=inodes[directory_entiries[j].i_node_number-1];
			printf("%d---------%d-%02d-%02d %02d:%02d:%02d---------%s\n",current_inode2.size, current_inode2.time.tm_year + 1900, current_inode2.time.tm_mon + 1, current_inode2.time.tm_mday, current_inode2.time.tm_hour, current_inode2.time.tm_min, current_inode2.time.tm_sec,directory_entiries[j].filename); 
			++j;
		}

   	}
   	free(inodes);
   	free(backup_directory_name);

   	return 0;
	}

   	for(int i=0;i<names_in_path-1;++i){

   		int mode=-1;
   		exist_file=search_file_in_directory(current_inode,inodes,path_as_list[i],&mode,fp,size_block_in_KB,begining_of_data_blocks);

   		if(exist_file==-1){

   			free(inodes);
   			free(backup_directory_name);
   			printf("No such a file or directory\n");
   			return 0;
   		}
   		else{

   			if(mode==1){

   				free(inodes);
   				free(backup_directory_name);
   				printf("This is not directory\n");

   				return 0;
   			}
   		}

   	current_inode=inodes[exist_file-1];
   	}

   	int dumpmode;

    exist_file=search_file_in_directory(current_inode,inodes,path_as_list[names_in_path-1],&dumpmode,fp,size_block_in_KB,begining_of_data_blocks);

    if(exist_file==-1){
   		free(inodes);
   		free(backup_directory_name);    	
   		printf("No such a file or directory\n");
   		return 0;
   	}

   	if(dumpmode==1){

   		free(inodes);
   		free(backup_directory_name);   		
   		printf("Can not list: File is not directory\n");
   		return 0;
   	}
   	current_inode=inodes[exist_file-1];

   	for(int i=0;i<10 && current_inode.block_pointers[i]!=-1;++i){

		vector<struct directory> directory_entiries= read_directory_block(fp,current_inode.block_pointers[i],size_block_in_KB,begining_of_data_blocks);

		int number_of_directory_entries= directory_entiries.size();

		int j=1;

		while(j<number_of_directory_entries){

			struct i_node current_inode2=inodes[directory_entiries[j].i_node_number-1];
			printf("%d---------%d-%02d-%02d %02d:%02d:%02d---------%s\n",current_inode2.size, current_inode2.time.tm_year + 1900, current_inode2.time.tm_mon + 1, current_inode2.time.tm_mday, current_inode2.time.tm_hour, current_inode2.time.tm_min, current_inode2.time.tm_sec,directory_entiries[j].filename); 
			++j;
		}
   	}

   	free(inodes);
   	free(backup_directory_name);
   return 0;
}

int rmdir(char *file_name,FILE *fp,int begining_inode_blocks,int begining_of_data_blocks, int size_block_in_KB,int total_number_of_inodes){

	int total_number_of_blocks= 1024/size_block_in_KB;

	fseek(fp,1024*size_block_in_KB,SEEK_SET);

	char *free_inode_list=(char *)malloc(sizeof(char)*total_number_of_inodes);
	char *free_block_list=(char *)malloc(sizeof(char)*total_number_of_blocks);

	fread(free_inode_list,sizeof(char),total_number_of_inodes,fp);

	fread(free_block_list,sizeof(char),total_number_of_blocks,fp);

	int size_of_path=strlen(file_name);

	char *backup_directory_name=(char *)malloc(sizeof(char)*(size_of_path+1));
	strcpy(backup_directory_name,file_name);

	char **path_as_list;
	int names_in_path=0;

	char * token = strtok(file_name, "/");
	char * current=NULL;
 
  	while( token != NULL ) {
     	current=token;
     	token = strtok(NULL, "/");
      	names_in_path+=1;
   	}
   	path_as_list=(char **)malloc(sizeof(char *)*names_in_path);

	token = strtok(backup_directory_name, "/");
	int i=0;
   	while( token != NULL ) {
    	path_as_list[i]=token;
      	token = strtok(NULL, "/");
      	++i;
   	}

   	fseek(fp,1024*begining_inode_blocks*size_block_in_KB,SEEK_SET);

   	struct i_node *inodes=(struct i_node*)malloc(sizeof(struct i_node)*total_number_of_inodes);

   	fread(inodes,sizeof(struct i_node),total_number_of_inodes,fp);

   	struct i_node current_inode;

   	current_inode=inodes[0];  // read 1 th i node. root i_node is in the begining of the inode blocks
   	int exist_file=0;

   	for(int i=0;i<names_in_path-1;++i){

   		int mode=-1;
   		exist_file=search_file_in_directory(current_inode,inodes,path_as_list[i],&mode,fp,size_block_in_KB,begining_of_data_blocks);

   		if(exist_file==-1){

   			free(free_inode_list);
   			free(free_block_list);
   			free(inodes);
   			free(backup_directory_name);
   			printf("No such a file or directory\n");
   			return 0;
   		}
   		else{

   			if(mode==1){
   				free(free_inode_list);
   				free(free_block_list);
   				free(inodes);
   				free(backup_directory_name);   				
   				printf("This is not directory\n");

   				return 0;
   			}
   		}

   	current_inode=inodes[exist_file-1];
   	}
	int current_inode_number=exist_file;

   	int dumpmode;

    exist_file=search_file_in_directory(current_inode,inodes,path_as_list[names_in_path-1],&dumpmode,fp,size_block_in_KB,begining_of_data_blocks);

    if(exist_file==-1){
   		free(free_inode_list);
   		free(free_block_list);
   		free(inodes);
   		free(backup_directory_name);    	
   		printf("No such a file or directory\n");
   		return 0;
   	}
   	if(dumpmode==1){
   		free(free_inode_list);
   		free(free_block_list);
   		free(inodes);
   		free(backup_directory_name);   		
   		printf("Can not remove: File is not directory\n");
   		return 0;
   	}

   	time_t tm= time(NULL);
 	struct tm *timepointer=localtime(&tm);


   	for(int i=0;i<10 && current_inode.block_pointers[i]!=-1;++i){

		vector<struct directory> directory_entiries= read_directory_block(fp,current_inode.block_pointers[i],size_block_in_KB,begining_of_data_blocks);

		int number_of_directory_entries= directory_entiries.size();

		int j=1;
		while(j<number_of_directory_entries){

			if(strcmp(directory_entiries.at(j).filename,path_as_list[names_in_path-1])==0){

				current_inode.time=*timepointer;
			
				vector<struct directory> temp_directory_entiries= read_directory_block(fp,inodes[directory_entiries.at(j).i_node_number-1].block_pointers[0],size_block_in_KB,begining_of_data_blocks);

				if(temp_directory_entiries.size()==1){
					free_inode_list[directory_entiries.at(j).i_node_number-1]='1';
					free_block_list[inodes[directory_entiries.at(j).i_node_number-1].block_pointers[0]-1]='1';

					if(directory_entiries.size()==2){
					
					if(i!=0){
					free_block_list[current_inode.block_pointers[i]-1]='1';
					current_inode.block_pointers[i]=-1;
					// it must be anyway
					if(current_inode_number==0)    //if parent is root directory
						fseek(fp,begining_inode_blocks*1024*size_block_in_KB,SEEK_SET);
					else	
						fseek(fp,begining_inode_blocks*1024*size_block_in_KB+(current_inode_number-1)*sizeof(struct i_node),SEEK_SET);
					fwrite(&current_inode,sizeof(struct i_node),1,fp);
					fseek(fp,1024*size_block_in_KB,SEEK_SET);

					fwrite(free_inode_list,sizeof(char),total_number_of_inodes,fp);
					fwrite(free_block_list,sizeof(char),total_number_of_blocks,fp);

					}
					else{

					struct directory temp_directory;
					temp_directory.i_node_number=-1;      
					//change modification time
					if(current_inode_number==0)    //if parent is root directory
						fseek(fp,begining_inode_blocks*1024*size_block_in_KB,SEEK_SET);
					else	
						fseek(fp,begining_inode_blocks*1024*size_block_in_KB+(current_inode_number-1)*sizeof(struct i_node),SEEK_SET);
					
					fwrite(&current_inode,sizeof(struct i_node),1,fp);

					fseek(fp,(current_inode.block_pointers[i]-1)*1024*size_block_in_KB+j*sizeof(struct directory),SEEK_SET);

					fwrite(&temp_directory,sizeof(struct directory),1,fp);

					fseek(fp,1024*size_block_in_KB,SEEK_SET);

					fwrite(free_inode_list,sizeof(char),total_number_of_inodes,fp);
					fwrite(free_block_list,sizeof(char),total_number_of_blocks,fp);
					}

					free(free_inode_list);
   					free(free_block_list);
   					free(inodes);
   					free(backup_directory_name);
					return 0;

					}
					else{

						if(current_inode_number==0)    //if parent is root directory
							fseek(fp,begining_inode_blocks*1024*size_block_in_KB,SEEK_SET);
						else	
							fseek(fp,begining_inode_blocks*1024*size_block_in_KB+(current_inode_number-1)*sizeof(struct i_node),SEEK_SET);
					
						fwrite(&current_inode,sizeof(struct i_node),1,fp);

						struct directory temp_directory;
						temp_directory.i_node_number=-1;

						fseek(fp,(current_inode.block_pointers[i]-1)*1024*size_block_in_KB+j*sizeof(struct directory),SEEK_SET);

						for(int i=j+1;i<directory_entiries.size();++i){

							fwrite(&directory_entiries.at(i),sizeof(struct directory),1,fp);
						}

						fwrite(&temp_directory,sizeof(struct directory),1,fp);


						fseek(fp,1024*size_block_in_KB,SEEK_SET);

						fwrite(free_inode_list,sizeof(char),total_number_of_inodes,fp);
						fwrite(free_block_list,sizeof(char),total_number_of_blocks,fp);

						free(free_inode_list);
   						free(free_block_list);
   						free(inodes);
   						free(backup_directory_name);
						return 0;
					}
				}
				else{

   					free(free_inode_list);
   					free(free_block_list);
   					free(inodes);
   					free(backup_directory_name);					
					printf("Can not remove: directory is not empty\n");
					return 0;
				}
			}
			++j;
		}
   	}

   	free(free_inode_list);
   	free(free_block_list);
   	free(inodes);
   	free(backup_directory_name);
   	return 0;
}

int del(char *file_name,FILE *fp,int begining_inode_blocks,int begining_of_data_blocks, int size_block_in_KB,int total_number_of_inodes){

	int total_number_of_blocks= 1024/size_block_in_KB;

	fseek(fp,1024*size_block_in_KB,SEEK_SET);

	char *free_inode_list=(char *)malloc(sizeof(char)*total_number_of_inodes);
	char *free_block_list=(char *)malloc(sizeof(char)*total_number_of_blocks);

	fread(free_inode_list,sizeof(char),total_number_of_inodes,fp);

	fread(free_block_list,sizeof(char),total_number_of_blocks,fp);

	int size_of_path=strlen(file_name);

	char *backup_directory_name=(char *)malloc(sizeof(char)*(size_of_path+1));

	strcpy(backup_directory_name,file_name);

	char **path_as_list;

	int names_in_path=0;
	char * token = strtok(file_name, "/");
	char * current=NULL;

  	while( token != NULL ) {
     	current=token;
     	token = strtok(NULL, "/");
      	names_in_path+=1;
   	}

   	path_as_list=(char **)malloc(sizeof(char *)*names_in_path);
	token = strtok(backup_directory_name, "/");

	int i=0;
   	while( token != NULL ) {
    	path_as_list[i]=token;
      	token = strtok(NULL, "/");
      	++i;
   	}

   	fseek(fp,1024*begining_inode_blocks*size_block_in_KB,SEEK_SET);

   	struct i_node *inodes=(struct i_node*)malloc(sizeof(struct i_node)*total_number_of_inodes);

   	fread(inodes,sizeof(struct i_node),total_number_of_inodes,fp);

   	struct i_node current_inode;

   	current_inode=inodes[0];  // read 1 th i node. root i_node is in the begining of the inode blocks
   	int exist_file=0;

   	for(int i=0;i<names_in_path-1;++i){

   		int mode=-1;
   		exist_file=search_file_in_directory(current_inode,inodes,path_as_list[i],&mode,fp,size_block_in_KB,begining_of_data_blocks);

   		if(exist_file==-1){
   			free(free_inode_list);
   			free(free_block_list);
   			free(inodes);
   			free(backup_directory_name);
   			printf("No such a file or directory\n");
   			return 0;
   		}
   		else{

   			if(mode==1){
   				free(free_inode_list);
   				free(free_block_list);
   				free(inodes);
   				free(backup_directory_name);
   				printf("This is not directory\n");

   				return 0;
   			}
   		}

   	current_inode=inodes[exist_file-1];
   	}

	int current_inode_number=exist_file;

   	int dumpmode;

    exist_file=search_file_in_directory(current_inode,inodes,path_as_list[names_in_path-1],&dumpmode,fp,size_block_in_KB,begining_of_data_blocks);

    if(exist_file==-1){
    	free(free_inode_list);
   		free(free_block_list);
   		free(inodes);
   		free(backup_directory_name);
   		printf("No such a file or directory\n");
   		return 0;
   	}

   	if(dumpmode==0){
   		free(free_inode_list);
   		free(free_block_list);
   		free(inodes);
   		free(backup_directory_name);
   		printf("Can not delete: File is a directory\n");
   		return 0;
   	}

   	for(int i=0;i<10 && current_inode.block_pointers[i]!=-1;++i){

		vector<struct directory> directory_entiries= read_directory_block(fp,current_inode.block_pointers[i],size_block_in_KB,begining_of_data_blocks);

		int number_of_directory_entries= directory_entiries.size();

		int j=1;

		while(j<number_of_directory_entries){

			if(strcmp(directory_entiries.at(j).filename,path_as_list[names_in_path-1])==0){

  				time_t tm= time(NULL);
 				struct tm *timepointer=localtime(&tm);
 				current_inode.time=*timepointer;

				free_inode_list[directory_entiries.at(j).i_node_number-1]='1';

				int t=0;
				for(;t<10 && inodes[directory_entiries.at(j).i_node_number-1].block_pointers[t]!=-1;++t){

				free_block_list[inodes[directory_entiries.at(j).i_node_number-1].block_pointers[t]-1]='1';				

				}

				if(t==10 && inodes[directory_entiries.at(j).i_node_number-1].block_pointers[t]!=-1){

					free_block_list[inodes[directory_entiries.at(j).i_node_number-1].block_pointers[t]-1]='1';	

					fseek(fp,(inodes[directory_entiries.at(j).i_node_number-1].block_pointers[t]-1)*1024*size_block_in_KB,SEEK_SET);
					int temp_int;
					fread(&temp_int,sizeof(int),1,fp);

					while(temp_int!=-1){
						free_block_list[temp_int-1]='1';
						fread(&temp_int,sizeof(int),1,fp);
					}
				}				
			
				if(directory_entiries.size()==2){

					if(i!=0){

					free_block_list[current_inode.block_pointers[i]-1]='1';
					current_inode.block_pointers[i]=-1;

					if(current_inode_number==0)    //if parent is root directory
						fseek(fp,begining_inode_blocks*1024*size_block_in_KB,SEEK_SET);
					else	
						fseek(fp,begining_inode_blocks*1024*size_block_in_KB+(current_inode_number-1)*sizeof(struct i_node),SEEK_SET);
					
					fwrite(&current_inode,sizeof(struct i_node),1,fp);
					fseek(fp,1024*size_block_in_KB,SEEK_SET);

					fwrite(free_inode_list,sizeof(char),total_number_of_inodes,fp);
					fwrite(free_block_list,sizeof(char),total_number_of_blocks,fp);

					}
					else{

					if(current_inode_number==0)    //if parent is root directory
						fseek(fp,begining_inode_blocks*1024*size_block_in_KB,SEEK_SET);
					else	
						fseek(fp,begining_inode_blocks*1024*size_block_in_KB+(current_inode_number-1)*sizeof(struct i_node),SEEK_SET);
					
					fwrite(&current_inode,sizeof(struct i_node),1,fp);

					struct directory temp_directory;
					temp_directory.i_node_number=-1;

					fseek(fp,(current_inode.block_pointers[i]-1)*1024*size_block_in_KB+j*sizeof(struct directory),SEEK_SET);

					fwrite(&temp_directory,sizeof(struct directory),1,fp);

					fseek(fp,1024*size_block_in_KB,SEEK_SET);

					fwrite(free_inode_list,sizeof(char),total_number_of_inodes,fp);
					fwrite(free_block_list,sizeof(char),total_number_of_blocks,fp);
					}

   					free(free_inode_list);
   					free(free_block_list);
   					free(inodes);
   					free(backup_directory_name);					
					return 0;
				}
				else{

						if(current_inode_number==0)    //if parent is root directory
							fseek(fp,begining_inode_blocks*1024*size_block_in_KB,SEEK_SET);
						else	
							fseek(fp,begining_inode_blocks*1024*size_block_in_KB+(current_inode_number-1)*sizeof(struct i_node),SEEK_SET);
					
						fwrite(&current_inode,sizeof(struct i_node),1,fp);

						struct directory temp_directory;
						temp_directory.i_node_number=-1;

						fseek(fp,(current_inode.block_pointers[i]-1)*1024*size_block_in_KB+j*sizeof(struct directory),SEEK_SET);

						for(int i=j+1;i<directory_entiries.size();++i){

							fwrite(&directory_entiries.at(i),sizeof(struct directory),1,fp);
						}
						fwrite(&temp_directory,sizeof(struct directory),1,fp);

						fseek(fp,1024*size_block_in_KB,SEEK_SET);
						fwrite(free_inode_list,sizeof(char),total_number_of_inodes,fp);
						fwrite(free_block_list,sizeof(char),total_number_of_blocks,fp);

						free(free_inode_list);
   						free(free_block_list);
   						free(inodes);
   						free(backup_directory_name);
						return 0;
				}
			}
			++j;
		}
   	}
}


int write(char *file_name,char *linux_file,FILE *fp,int begining_inode_blocks,int begining_of_data_blocks, int size_block_in_KB,int total_number_of_inodes){

	FILE *fp2=fopen(linux_file,"rb+");

	if(fp2==NULL){
		printf("There is no file %s",linux_file);
		return 0;
	}

	int total_number_of_blocks= 1024/size_block_in_KB;

	fseek(fp,1024*size_block_in_KB,SEEK_SET);

	char *free_inode_list=(char *)malloc(sizeof(char)*total_number_of_inodes);
	char *free_block_list=(char *)malloc(sizeof(char)*total_number_of_blocks);

	fread(free_inode_list,sizeof(char),total_number_of_inodes,fp);

	fread(free_block_list,sizeof(char),total_number_of_blocks,fp);

	int size_of_path=strlen(file_name);

	char *backup_directory_name=(char *)malloc(sizeof(char)*(size_of_path+1));
	strcpy(backup_directory_name,file_name);

	char **path_as_list;
	int names_in_path=0;

	char * token = strtok(file_name, "/");
	char * current=NULL;
  	while( token != NULL ) {
     	current=token;
     	token = strtok(NULL, "/");
      	names_in_path+=1;
   	}
   	path_as_list=(char **)malloc(sizeof(char *)*names_in_path);

	token = strtok(backup_directory_name, "/");

	int i=0;
   	while( token != NULL ) {
    	path_as_list[i]=token;
      	token = strtok(NULL, "/");
      	++i;
   	}

   	fseek(fp,1024*begining_inode_blocks*size_block_in_KB,SEEK_SET);

   	struct i_node *inodes=(struct i_node*)malloc(sizeof(struct i_node)*total_number_of_inodes);

   	fread(inodes,sizeof(struct i_node),total_number_of_inodes,fp);

   	struct i_node current_inode;

   	current_inode=inodes[0];  // read 1 th i node. root i_node is in the begining of the inode blocks
   	int exist_file=0;

   	for(int i=0;i<names_in_path-1;++i){

   		int mode=-1;
   		exist_file=search_file_in_directory(current_inode,inodes,path_as_list[i],&mode,fp,size_block_in_KB,begining_of_data_blocks);

   		if(exist_file==-1){
   			free(free_inode_list);
   			free(free_block_list);
   			free(inodes);
   			free(backup_directory_name);
   			printf("No such a file or directory\n");
   			fclose(fp2);
   			return 0;
   		}
   		else{

   			if(mode==1){
   				free(free_inode_list);
   				free(free_block_list);
   				free(inodes);
   				free(backup_directory_name);   				
   				printf("This is not directory\n");
   				fclose(fp2);
   				return 0;
   			}
   		}
   	current_inode=inodes[exist_file-1];
   	}

   int current_inode_number=exist_file;
   int block_index_locate_file;
   int block_entry_index;
   int dumpmode;

   exist_file=search_file_in_directory(current_inode,inodes,path_as_list[names_in_path-1],&dumpmode,fp,size_block_in_KB,begining_of_data_blocks);

   if(exist_file!=-1){

   	if(dumpmode==1){

   		free(free_inode_list);
   		free(free_block_list);
   		free(inodes);
   		free(backup_directory_name);

   		fclose(fp2);

   		del(file_name,fp,begining_inode_blocks,begining_of_data_blocks,size_block_in_KB,total_number_of_inodes);
   		write(file_name,linux_file,fp,begining_inode_blocks,begining_of_data_blocks,size_block_in_KB,total_number_of_inodes);

   		return 0;
   	}
   	free(free_inode_list);
   	free(free_block_list);
   	free(inodes);
   	free(backup_directory_name);

   	printf("can not create file : there is a directory with same file name\n");
   	fclose(fp2);
   	return 0;
   }
   int block_pointer_index;

   block_index_locate_file=find_place_to_locate_block_index(current_inode,fp,size_block_in_KB,&block_entry_index,&block_pointer_index);

   		int new_file_inode_number;
   		i=0;

   		while(free_inode_list[i]=='0' && i<total_number_of_inodes){
   			++i;
   		}
   		if(total_number_of_inodes==i){
   			free(free_inode_list);
   			free(free_block_list);
   			free(inodes);
   			free(backup_directory_name);

   			printf("can not create file: there is no space for inodes\n");
   			fclose(fp2);
   			return 0;
   		}
   		new_file_inode_number=i+1;
   		free_inode_list[i]='0';

   		i=0;
   		int avaible_block_number=0;
   		while(i<total_number_of_blocks){

   			if(free_block_list[i]=='1')
   				avaible_block_number++;
   			++i;
   		}
   		int size_of_linux_file;
		fseek(fp2, 0, SEEK_END); 
		size_of_linux_file = ftell(fp2); 
		fseek(fp2, 0, SEEK_SET);

		int needed_blocks_number_for_file;

		if(size_of_linux_file%(size_block_in_KB*1024)==0){
			needed_blocks_number_for_file=size_of_linux_file/(size_block_in_KB*1024);
		}
		else{
			needed_blocks_number_for_file=size_of_linux_file/(size_block_in_KB*1024)+1;
		}
		int total_blocks_in_ten=needed_blocks_number_for_file;

		if(needed_blocks_number_for_file>10)
			needed_blocks_number_for_file+=1;

		if(block_index_locate_file==-2)
			needed_blocks_number_for_file+=1;

		if(needed_blocks_number_for_file>(10+((size_block_in_KB*1024)/4)) ){
   			free(free_inode_list);
   			free(free_block_list);
   			free(inodes);
   			free(backup_directory_name);

			printf("These file system cant address this too much data\n");
			fclose(fp2);
			return 0;
		}

		if(avaible_block_number<needed_blocks_number_for_file){
   			free(free_inode_list);
   			free(free_block_list);
   			free(inodes);
   			free(backup_directory_name);			
			printf("There is no space for coping linux file to system\n");
			fclose(fp2);
			return 0;
		}

   		time_t tm= time(NULL);
   		struct i_node temp_inode;
   		temp_inode.mode='f';
 		temp_inode.size=size_of_linux_file;
 		struct tm *timepointer=localtime(&tm);
 		temp_inode.time=*timepointer;
 
		int readed_size=0;
		int a;

		i=0;
		char *temp=(char*)malloc(sizeof(char)*(size_block_in_KB*1024));
		for(;i<10 && i<total_blocks_in_ten;++i){

   			a=0;
   			while(free_block_list[a]=='0' && a<total_number_of_blocks){
   				++a;
   			}
   			free_block_list[a]='0';
   			temp_inode.block_pointers[i]=a+1;

			readed_size=fread(temp,sizeof(char),size_block_in_KB*1024,fp2);

			fseek(fp,a*1024*size_block_in_KB,SEEK_SET);
			fwrite(temp,sizeof(char),readed_size,fp);
		}

		if(i<total_blocks_in_ten){
			a=0;
   			while(free_block_list[a]=='0' && a<total_number_of_blocks){
   				++a;
   			}
   			free_block_list[a]='0';
   			int single_indirect_pointer=a;

   			temp_inode.block_pointers[10]=single_indirect_pointer+1;

   			int c=0;

   			for(int b=i;b<total_blocks_in_ten;++b){

   				a=0;
   				while(free_block_list[a]=='0' && a<total_number_of_blocks){
   					++a;
   				}
   				free_block_list[a]='0';
   				int block_index_new=a+1;
   				fseek(fp,single_indirect_pointer*1024*size_block_in_KB+c*sizeof(int),SEEK_SET);
   				fwrite(&block_index_new,sizeof(int),1,fp);

				readed_size=fread(temp,sizeof(char),size_block_in_KB*1024,fp2);

				fseek(fp,(block_index_new-1)*1024*size_block_in_KB,SEEK_SET);
				fwrite(temp,sizeof(char),readed_size,fp);
				c++;
   			}

   			if(c+1<((1024*size_block_in_KB)/4)){
   				int tempint=-1;
   				fseek(fp,single_indirect_pointer*1024*size_block_in_KB+c*sizeof(int),SEEK_SET);
   				fwrite(&tempint,sizeof(int),1,fp);
   			}

		}
		free(temp);

 		struct directory temp_directory;
 		temp_directory.i_node_number=new_file_inode_number;
 		struct directory temp_directory2;
 		temp_directory2.i_node_number=-1;
		if(names_in_path==1){
			strcpy(temp_directory.filename,path_as_list[0]);
		}
		else{
			strcpy(temp_directory.filename,path_as_list[names_in_path-1]);
		}

	if(block_index_locate_file==-1){
   		free(free_inode_list);
   		free(free_block_list);
   		free(inodes);
   		free(backup_directory_name);

		printf("You can not create this much file under this directory\n");
		fclose(fp2);
		return 0;
	}

	else if(block_index_locate_file==-2){

		i=0;
		while(free_block_list[i]=='0' && i<total_number_of_blocks){      //yeni block ayır i nodun gostermesi icin
   			++i;
   		}

   		if(total_number_of_blocks==i){

   			free(free_inode_list);
   			free(free_block_list);
   			free(inodes);
   			free(backup_directory_name);   			

   			printf("can not create directory: there is no space for new blocks\n");
   			fclose(fp2);
   			return 0;
   		}

   		free_block_list[i]='0';

   		int new_block_pointer_to_block=i+1;

   		current_inode.block_pointers[block_pointer_index]=new_block_pointer_to_block;

   		current_inode.time=*timepointer;

		if(current_inode_number==0)    //if parent is root directory
			fseek(fp,begining_inode_blocks*1024*size_block_in_KB,SEEK_SET);
		else	
			fseek(fp,begining_inode_blocks*1024*size_block_in_KB+(current_inode_number-1)*sizeof(struct i_node),SEEK_SET);
					
		fwrite(&current_inode,sizeof(struct i_node),1,fp);

   		fseek(fp,(new_block_pointer_to_block-1)*1024*size_block_in_KB ,SEEK_SET);
   		struct directory temp3_directory;

   		temp3_directory.i_node_number=current_inode_number;
   		strcpy(temp3_directory.filename,".");

   		fwrite(&temp3_directory,sizeof(struct directory),1,fp);

   		fwrite(&temp_directory,sizeof(struct directory),1,fp);
   		fwrite(&temp_directory2,sizeof(struct directory),1,fp);

 		fseek(fp,begining_inode_blocks*1024*size_block_in_KB+sizeof(struct i_node)*(new_file_inode_number-1),SEEK_SET);
 		fwrite(&temp_inode,sizeof(struct i_node),1,fp);

 		fseek(fp,1024*size_block_in_KB,SEEK_SET);
		fwrite(free_inode_list,sizeof(char),total_number_of_inodes,fp);
		fwrite(free_block_list,sizeof(char),total_number_of_blocks,fp);

   	}
   	else{

   		current_inode.time=*timepointer;
		if(current_inode_number==0)    //if parent is root directory
			fseek(fp,begining_inode_blocks*1024*size_block_in_KB,SEEK_SET);
		else	
			fseek(fp,begining_inode_blocks*1024*size_block_in_KB+(current_inode_number-1)*sizeof(struct i_node),SEEK_SET);
					
		fwrite(&current_inode,sizeof(struct i_node),1,fp);

		fseek(fp,(block_index_locate_file-1)*1024*size_block_in_KB+(block_entry_index)*sizeof(struct directory),SEEK_SET);
		fwrite(&temp_directory,sizeof(struct directory),1,fp);

		if(((1024*size_block_in_KB)/sizeof(struct directory)) !=(block_entry_index+1)){

			temp_directory.i_node_number=-1;

			fwrite(&temp_directory,sizeof(struct directory),1,fp);
		}

 		fseek(fp,begining_inode_blocks*1024*size_block_in_KB+sizeof(struct i_node)*(new_file_inode_number-1),SEEK_SET);  //burada
 		fwrite(&temp_inode,sizeof(struct i_node),1,fp);

		fseek(fp,1024*size_block_in_KB,SEEK_SET);
		fwrite(free_inode_list,sizeof(char),total_number_of_inodes,fp);
		fwrite(free_block_list,sizeof(char),total_number_of_blocks,fp);
   	}

   	free(free_inode_list);
   	free(free_block_list);
   	free(inodes);
   	free(backup_directory_name);
   	fclose(fp2);

   	return 0;

}

int read(char *file_name,char *linux_file,FILE *fp,int begining_inode_blocks,int begining_of_data_blocks, int size_block_in_KB,int total_number_of_inodes){

	
	int total_number_of_blocks= 1024/size_block_in_KB;

	fseek(fp,1024*size_block_in_KB,SEEK_SET);

	char *free_inode_list=(char *)malloc(sizeof(char)*total_number_of_inodes);
	char *free_block_list=(char *)malloc(sizeof(char)*total_number_of_blocks);

	fread(free_inode_list,sizeof(char),total_number_of_inodes,fp);

	fread(free_block_list,sizeof(char),total_number_of_blocks,fp);

	int size_of_path=strlen(file_name);

	char *backup_directory_name=(char *)malloc(sizeof(char)*(size_of_path+1));

	strcpy(backup_directory_name,file_name);

	char **path_as_list;

	int names_in_path=0;

	char * token = strtok(file_name, "/");
	char * current=NULL;

  	while( token != NULL ) {
     	current=token;
     	token = strtok(NULL, "/");
      	names_in_path+=1;
   	}

   	path_as_list=(char **)malloc(sizeof(char *)*names_in_path);

	token = strtok(backup_directory_name, "/");

	int i=0;
   	while( token != NULL ) {
    	path_as_list[i]=token;
      	token = strtok(NULL, "/");
      	++i;
   	}
   	fseek(fp,1024*begining_inode_blocks*size_block_in_KB,SEEK_SET);

   	struct i_node *inodes=(struct i_node*)malloc(sizeof(struct i_node)*total_number_of_inodes);

   	fread(inodes,sizeof(struct i_node),total_number_of_inodes,fp);

   	struct i_node current_inode;

   	current_inode=inodes[0];  // read 1 th i node. root i_node is in the begining of the inode blocks
   	int exist_file=0;

   	for(int i=0;i<names_in_path-1;++i){

   		int mode=-1;
   		exist_file=search_file_in_directory(current_inode,inodes,path_as_list[i],&mode,fp,size_block_in_KB,begining_of_data_blocks);

   		if(exist_file==-1){

   			free(free_inode_list);
   			free(free_block_list);
   			free(inodes);
   			free(backup_directory_name);

   			printf("No such a file or directory\n");
   			return 0;
   		}
   		else{

   			if(mode==1){

   				free(free_inode_list);
   				free(free_block_list);
   				free(inodes);
   				free(backup_directory_name);   				
   				printf("This is not directory\n");
   				return 0;
   			}
   		}

   	current_inode=inodes[exist_file-1];
   	}

   int current_inode_number=exist_file;

   int block_index_locate_file;
   int block_entry_index;
   int dumpmode;

   exist_file=search_file_in_directory(current_inode,inodes,path_as_list[names_in_path-1],&dumpmode,fp,size_block_in_KB,begining_of_data_blocks);

   if(exist_file==-1){

   	free(free_inode_list);
   	free(free_block_list);
   	free(inodes);
   	free(backup_directory_name);   	
   	printf("There is no such a file in my filesystem\n");
   	return 0;
   }
   if(exist_file!=-1 && dumpmode==0){
   	
   	free(free_inode_list);
   	free(free_block_list);
   	free(inodes);
   	free(backup_directory_name);

   	printf("Can not read : file is a directory\n");
   	return 0;
   }

   FILE *fp2=fopen(linux_file,"wb+");

   struct i_node file_inode=inodes[exist_file-1];

   int readed_size=0;

   int file_content_size=file_inode.size;
   int file_content_size_in_block;

   file_content_size_in_block=file_content_size/(size_block_in_KB*1024);
   int file_content_size_in_remain=file_content_size-(file_content_size_in_block*size_block_in_KB*1024);

   int remainder_count=0;

   char *file_contents=(char*)malloc(sizeof(char)*file_inode.size);
   i=0;

   for(;i<10 && i<file_content_size_in_block; ++i){


   	fseek(fp,(file_inode.block_pointers[i]-1)*1024*size_block_in_KB,SEEK_SET);

   	fread(&file_contents[i*size_block_in_KB*1024],sizeof(char),(size_block_in_KB*1024),fp);
   }

   int a=i;
   int b=i;

   int d=0;
   for(int i=0;a<file_content_size_in_block;++a){

   		int block_index_single;

   		fseek(fp,(file_inode.block_pointers[10]-1)*1024*size_block_in_KB + i*sizeof(int),SEEK_SET);

   		fread(&block_index_single,sizeof(int),1,fp);

   		fseek(fp,(block_index_single-1)*1024*size_block_in_KB,SEEK_SET);

   		fread(&file_contents[a*1024*size_block_in_KB],sizeof(char),(size_block_in_KB*1024),fp);

   		++i;
   		++b;
   		d++;
   }

   if(file_content_size_in_remain!=0 && b<10){

   		fseek(fp,(file_inode.block_pointers[b]-1)*1024*size_block_in_KB,SEEK_SET);

   		for(int c=0;c<file_content_size_in_remain;++c){

   			fread(&file_contents[file_content_size_in_block*1024*size_block_in_KB+c*sizeof(char)],sizeof(char),1,fp);
   		}
   }

   else if(file_content_size_in_remain!=0 && b>=10){

   		int block_index_single;

   		fseek(fp,(file_inode.block_pointers[10]-1)*1024*size_block_in_KB + d*sizeof(int),SEEK_SET);
   		fread(&block_index_single,sizeof(int),1,fp);

   		fseek(fp,(block_index_single-1)*1024*size_block_in_KB,SEEK_SET);

   		for(int c=0;c<file_content_size_in_remain;++c){

   			fread(&file_contents[file_content_size_in_block*1024*size_block_in_KB+c*sizeof(char)],sizeof(char),1,fp);
   		}
   }

   	free(free_inode_list);
   	free(free_block_list);
   	free(inodes);
   	free(backup_directory_name);

   	fwrite(file_contents,sizeof(char),file_content_size,fp2);

   	free(file_contents);

   	fclose(fp2);

   return 0;
}

int check_path_name(char *path_name){

	int size_of_path=strlen(path_name);

	if(path_name[0]!='/'){
		printf("path must be start with / character\n");
		return -1;
	}
	else if(path_name[size_of_path-1]=='/' && size_of_path>1){
		printf("path name can't end with / character\n");
		return -1;
	}
	else{
		int wrong=1;
		for(int i=1;i<size_of_path;++i){

			if(path_name[i]=='/' && wrong==1){
				printf("There cant be 2 / character after each other\n");
				return -1;
			}
			else if(path_name[i]!='/'){
				wrong=0;
			}
			else if(path_name[i]=='/' && wrong==0){
				wrong=1;
			}
		}
	}
	return 0;
}

int dumpe2fs(FILE *fp){

	struct super_block super_block_of_file_system;
	fseek(fp,0,SEEK_SET);
	fread(&super_block_of_file_system,sizeof(struct super_block),1,fp);

	printf("SUPERBLOCK INFORMATIONS OF FILE SYSTEM >\n\n");
	printf("          Number Of I-nodes:              %d\n",super_block_of_file_system.number_of_inodes);
	printf("          Size Of Blocks (in Bytes):      %d\n",super_block_of_file_system.size_of_a_block*1024);
	printf("          Start Of I-nodes in Block:      %d\n",super_block_of_file_system.inode_blocks_position_starts+1);
	printf("          Start Of Data Blocks in Block:  %d\n",super_block_of_file_system.data_blocks_position_starts+2);
	printf("          Size Of I-node (in Bytes):      %d\n",super_block_of_file_system.size_of_inode);
	printf("          Root Directory First Block:     %d\n",super_block_of_file_system.block_index_of_root);
	printf("          Total Number Of Blocks:         %d\n\n\n",1024/super_block_of_file_system.size_of_a_block);

	printf("FREE I-NODES AND FREE BLOCKS >\n\n");

	fseek(fp,super_block_of_file_system.size_of_a_block*1024,SEEK_SET);

	char *free_inode_list=(char *)malloc(sizeof(char)*super_block_of_file_system.number_of_inodes);
	char *free_block_list=(char *)malloc(sizeof(char)*(1024/super_block_of_file_system.size_of_a_block));

	fread(free_inode_list,sizeof(char),super_block_of_file_system.number_of_inodes,fp);

	fread(free_block_list,sizeof(char),(1024/super_block_of_file_system.size_of_a_block),fp);

	int count_of_free_inodes=0;
	int count_of_free_blocks=0;
	int i=0;

	while(i<super_block_of_file_system.number_of_inodes){
		if(free_inode_list[i]=='1')
			++count_of_free_inodes;
		++i;
	}

	i=0;
	while(i<(1024/super_block_of_file_system.size_of_a_block)){
		if(free_block_list[i]=='1')
			++count_of_free_blocks;
		++i;
	}

	printf("          Number Of Free I-nodes:         %d\n",count_of_free_inodes);
	printf("          Number Of Free Blocks:          %d\n\n\n",count_of_free_blocks);

	printf("          Number Of I-nodes In Use:       %d\n",super_block_of_file_system.number_of_inodes-count_of_free_inodes);
	printf("          Number Of Blocks In Use:        %d\n\n\n",(1024/super_block_of_file_system.size_of_a_block)-count_of_free_blocks);

	printf("NUMBER OF FILES AND DIRECTORIES >\n\n");

	fseek(fp,1024*super_block_of_file_system.inode_blocks_position_starts*super_block_of_file_system.size_of_a_block,SEEK_SET);

   	struct i_node *inodes=(struct i_node*)malloc(sizeof(struct i_node)*super_block_of_file_system.number_of_inodes);

   	fread(inodes,sizeof(struct i_node),super_block_of_file_system.number_of_inodes,fp);

   	int count_of_files=0;
   	int count_of_directories=0;
   	i=0;

   	while(i<super_block_of_file_system.number_of_inodes){

   		if(free_inode_list[i]=='0'){

   			if(inodes[i].mode=='d')
   				count_of_directories+=1;
   			else
   				count_of_files+=1;
   		}
   		++i;
   	}

   	printf("          Number Of Files:                %d\n",count_of_files);
   	printf("          Number Of Directories:          %d\n\n\n",count_of_directories);

   	printf("I-NODES THAT ARE USING >\n\n");

   	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
   	printf("I-node 1:                     file_name: Root              file_mode: d\n");
   	printf("This I-node use these blocks:");

   	int h=0;

   	while(h<10 && inodes[0].block_pointers[h]!=-1){

   	printf("   %d",inodes[0].block_pointers[h]);
   					++h;
   	}
   	printf("\n");

   	i=1; 
   	while(i<super_block_of_file_system.number_of_inodes){

   		if(free_inode_list[i]=='0'){

   			int j=0;

   			int found=0;

   			while(j<super_block_of_file_system.number_of_inodes && found==0){

   				if(free_inode_list[j]=='0'){


   					if(inodes[j].mode=='d') {

   						int k=0;

   						vector<struct directory> entiries;
   						while(k<10 && inodes[j].block_pointers[k]!=-1 && found==0){

   							entiries=read_directory_block(fp,inodes[j].block_pointers[k],super_block_of_file_system.size_of_a_block,super_block_of_file_system.data_blocks_position_starts);

   							int a=entiries.size();

   							int m=1;

   							while(m<a){


   								if(entiries.at(m).i_node_number==(i+1)){
   									printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
   									printf("I-node %d:                     file_name: %s              file_mode: %c\n",i+1,entiries.at(m).filename,inodes[i].mode);
   									found=1;
   									break;

   								}

   								++m;
   							}
   							++k;
   						}

   					}

   				}
   				++j;

   			}

   			printf("This I-node use these blocks:");

   			if(inodes[i].mode=='d'){

   				int k=0;

   				while(k<10 && inodes[i].block_pointers[k]!=-1){

   					printf("   %d",inodes[i].block_pointers[k]);
   					++k;
   				}
   				printf("\n");
   			}
   			else{

   				int k=0;

   				while(k<10 && inodes[i].block_pointers[k]!=-1){

   					printf("   %d",inodes[i].block_pointers[k]);
   					++k;
   				}

   				if(k>=10 && inodes[i].block_pointers[10]!=-1){

   					printf("   %d",inodes[i].block_pointers[10]);

   					fseek(fp,super_block_of_file_system.size_of_a_block*1024*(inodes[i].block_pointers[10]-1),SEEK_SET);

   					int temp_int=0;

   					fread(&temp_int,sizeof(int),1,fp);

   					int int_count=0;
   					while(temp_int!=-1 && int_count< ((super_block_of_file_system.size_of_a_block*1024)/4) ){

   						printf("   %d",temp_int);
   						fread(&temp_int,sizeof(int),1,fp);
   					}
   				}

   				printf("\n");

   			}

   		}

   		++i;

   	}
   	free(free_inode_list);
   	free(free_block_list);
   	free(inodes);

	return 0;
}

int main(int argc, char **argv){

if(argc<3){

	printf("missing parameter to the program\n");
	return 0;
}
else{

	FILE *fp= fopen(argv[1],"rb+");

	if(fp==NULL){

		printf("There is no file named %s\n",argv[1]);
		return 0;
	}

	struct super_block super_block_of_file_system;
	fread(&super_block_of_file_system,sizeof(struct super_block),1,fp);

	if(strcmp(argv[2],"mkdir")==0){

		if(argc!=4){
			fclose(fp);
			printf("Missing parameter to mkdir\n");
			return 0;
		}

		if(check_path_name(argv[3])==-1){
			fclose(fp);
			return 0;
		}
		if(strlen(argv[3])==1){
			printf("There is no file name under /\n");
			fclose(fp);
			return 0;
		}

		mkdir_command(argv[3],fp,super_block_of_file_system.inode_blocks_position_starts,super_block_of_file_system.data_blocks_position_starts, super_block_of_file_system.size_of_a_block,super_block_of_file_system.number_of_inodes);
		fclose(fp);
		return 0;
	}
	else if(strcmp(argv[2],"write")==0){

		if(argc!=5){
			fclose(fp);
			printf("Missing parameter to write\n");
			return 0;
		}

		if(check_path_name(argv[3])==-1){
			fclose(fp);
			return 0;
		}

		if(strlen(argv[3])==1){
			printf("There is no file name under /\n");
			fclose(fp);
			return 0;
		}

		write(argv[3],argv[4],fp,super_block_of_file_system.inode_blocks_position_starts,super_block_of_file_system.data_blocks_position_starts, super_block_of_file_system.size_of_a_block,super_block_of_file_system.number_of_inodes);
		fclose(fp);
		return 0;
	}
	else if(strcmp(argv[2],"list")==0){

		if(argc!=4){
			fclose(fp);
			printf("Missing parameter to list\n");
			return 0;
		}
		if(check_path_name(argv[3])==-1){
			fclose(fp);
			return 0;
		}

		list(argv[3],fp,super_block_of_file_system.inode_blocks_position_starts,super_block_of_file_system.data_blocks_position_starts, super_block_of_file_system.size_of_a_block,super_block_of_file_system.number_of_inodes);
		fclose(fp);
		return 0;

	}


	else if(strcmp(argv[2],"rmdir")==0){

		if(argc!=4){
			fclose(fp);
			printf("Missing parameter to rmdir\n");
			return 0;
		}

		if(check_path_name(argv[3])==-1){
			fclose(fp);
			return 0;
		}
		if(strlen(argv[3])==1){
			printf("There is no file name under /\n");
			fclose(fp);
			return 0;
		}		

		rmdir(argv[3],fp,super_block_of_file_system.inode_blocks_position_starts,super_block_of_file_system.data_blocks_position_starts, super_block_of_file_system.size_of_a_block,super_block_of_file_system.number_of_inodes);
		fclose(fp);
		return 0;
	}

	else if(strcmp(argv[2],"del")==0){

		if(argc!=4){
			fclose(fp);
			printf("Missing parameter to del\n");
			return 0;
		}

		if(check_path_name(argv[3])==-1){
			fclose(fp);
			return 0;
		}
		if(strlen(argv[3])==1){
			printf("There is no file name under /\n");
			fclose(fp);
			return 0;
		}

		del(argv[3],fp,super_block_of_file_system.inode_blocks_position_starts,super_block_of_file_system.data_blocks_position_starts, super_block_of_file_system.size_of_a_block,super_block_of_file_system.number_of_inodes);
		fclose(fp);
		return 0;
	}

	else if(strcmp(argv[2],"read")==0){

		if(argc!=5){
			fclose(fp);
			printf("Missing parameter to read\n");
			return 0;
		}

		if(check_path_name(argv[3])==-1){
			fclose(fp);
			return 0;
		}

		if(strlen(argv[3])==1){
			printf("There is no file name under /\n");
			fclose(fp);
			return 0;
		}

		read(argv[3],argv[4],fp,super_block_of_file_system.inode_blocks_position_starts,super_block_of_file_system.data_blocks_position_starts, super_block_of_file_system.size_of_a_block,super_block_of_file_system.number_of_inodes);
		fclose(fp);
		return 0;
	}

	else if(strcmp(argv[2],"dumpe2fs")==0){
		if(argc!=3){
			fclose(fp);
			printf("Wrong parameter to dumpe2fs\n");
			return 0;
		}
		dumpe2fs(fp);
		fclose(fp);
		return 0;
	}

	else{
		printf("There is no operation like that\n");
		return 0;

	}

}




}
