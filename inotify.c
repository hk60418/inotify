#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <sys/inotify.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

void usage(void) {
  (void)fprintf(stderr, "%s\n%s\n\n%s\n%s\n",
		"usage: inotify [-m | --mask MASK] ... PATH ...",
		"       monitor file paths",
		"       MASK is event mask of the inotify interface",
		"       PATH is any file system path"
		);
  exit(1);
}

extern int errno;

// linked list of paths and respective watch descriptors
struct node {
  char * path;
  struct node * next;
  int watch_descriptor;
};

struct node * files;

// find path for watch descriptor
char * find_wd_path(int wd) {
  struct node * helper;
  helper = files;
  while(helper->watch_descriptor != wd) {
    helper = helper->next;
    if(helper == NULL)
      return NULL;
  }
  return helper->path;
}

// quit with errno message
void errno_quit(char * msg) {
  fprintf(stderr, "Error: %s: %s\n", msg, strerror(errno));
  _exit(1);
}

int main(int argc, char ** argv) {
  int inotify_instance;
  int c;
  int digit_optind;
  uint32_t mask;
  struct node * files_helper;
  struct stat statbuf;
  int readval;
  char action[9];

  int watch_descriptor;
  int f_descriptor;
  struct inotify_event * results;
  time_t timer;
  char timebuffer[20];
  struct tm* tm_info;

  mask = 0;
  files = NULL;
  files_helper = files;
  while(1) {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    static struct option long_options[] = {
      {"mask", required_argument, NULL, 'm'},
      {0, 0, 0, 0}
    };
    
    c = getopt_long(argc, argv, "m:", long_options, &option_index);

    if (c == -1)
      break;
    
    switch (c) {
    case 'm':
      if(strcmp("IN_ACCESS", optarg) == 0)
	mask = mask | IN_ACCESS;
      else if(strcmp("IN_ATTRIB", optarg) == 0)
	mask = mask | IN_ATTRIB;
      else if(strcmp("IN_CLOSE_WRITE", optarg) == 0)
	mask = mask | IN_CLOSE_WRITE;
      else if(strcmp("IN_CREATE", optarg) == 0)
	mask = mask | IN_CREATE;
      else if(strcmp("IN_DELETE", optarg) == 0)
	mask = mask | IN_DELETE;
      else if(strcmp("IN_DELETE_SELF", optarg) == 0)
	mask = mask | IN_DELETE_SELF;
      else if(strcmp("IN_MODIFY", optarg) == 0)
	mask = mask | IN_MODIFY;
      else if(strcmp("IN_MOVE_SELF", optarg) == 0)
	mask = mask | IN_MOVE_SELF;
      else if(strcmp("IN_MOVED_FROM", optarg) == 0)
	mask = mask | IN_MOVED_FROM;
      else if(strcmp("IN_MOVED_TO", optarg) == 0)
	mask = mask | IN_MOVED_TO;
      else if(strcmp("IN_OPEN", optarg) == 0)
	mask = mask | IN_OPEN;
      break;
    default:
      fprintf(stderr, "unknown option");
      break;
    }    
  }
  
  // Set mask for all events if non was given
  if( mask == 0)
    mask = IN_ACCESS | IN_ATTRIB | IN_CLOSE_WRITE | IN_CLOSE_NOWRITE | 
      IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF | 
      IN_MOVED_FROM | IN_MOVED_TO | IN_OPEN;
  
  files = NULL;
  files_helper = files;
  while(argv[optind]) {
    if(files == NULL) {
      errno = 0;
      files = (struct node *) malloc(sizeof(struct node));
      if(files == NULL)
	errno_quit("Could not reserve memory for internal structures");
      files_helper = files;
    } else {
      errno = 0;
      files_helper->next = (struct node *) malloc(sizeof(struct node));
      if(files_helper->next == NULL)
	errno_quit("Could not reserve memory for internal structures");
      files_helper = files_helper->next;
    }
    files_helper->path = (char *) malloc(strlen(argv[optind]) + 2);
    if(files_helper->path == NULL)
      errno_quit("Could not reserve memory for internal structures");
    strcpy(files_helper->path, argv[optind]);
    // If dir is monitored, then ensure that it is printed with ending '/'
    if(files_helper->path[strlen(argv[optind]) - 1] != '/') {
      errno = 0;
      if(stat(files_helper->path, &statbuf) == -1)
	errno_quit("System error");
      if(S_ISDIR(statbuf.st_mode))
	strcat(files_helper->path, "/");
    }
    files_helper->next == NULL;
    optind = optind + 1;
  }
  
  if(files == NULL)
    usage();
  
  f_descriptor = inotify_init();
  if(f_descriptor == -1)
    errno_quit("Could not create inotify instance");
  files_helper = files;
  while(files_helper != NULL) {
    files_helper->watch_descriptor = inotify_add_watch(f_descriptor, 
						       files_helper->path, 
						       mask);
    if(files_helper->watch_descriptor == -1)
      errno_quit("Could not create inotify watch");
    files_helper = files_helper->next;
  }
  
  errno = 0;
  results = (struct inotify_event *) 
    malloc(sizeof(struct inotify_event) + NAME_MAX + 1);
  if(results == NULL)
    errno_quit("Could not allocate memory for internal structures");
  
  while( 1 == 1 ) {
    memset(results, 0, sizeof(struct inotify_event) + NAME_MAX + 1);
    readval = read(f_descriptor, 
		   results, 
		   sizeof(struct inotify_event) + NAME_MAX + 1 );
    if(readval == -1)
      errno_quit("Could not read inotify instance");
    if(readval > 0) {
      if(time(&timer) == ((time_t) -1))
	errno_quit("Could not read time");
      tm_info = localtime(&timer);
      if(tm_info == NULL)
	errno_quit("Could not read time");
      strftime(timebuffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
      if(strlen(timebuffer) == 0)
	errno_quit("Could not read time");
      if(printf("%s %s%s ", 
		timebuffer, 
		find_wd_path(results->wd), 
		results->name) 
	 < 0)
	errno_quit("Could not print the results");
      switch(results->mask) {
      case IN_ACCESS:
	{
	  strcpy(action, "accessed");
	  break;
	}
      case IN_ATTRIB:
      	{
	  strcpy(action, "changed");
	  break;
	}
      case IN_CLOSE_WRITE:
	{
	  strcpy(action, "closed");
	  break;
	}
      case IN_CLOSE_NOWRITE:
	{
	  strcpy(action, "closed");
	  break;
	}
      case IN_CREATE:
	{
	  strcpy(action, "created");
	  break;
	}
      case IN_DELETE:
	{
	  strcpy(action, "deleted");
	  break;
	}
      case IN_DELETE_SELF:
	{
	  strcpy(action, "deleted");
	  break;
	}
      case IN_MODIFY:
	{
	  strcpy(action, "modified");
	  break;
	}
      case IN_MOVE_SELF:
	{
	  strcpy(action, "moved");
	  break;
	}
      case IN_MOVED_FROM:
	{
	  strcpy(action, "moved");
	  break;
	}
      case IN_MOVED_TO:
	{
	  strcpy(action, "moved");
	  break;
	}
      case IN_OPEN:
	{
	  strcpy(action, "opened");
	  break;
	}
      default:
	{
	  strcpy(action, "\n");
	  break;
	}
      }
      if(printf("%s\n", action) < 0)
	errno_quit("Sould not print the results");
    }
  }
  
  return 0;
}
