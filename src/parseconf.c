#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "irdctype.h"

int mapOption(const char* opt){
  if (strncmp(opt, "cancel:", 7)==0) return 1;
  if (strncmp(opt, "1:", 2)==0) return 2;
  if (strncmp(opt, "2:", 2)==0) return 3;
  if (strncmp(opt, "3:", 2)==0) return 4;
  if (strncmp(opt, "4:", 2)==0) return 5;
  if (strncmp(opt, "5:", 2)==0) return 6;
  if (strncmp(opt, "6:", 2)==0) return 7;
  if (strncmp(opt, "7:", 2)==0) return 8;
  if (strncmp(opt, "8:", 2)==0) return 9;
  if (strncmp(opt, "9:", 2)==0) return 10;
  if (strncmp(opt, "0:", 2)==0) return 11;
  if (strncmp(opt, "tab:", 4)==0) return 15;
  if (strncmp(opt, "epg:", 4)==0) return 18;
  if (strncmp(opt, "pause:", 6)==0) return 20;
  if (strncmp(opt, "rewind:", 7)==0) return 23;
  if (strncmp(opt, "capture:", 8)==0) return 25;
  if (strncmp(opt, "enter:", 6)==0) return 28;
  if (strncmp(opt, "teletext:", 9)==0) return 30;
  if (strncmp(opt, "preview:", 8)==0) return 37;
  if (strncmp(opt, "list:", 5)==0) return 38;
  if (strncmp(opt, "recall:", 7)==0) return 46;
  if (strncmp(opt, "favorite:", 9)==0) return 47;
  if (strncmp(opt, "forward:", 8)==0) return 49;
  if (strncmp(opt, "mute:", 5)==0) return 50;
  if (strncmp(opt, "rec:", 4)==0) return 102;
  if (strncmp(opt, "vol+:", 5)==0) return 103;
  if (strncmp(opt, "ch+:", 4)==0) return 104;
  if (strncmp(opt, "stop:", 5)==0) return 107;
  if (strncmp(opt, "vol-:", 5)==0) return 108;
  if (strncmp(opt, "ch-:", 4)==0) return 109;
  return 0;
}

/*

             TODO
             - check the patch for execve
             - free structure procedure
             - ret_code as an enumeration item
             
*/
int parse_conf(const char* conf_filename, int* n_params, config_item* config_params[]){

  size_t COUNT = 512;
  size_t MAXBUFLEN = 10000;
  char* LINESEP = "\n";
  char* CMDSEP = " \t";

  ssize_t len;
  char buf[512];
  char* config_raw;
  int left;
  int cmd_idx;
  int tmpOpt;
  
  char *line, *saveptr1;
  char *token, *saveptr2;
  
  config_item *tmplist = NULL;
  char** arg_list;
  
  int ret_code = IRDC_OK;
  
  FILE* conf_file = fopen(conf_filename, "r");
  if( conf_file==NULL ){
    return IRDC_NOCNFERR;
  }

  int conf_fd = fileno(conf_file);
  if( conf_fd<0 ){
    ret_code = IRDC_NOCNFERR;
    goto close_all;
  }
  
  memset(buf, 0, COUNT);
  if( config_raw=malloc(MAXBUFLEN) ){
    memset(config_raw, 0 , MAXBUFLEN);
    left = MAXBUFLEN-1;
  }else{
    ret_code = IRDC_MEMERR;
    goto close_all;
  }
  
  len = read(conf_fd, buf, COUNT);
  while( len>0 && len<left ){
  
    strncat(config_raw, buf, len);
    left = left - len;
    
    memset(buf, 0, COUNT);
    len = read(conf_fd, buf, COUNT);
  }
  
  if( len<0 ){
    ret_code = IRDC_CNFREADERR;
    goto close_all;
  }
  if( len>left ){
    ret_code = IRDC_CNFLONGERR;
    goto close_all;
  }
  
  
  tmplist = malloc(NUMOFBUTTONS * sizeof(config_item));
  *n_params = 0;
  arg_list = NULL;
  
  line = strtok_r(config_raw, LINESEP, &saveptr1);
  while( line!=NULL ){
    if( line[0]!='#' ){
      cmd_idx = 0;
      token = strtok_r(line, CMDSEP, &saveptr2);
      while( token!=NULL ){
        switch(cmd_idx){
          case 0:
            tmpOpt = mapOption(token);
            if (tmpOpt==0) goto next_line;
            tmplist[*n_params].sel = tmpOpt;
          break;
          case 1:
            tmplist[*n_params].executable = strdup(token);
            tmplist[*n_params].args = NULL;
          break;
          case 2:
            arg_list = malloc(NUMOFARGS * sizeof(char*));
            if (arg_list==NULL){
              ret_code = IRDC_MEMERR;
              goto close_all;
            }
            tmplist[*n_params].args = arg_list;
            arg_list[cmd_idx-2] = strdup("");               /*just a path*/
          default:
            if (cmd_idx>NUMOFARGS-2){
              ret_code = IRDC_TOOARGSERR;
              goto close_all;
            }
            arg_list[cmd_idx-1] = strdup(token);
        }
        token = strtok_r(NULL, CMDSEP, &saveptr2);
        cmd_idx++;
      }
      if (arg_list!=NULL){
        arg_list[cmd_idx-1] = NULL;
      }
      if (cmd_idx>0){
        *n_params = (*n_params) + 1;
      }
    }
next_line:
    line = strtok_r(NULL, LINESEP, &saveptr1);
  }
  
  *config_params = tmplist;
  
  free(config_raw);  

close_all:
  fclose(conf_file);
  return ret_code;
}
