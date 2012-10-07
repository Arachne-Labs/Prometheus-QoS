/* Modified by: xChaos, 20121007 */

#include "cll1-0.6.2.h"
#include "ipstruct.h"

#define FIRSTGROUPID 1024
#define FIRSTIPCLASS 2048

/* globals declared in prometheus.c */
extern struct IP *ips, *ip, *sharedip;
extern struct Group *groups, *group;
extern struct Keyword *keyword, *defaultkeyword, *keywords;
extern int class_count;
extern int ip_count;
extern int found_lmsid;
extern int free_min;

/* function implemented in prometheus.c */
void TheIP(void);

/* == This function strips extra characters after IPv4 address and stores it = */
void parse_ip(char *str)
{
 char *ptr, *ipaddr = NULL, *ipname = NULL, *lmsid = NULL;

 ptr = strchr(str, '{');
 if(ptr)
 {
  lmsid = ++ptr;
  while(*ptr and *ptr != '}')
  {
   ptr++;
  }
  *ptr = 0;
 }
 
 ptr = str;
 while(*ptr and *ptr!=' ' and *ptr!=9)
 {
  ptr++;
 }
 
 *ptr = 0;
 ipaddr = str;
 ptr++;
 while(*ptr and (*ptr==' ' or *ptr==9))
 {
  ptr++;
 }
 ipname=ptr; 
 while(*ptr and *ptr!=' ' and *ptr!=9)
 {
  ptr++;
 }
 *ptr=0;

 if_exists(ip, ips, eq(ip->addr,ipaddr));
 else
 {
  TheIP();
 }
 ip->addr = ipaddr;
 ip->name = ipname;
 if(lmsid)
 {
  ip->lmsid = atoi(lmsid);
  found_lmsid = TRUE;
 }
}

/* == This function parses hosts style main configuration file == */
void parse_hosts(char *hosts)
{
 int groupidx = FIRSTGROUPID;
 char *str, *ptr;
 char *substring;

 parse(hosts)
 {
  str=_;

  if(*str < '0' or *str > '9')
  {
   /* any line starting with non-number is comment ...*/
   continue;
  }
  
  /* Does this IP share QoS class with some other ? */
  substring = strstr(str, "sharing-");
  if(substring)
  { 
   substring += 8; /* "sharing-" */
   parse_ip(str);
   ip_count++;
   ip->sharing = substring;
   ip->keyword = defaultkeyword; /* settings for default keyword */
   while(*substring and *substring != '\n')
   {
    substring++;
   }
   *substring = 0; 
  }
  else
  {
   /*Do we have to create new QoS class for this IP ? */

   if_exists(keyword,keywords,(substring=strstr(str,keyword->key)))
   {
    parse_ip(str);
    ip_count++;
    ip->keyword = keyword;
    keyword->ip_count++;
    ip->prio = keyword->default_prio;
    substring += strlen(keyword->key)+1;
    ptr = substring;
    while(*ptr and *ptr != '-')
    {
     ptr++;
    }
    if(*ptr == '-')
    {
     *ptr=0;
     ip->max = ip->desired = atoi(ptr+1);
    }
    ip->min = atoi(substring);
    if(ip->min <= 0)
    {
     printf(" %s: Illegal value of minimum bandwidth 0 kbps, using %d kb/s\n",
            str, free_min);
     ip->min = free_min;
    }
    if(ip->max <= ip->min)
    {
     ip->fixedprio = TRUE;
     ip->max = ip->min + ip->keyword->reserve_min;
    }
    else 
    {
     ip->max -= ip->keyword->reserve_max;
     if(ip->max<ip->min)
     {
      ip->max=ip->min;
     }
    }
    ip->mark = FIRSTIPCLASS+1+class_count++;

    if_exists(group,groups,(group->min == ip->min)) 
    { 
     group->count++;      
     group->desired += ip->min;
     ip->group = group->id;   
    }
    else
    {
     create(group,Group);
     group->min = ip->min;
     group->id = groupidx++;
     ip->group = group->id;

     if(group->min < 8) group->min = 8;
     /* Warning - this is maybe because of primitive tc namespace, can be fixed */
     /* it is because class IDs are derived from min. bandwidth. - xCh */
     //if(group->min>MAX_GUARANTED_KBPS) group->min=MAX_GUARANTED_KBPS;
     
     group->count = 1;
     group->desired = ip->min;   
     insert(group, groups, desc_order_by,min);
    }
   }//endif keyword-
  }//endif sharing-
 }
 fail
 {
  perror(hosts);
  exit(-1);
 }
 done; /* ugly macro end */
}