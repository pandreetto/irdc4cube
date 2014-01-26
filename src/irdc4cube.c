#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <syslog.h>
#include <signal.h>
#include "parseconf.h"
#include "scan.h"
#include "irdctype.h"

extern char **environ;
int goon = 1;


void printUsage(){
  fprintf(stdout, "Usage: irdc4cube -c configfile [-d device dir] [-h]\n");
}

void signal_handler(int sig){
  switch(sig){
  case SIGTERM:
    goon = 0;
    break;
  }
}

int launch(char* executable, char** args){
  int child_status;
  
  int pid = fork();
  if( pid==0 ){
    
    if( execve(executable, args, environ)<0 ){
      return -1;
    }
    return 0;
    
  }else if( pid>0 ){
    
    if( waitpid(pid, &child_status, 0)<0 ){
      return -1;
    }
    
    if( WIFEXITED(child_status) ){
      syslog(LOG_INFO, "Application %s terminates with code: %d",
          executable, WEXITSTATUS(child_status));
    }
    
    if( WIFSIGNALED(child_status) ){
      syslog(LOG_INFO, "Application %s received signal: %d",
          executable, WTERMSIG(child_status));
    }
        
  }else{
    return 1;
  }
  
  return 0;
}

int main(int argc, char* argv[]){

  int opt;
  char* config_file = NULL;
  char* dev_dir = NULL;
  char* dev_filename = NULL;
  config_item *parameters;
  int n_params;
  int k;
  int tmp_fd;
  
  char dev_type[EV_MAX];
  int yalv;
  FILE* dev_file = NULL;
  int dev_fd = 0;
  fd_set rfds;
  size_t rb;
  struct input_event ev[64];
  
  int ret_code = IRDC_OK;
  
  pid_t s_id;
  int daemon_pid;
  
  while ((opt = getopt(argc, argv, "hc:d:")) != -1) {
    switch (opt) {
    case 'h':
      printUsage();
      return IRDC_OK;
    case 'c':
      if( (config_file = strdup(optarg)) == NULL){
        fprintf(stderr, "Cannot allocate memory");
        return IRDC_MEMERR;
      }
      break;
    case 'd':
      if( (dev_dir = strdup(optarg)) == NULL){
        fprintf(stderr, "Cannot allocate memory");
        return IRDC_MEMERR;
      } 
    default:
      printUsage();
      return IRDC_OPTERR;
    }
  }
  
  if( config_file==NULL ){
    printUsage();
    return IRDC_OPTERR;
  }
  
  /*
    Daemon code
  */  

  daemon_pid = fork();

  if (daemon_pid<0) return IRDC_DAEMERR;
  if (daemon_pid>0) return IRDC_OK;

  if ((s_id = setsid())<0) {
    return IRDC_DAEMERR;
  }

  /* close all descriptors */
  for (tmp_fd=getdtablesize();tmp_fd>=0;--tmp_fd) {
    close(tmp_fd);
  }

  tmp_fd = open("/dev/null",O_RDWR); /* open stdin */
  dup(tmp_fd); /* stdout */
  dup(tmp_fd); /* stderr */

  umask(027);
  chdir("/tmp/");
  
  signal(SIGTERM, signal_handler);
  
  openlog(argv[0], LOG_ODELAY, LOG_USER);
  
  /* TODO read pid filename from environ */
  
  tmp_fd = open("/var/run/irdc4cube.pid",O_RDWR|O_CREAT,0640);
  if (tmp_fd<0) {
    syslog(LOG_ERR, "Cannot open /var/run/irdc4cube.pid");
    return IRDC_DAEMERR;
  }
  
  if (lockf(tmp_fd,F_TLOCK,0)<0) {
    syslog(LOG_ERR, "Cannot lock /var/run/irdc4cube.pid");
    return IRDC_DAEMERR;
  }else{
    char pidStr[64];
    memset(pidStr, 0, 64);
    snprintf(pidStr, 63, "%d\n",getpid());
    if (write(tmp_fd, pidStr, strlen(pidStr))<0){
      syslog(LOG_ERR, "Cannot write pid to /var/run/irdc4cube.pid");
      return IRDC_DAEMERR;
    }
    close(tmp_fd);
  }

  /* End daemon code */
  
  if( dev_dir==NULL ){
    if( (dev_dir = strdup("/dev/input")) == NULL){
      syslog(LOG_ERR, "Cannot allocate memory");
      return IRDC_MEMERR;
    }      
  }
  
  if ((ret_code = parse_conf(config_file, &n_params, &parameters))!= IRDC_OK){
    syslog(LOG_ERR, "Error parsing configuration file");
    goto free_strings;
  }else{

    for(k=0; k< n_params; k++){
      char** arg_list = parameters[k].args;
      char* item;
      int j = 0;
      syslog(LOG_DEBUG, "executable %s (%d)", parameters[k].executable, parameters[k].sel);
      item = arg_list[j];
      while(item!=NULL){
        syslog(LOG_DEBUG, "           %s", item);
        j++;
        item = arg_list[j];
      }
    }
  }    
   
  if( scan(dev_dir, &dev_filename)>0 ){
    syslog(LOG_ERR, "Error scanning directory: %m");
    ret_code = IRDC_SCANERR;
  }else if( dev_filename!=NULL ){
    syslog(LOG_INFO, "Found device in %s\n", dev_filename);

    memset(dev_type, 0, sizeof(dev_type));
    dev_file = fopen(dev_filename, "r");
    if (dev_file==NULL) {
      syslog(LOG_ERR, "Cannot open event device: %m");
      ret_code = IRDC_DEVERR;
      goto free_strings;
    }

    dev_fd = fileno(dev_file);
    if (dev_fd<0) {
      syslog(LOG_ERR, "Wrong stream: %m");
      ret_code = IRDC_DEVERR;
      goto close_device;
    }else{
      FD_ZERO(&rfds);
      FD_SET(dev_fd, &rfds);
    }
    
    /* TODO investigate pselect and signal interaction */
    while (goon>0){
      int retval = select(dev_fd+1, &rfds, NULL, NULL, NULL);
      if (retval<0) {
        if (errno==EINTR){
          syslog(LOG_INFO, "Signal detected");
          continue;
        } else {
          syslog(LOG_ERR, "Error reading from device: %m");
          break;
        }
      }
       
      rb=read(dev_fd, ev, sizeof(struct input_event)*64);
      if (rb < (int) sizeof(struct input_event)) {
        syslog(LOG_ERR, "Error reading input: %m");
        ret_code = IRDC_READERR;
        goto close_device;
      }

      for (yalv = 0;
           yalv < (int) (rb / sizeof(struct input_event));
           yalv++){
        if (ev[yalv].type!=1 || ev[yalv].value!=1) continue;
        
        for(k=0; k<n_params; k++){
          if (parameters[k].sel==ev[yalv].code) {
            if (launch(parameters[k].executable,
                parameters[k].args)!=0) {
              syslog(LOG_ERR, "Cannot execute command %s",
                  parameters[k].executable);
            }
          }
        }        

      }
      
    }

close_device:
    fclose(dev_file);

  }else{
    syslog(LOG_ERR, "Cannot find IR device");
    ret_code = IRDC_NODEVERR;
  }
  
  syslog(LOG_INFO, "Shutdown");
  
  if (unlink("/var/run/irdc4cube.pid")<0){
    syslog(LOG_WARNING, "Cannot remove file /var/run/irdc4cube.pid");
  }
  
  closelog();

  /*
    TODO
    free the parameters list, method in parseconf.c
  */  
free_strings:
  free(dev_filename);
  free(dev_dir);
  free(config_file);

  return ret_code;
}
