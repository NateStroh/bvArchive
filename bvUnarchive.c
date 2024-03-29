#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

struct metaData{
  int fileNameLen;
  char isDirectory;
  int numFiles;
  long numBytes;
  mode_t permissions;

} typedef metaData;

//globals
int archiveFD;
const int MAX_BUFFER_SIZE = 134217728; //128 MB is max buffer size

void unpackDirectory(char* dir){
  metaData mData;
  read(archiveFD, &mData, sizeof(metaData));
  //printf("size of metadata: %ld\n", sizeof(metaData));

  //read filename
  char fName [mData.fileNameLen];
  read(archiveFD, &fName, sizeof(fName));
  //printf("size of filename %ld\n", sizeof(fName));
  

  printf("Filename: %s\n", fName);
  int fpLen = strlen(dir);
  char fullPath[fpLen+strlen(fName)+1];
  strcpy(fullPath, dir);
  strcat(fullPath, "/");
  strcat(fullPath, fName);
  

  if(mData.isDirectory){
    //it's a directory
    printf("directory: %s\n", fullPath);
    mkdir(fullPath, 0777);
    
    for(int i=0; i<mData.numFiles; i++){
      unpackDirectory(fullPath);
    }
  }
  else{
    //it's a file
    printf("file : %s\n", fullPath);
    int fd = open(fullPath, O_WRONLY|O_CREAT|O_TRUNC, mData.permissions);

    // if the file is larger than our maximum buffer, then write 10MB at a time
    if(mData.numBytes > MAX_BUFFER_SIZE){
      for(int i=0; i<mData.numBytes; i+=MAX_BUFFER_SIZE){
        // We may not have 10MB left in the given file... This means that we 
        //   run the risk of reading too much. Gotta throttle it with this if
        if (mData.numBytes-i < MAX_BUFFER_SIZE) {
          char buff[mData.numBytes-i];
          int size = read(archiveFD, buff, MAX_BUFFER_SIZE);
          write(fd, buff, size);
        }
        // this else executes if the size is still greater than MAX. It will read 10MB
        else {
          char buff[MAX_BUFFER_SIZE];
          int size = read(archiveFD, buff, MAX_BUFFER_SIZE);
          write(fd, buff, size);
        }
      }
    }
    else{
      char buff[mData.numBytes];
      read(archiveFD, buff, mData.numBytes);
      write(fd, buff, mData.numBytes);
    }

    close(fd);
  }
}

int main(int argc, char** argv){
  if(argc != 2){
    fprintf(stderr, "Incorrect number of arguments provided.\n");
    return -1;
  }

  char* archiveFile = argv[1]; 

  archiveFD = open(archiveFile, O_RDONLY);

  char fullPath[4096];
  getcwd(fullPath, sizeof(fullPath));

  // recursive call to dive into directories and make files based on contents
  unpackDirectory(fullPath);

  close(archiveFD);

  return 0;
}
