#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <linux/input.h>
#include <stddef.h>
#include <unistd.h>
#include "irdctype.h"

#define test_bit(bit, array) (array[bit/8] & (1<<(bit%8)))

int scan(const char* dev_dir, char** dev_path){

  struct dirent *dir_item;
  struct dirent *dummy;
  int scan = 1;
  struct input_id dev_info;
  char *event_filename;
  int found = 0;
  char dev_type[EV_MAX/8 + 1];
  
  dir_item = malloc(offsetof(struct dirent, d_name) +
                     pathconf(dev_dir, _PC_NAME_MAX) + 1);
  
  
  DIR *d_stream = opendir(dev_dir);
  if (d_stream==NULL){
    errno = EIO;
    return 1;
  }
  
  while( scan ){
    if( readdir_r(d_stream, dir_item, &dummy)==0 ){

      if( dummy!=NULL ){
        if( strncmp(dir_item->d_name, "event", 5)==0 ){
          
          int path_len = strlen(dev_dir)+strlen(dir_item->d_name)+2;
          event_filename = malloc(path_len);
          memset(event_filename, 0, path_len);
          strcat(event_filename, dev_dir);
          strcat(event_filename, "/");
          strcat(event_filename, dir_item->d_name);
          
          FILE* dev_file = fopen(event_filename, "r");
          if (dev_file==NULL) {
            goto next_device;
          }

          int dev_fd = fileno(dev_file);
          if (dev_fd<0) {
            goto close_device;
          }
            
          if (ioctl(dev_fd, EVIOCGID, &dev_info)) {
            goto close_device;
          }
          
          if( dev_info.vendor==VENDORID && dev_info.product==PRODUCTID ){
            
            if (ioctl(dev_fd, EVIOCGBIT(0, EV_MAX), dev_type) < 0) {
              goto close_device;
            }
            /*
              The IRDC has 2 different devices inside
              the keyboard is the only one with LED supported
            */
            if (test_bit(EV_LED, dev_type)) {
              found = 1;
              scan = 0;
              *dev_path = event_filename;
            }
          }
           
close_device:
          fclose(dev_file);          
next_device:
          if( found==0 ){
            free(event_filename);
          }     
        }
      }else{
        scan = 0;
      }      
    }
  }
  
  free(dir_item);

  if (closedir(d_stream)<0) {
    return 1;
  }
  
  if( found==0 ){
    *dev_path = NULL;
  }

  return 0;
}
