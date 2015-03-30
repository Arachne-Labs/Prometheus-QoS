#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

/* http://stackoverflow.com/questions/4021479/getting-file-modification-time-on-unix-using-utime-in-c */
time_t get_mtime(const char *path)
{
 struct stat statbuf;
 if (stat(path, &statbuf) == -1) 
 {
  perror(path);
  exit(1);
 }
 return statbuf.st_mtime;
}

/* just text line parsing */
char *parse_datafile_line(char *str)
{
 char *ptr = strchr(str,' ');
 if(!ptr)
 {
  ptr = strchr(str,'\t');
 }

 if(ptr)
 {
  *ptr = 0;
  ptr++;
  while(*ptr == ' ' || *ptr == '\t')
  {
   ptr++;
  }
  return ptr;
 }
 else
 {
  return NULL;
 }
}
