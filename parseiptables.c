/* Modified by: xChaos, 20131208 */

#include "cll1-0.6.2.h"
#include "ipstruct.h"

#define STRLEN 512

/* globals declared in prometheus.c */
extern struct IP *ips, *ip, *sharedip;
extern char *mark;
/* extern char *proxy_ip; */
extern int free_min;
extern int free_max;
extern int include_upload;

/* ===================== traffic analyser - uses iptables  ================ */ 

void get_traffic_statistics(const char *whichiptables, int ipv6)
{
 char *str,*cmd;
 int downloadflag=0;

 textfile(Pipe,str) *line,*lines=NULL;
 string(str,STRLEN);
 string(cmd,STRLEN);

 sprintf(cmd,"%s -L -v -x -n -t mangle", whichiptables);
 shell(cmd);
 input(str,STRLEN)
 {
  create(line,Pipe);
  line->str=str;
  string(str,STRLEN);
  append(line,lines);
 }

 for_each(line,lines)
 {
  int col, accept = 0, /*proxyflag = 0, */valid = 1, setchainname = 0, commonflag = 0; 
  unsigned long long traffic = 0;
  unsigned long pkts = 0;
  char *ipaddr = NULL,*ptr;
  
  valid_columns(ptr, line->str, ' ', col)
  if(valid) switch(col)
  { 
   case 1: if(eq(ptr,"Chain"))
           {
            setchainname = 1;
           }
           else if(eq(ptr,"pkts")) 
           {
            valid = 0;
           }
           else
           {
            sscanf(ptr,"%lu",&pkts); 
           }
           break;
   case 2: if(setchainname)
           {
            if(!strncmp(ptr,"post_",5) || eq(ptr,"POSTROUTING"))
            {
             downloadflag = 1;            
            }
            else 
            {
             if(!strncmp(ptr,"forw_",5) || eq(ptr,"FORWARD"))
             {
              downloadflag = 0;
             }
            }            
            if(eq(ptr,"post_common") || eq(ptr,"forw_common"))
            {
             commonflag = 1;
            }
           }
           else
           {
            sscanf(ptr,"%Lu",&traffic); 
            traffic += (1<<19);
            traffic >>= 20;
           }
           break;
   case 3: if((strncmp(ptr,"post_",5) && strncmp(ptr,"forw_",5)) || commonflag)
           {
            accept = eq(ptr,mark);
           }
           break;
   case 7: if(ipv6 && !downloadflag)
           {
            ipaddr = ptr;
           }
           break;
   case 8: if(ipv6 && downloadflag)
           {
            ipaddr = ptr;
           }
           else if(!ipv6)
           {
/*          if(downloadflag)
            { 
             if(strstr(proxy_ip,ptr))
             {
              proxyflag = 1;
             }

            } 
            else 
            {  */
            if(!downloadflag)
            {
             ipaddr = ptr;
            }
           }
           break;
   case 9: if(!ipv6 && downloadflag)
           {
            ipaddr = ptr;
           }
           break;
  }
  
  if(accept && traffic>0 && ipaddr)
  {
   /* IPv6 subnet - /64 */
   char *isipv6 = strstr(ipaddr,"/64");
   if(ipv6 && isipv6)
   {
    *isipv6=0;
    printf("(IPv6) ");
   }
   else
   {
    printf("(IPv4) ");
   }
/*   
   if(proxyflag)
   {
    printf("(proxy) ");
   }
   else
*/   
   if(!downloadflag)
   {
    printf("(up) ");
   }
   else
   {
    printf("(down) ");
   }
   
   printf("%s %Lu MB (%ld pkts)\n", ipaddr, traffic, pkts);

   if_exists(ip, ips, eqi(ip->addr,ipaddr)); 
   else 
   {
    TheIP(ipaddr, FALSE);
    if(eq(ip->addr,"0.0.0.0/0"))
    {
     ip->name = "(unregistered)";
     ip->min = free_min;
     ip->max = ip->desired=free_max;
    }
    else
    {
     ip->name = ipaddr;
    }
   }
   
   if(downloadflag)
   {
/*
    if(proxyflag)
    {
     ip->proxy = traffic;
    }
    else
    {*/
    ip->traffic += traffic;
/*    } */
    ip->direct += traffic; /*-ip->proxy;*/
    ip->pktsdown += pkts;
   }
   else
   {
    ip->upload += traffic;
    ip->pktsup += pkts;
    if(include_upload)
    {
     ip->traffic += traffic;
    }
    else 
    {
     if(ip->upload > ip->traffic)
     {
      ip->traffic = ip->upload;
     }
    }
   }
  }  
 }
 free(cmd);
}
