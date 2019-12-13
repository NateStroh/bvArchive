#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

struct metaData{
  char fileName[256];
  char isDirectory;
  int numFiles;
  long numBytes;
  mode_t permissions;

} typedef metaData;

//globals
int archiveFD;
const int MAX_BUFFER_SIZE = 134217728; //128 MB is max buffer size

void writeFile(char* fileName, char* filePath, int fPerm){

  int fd = open(filePath, O_RDONLY);
  long numBytes = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  //setting up struct 
  metaData mData;
  strcpy(mData.fileName, fileName);
  mData.isDirectory = 0;
  mData.numFiles = 0;
  mData.numBytes = numBytes;
  mData.permissions = fPerm;
  printf("writing file: %s\n", mData.fileName);

  //write metaData
  write(archiveFD, &mData, sizeof(metaData));
  
  //printf("size of metadata %ld\n", sizeof(metaData));
  
  // write file 128 MB at a time, or entirely if <=128 MB
  if(numBytes > MAX_BUFFER_SIZE) {
    for(int i=0; i<numBytes; i+=MAX_BUFFER_SIZE) {
      char buff[MAX_BUFFER_SIZE];
      int size = read(fd, buff, MAX_BUFFER_SIZE);
      write(archiveFD, buff, size);
    }
  }
  else {
    char buff[numBytes];
    read(fd, buff, numBytes);
    write(archiveFD, buff, numBytes);
  }

  close(fd);
}

void directoryDelve(char* dir, char* fileName){
  //trying to open the directory they passed to us
  DIR* dr = opendir(dir);
  if(dr == NULL){
    fprintf(stderr, "Couldn't open that directory:%s\n", dir);
    exit(-1);
  }

  DIR* counter = opendir(dir);
  struct dirent *de;

  int numFiles = 0;
  while ((de = readdir(counter)) != NULL){ 
    //ignoring . .. in directories
    if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0){
      continue;
    }
    numFiles++;
  }

  //setting up metadata struct for the file
  metaData mData;
  strcpy(mData.fileName, fileName);
  mData.isDirectory = 1;
  mData.numFiles = numFiles;
  mData.numBytes = 0;
  mData.permissions = 0;//TODO: this might need to change, just setting it to defualt for now, all directories are probably fine to have the same hardcoded permissions anyways

  printf("writing directory: %s\n", mData.fileName);
  write(archiveFD, &mData, sizeof(metaData));

  //struct to store file info from lstat
  struct stat fileInfo; 

  //while there are more directorys
  while ((de = readdir(dr)) != NULL){ 
    //printf("%s\n", de->d_name); 

    int fpLen = strlen(dir);
    char fullPath[fpLen+strlen(de->d_name)+1];
    strcpy(fullPath, dir);
    strcat(fullPath, "/");
    strcat(fullPath, de->d_name);

    if(stat(fullPath, &fileInfo) == -1){
      perror("stat");
      continue;
    }

    //checking if it's a directory
    if(S_ISDIR(fileInfo.st_mode)){        
      //ignoring . .. in directories
      if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0){
        continue;
      }

      printf("%s\n", fullPath);

      //explore the directory
      directoryDelve(fullPath, de->d_name);  
    }
    //checking if it's a file
    else if(S_ISREG(fileInfo.st_mode)){
      printf("%s\n", fullPath);
      int filePerm = fileInfo.st_mode;
      writeFile(de->d_name, fullPath, filePerm); 
    }
  }
  closedir(dr);
}

int main(int argc, char** argv){
  //checking if they provided correct command line input
  if(argc != 3){
    fprintf(stderr, "Incorrect number of arguments provided.\n");
    return -1;
  }

  //setting up variables for desired directory to be archived and what they want it to be called
  char* directoryToArchive = argv[1]; 
  char* archiveFile = argv[2]; 

  //opening a new file
  archiveFD = open(archiveFile, O_RDWR|O_CREAT|O_TRUNC, 0644);

  //getting full path of file given to us - gets current directory and throws on a file
  char fullPath[4096];
  getcwd(fullPath, sizeof(fullPath));
  strcat(fullPath, "/");
  strcat(fullPath, directoryToArchive);

  //start recusrsive function with directory that is passed in to us
  directoryDelve(fullPath, directoryToArchive);

  close(archiveFD);

  return 0;
}
