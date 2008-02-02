
#include "cll1-0.6.h"

#define STRLEN 128

const char *logdir="/var/www/logs";
const char *htmldir="/var/www/logs/html";

/* Modified: Michal Polak (xChaos) 20070202 */

struct Ip
{
 char *name;
 long traffic;
 list(Ip);
} *ip,*ips;

int main (int argc, char **argv) 
{
 char *month,*year,*str,*name,*ptr,*ptr2;
 long traffic,traffic_month,total=0;
 int col,col2,y_ok,m_ok,accept_month,i=1,any_month=0;
 FILE *f;
 
 string(str,STRLEN);

 if(argc<3)
 {
  puts("Usage: monthly-stats Mmm YYYY (Mmm=Jan-Dec or Year, YYYY=year)");
  exit(-1);
 }
 else
 {
  month=argv[1];
  if(eq(month,"Year")) any_month=1;
  year=argv[2];
 }

 sprintf(str,"/bin/ls %s/*.log",logdir);
 shell(str);
 input(str,STRLEN)
 {
  ptr=strrchr(str,'\n');
  if(ptr) *ptr='\0';
  printf("Parsing %s ...",str);
  accept_month=0;
  traffic_month=0;
  parse(str)
  {
   y_ok=m_ok=0;  
   valid_columns(ptr,_,'\t',col) switch(col)
   {
    case 2: name=ptr;break;
    case 3: traffic=atol(ptr);break;
    case 7: valid_columns(ptr2,ptr,' ',col2) switch(col2)
            {
             case 2: if(any_month || eq(ptr2,month)) m_ok=1; break;
             case 5: if(eq(ptr2,year)) y_ok=1; break;
            }
   }
   if(y_ok && m_ok) 
   {
    traffic_month+=traffic;
    accept_month=1;
   }
  }
  done;
  if(accept_month)
  {
   create(ip,Ip);
   ip->name=name;
   ip->traffic=traffic_month;
   insert(ip,ips,desc_order_by,traffic);
   printf(" %ld MB\n",ip->traffic);
  }
  else
   puts(" no records.");
 }
 sprintf(str,"%s/%s-%s.html",htmldir,year,month);
 printf("Writing %s ...",str);
 f=fopen(str,"w");
 if(f)
 {
  fprintf(f,"<table border><tr><th colspan=\"4\">Data transfers - %s %s</th></tr>\n ",month,year);
  every(ip,ips)
   if(ip->traffic)
   {
    fprintf(f,"<tr><td align=\"right\">%d</td><th>%s</td><td align=\"right\">%ld MB</td><th align=\"right\">%ld GB</th></tr>\n",i++,ip->name,ip->traffic,ip->traffic>>10);
    total+=ip->traffic>>10;
   }
  fprintf(f,"<tr><th colspan=\"3\" align=\"left\">Total:</th><th align=\"right\">%ld GB</th></tr>\n",total);
  fputs("</table>\n",f);
  fclose(f);
  puts(" done.");
 }
}
