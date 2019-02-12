/* Modified by: xChaos, 20131220 */

#include "cll1-0.6.2.h"
#include "ipstruct.h"

#define FIRSTGROUPID 1024
#define FIRSTIPCLASS 2048

/* globals declared in prometheus.c */
extern struct IP *ips, *ip, *sharedip, *networks;
extern struct Group *groups, *group;
extern struct Keyword *keyword, *defaultkeyword, *keywords;
extern struct Macro *macro, *macros;
extern struct Textfile *previous_classmap, *textline;
extern int class_count;
extern int mix_new_hosts;
extern int ip_count;
extern int found_lmsid;
extern int free_min;
extern const int highest_priority;
extern char *ip6prefix;

void update_network(char *look_for, struct IP* ip);
/* implemented in networks.c */

/* This must be object oriented! This looks almost like constructor ;-) */
void TheIP(char *ipaddr, int is_network)
{
 create(ip,IP);
 ip->name         = "";
 ip->addr         = ipaddr;
 ip->sharing      = NULL;
 ip->prio         = highest_priority+1;
 ip->lmsid        = -1;
 ip->fixedprio    = \
 ip->aggregated   = \
 ip->mark         = \
 ip->min          = \
 ip->max          = \
 ip->desired      = \
 ip->credit       = \
 ip->upload       = \
 ip->proxy        = \
 ip->direct       = \
 ip->traffic      = \
 ip->traffic_down = \
 ip->traffic_up   = \
 ip->pktsup       = \
 ip->pps_limit    = \
 ip->pktsdown     = 0;
 ip->keyword      = keywords;
 ip->v6           = (strchr(ip->addr,':')!=NULL);
 ip->mask         = ((ip->v6)?64:32);
 if(is_network)
 {
  push(ip, networks);
 }
 else
 {
  push(ip, ips); 
 }
 ip_count++;
}

struct IP *lastIP6range, *lastIP6uplink;

/* == This function strips extra characters after IPv4 address and stores it = */
void parse_and_append_ip(char *str, struct IP *listhead)
{
 char *ptr, *ipaddr, *nextip6, *ip6buf; 
 char *ip6uplink = NULL, *ip6range = NULL, *ipname = NULL, *lmsid = NULL;

 if(ip6prefix) /* Try this only if IPv6 subsystem is active... */
 {
  ptr = strstr(str, "::");
  while(ptr && ptr-str > 4)
  {
   nextip6 = strstr(ptr + 2, "::");
   ptr -= 4;
   duplicate(ptr, ip6buf);
   ptr = strstr(ip6buf, "::");
   if(ptr)
   {
    if(*(ptr+2) == '+')
    {
     *(ptr+3) = 0; /* ends with ::+ */
     ip6uplink = ip6buf;
    }
    else
    {
     *(ptr+2) = 0; /* ends with :: */
     ip6range = ip6buf;
    }    
   }
   ptr = nextip6;
  }
 }

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
 ipname = ptr; 
 while(*ptr and *ptr!=' ' and *ptr!=9)
 {
  ptr++;
 }
 *ptr=0;

 if(ip6range)
 {
  concatenate(ip6prefix,ip6range,ptr);
  ip6range=ptr;
  if_exists(ip, ips, eq(ip->addr,ip6range)); /* check - allocated range must be unique */
  else
  {
   TheIP(ip6range, FALSE);
  }
  ip->name = ip6range;
  ip->keyword = defaultkeyword; /* settings for default keyword */
  if(lmsid)
  {
   ip->lmsid = atoi(lmsid);
  }
  lastIP6range = ip;
 }
 else
 {
  lastIP6range = NULL;
 }

 /* it is ugly to copy+paste and search+replace, but... */
 if(ip6uplink)
 {
  concatenate(ip6prefix,ip6uplink,ptr);
  ip6uplink=ptr;
  TheIP(ip6uplink, FALSE); /* always new IP - more IPs in single uplink network */
  ip->name = ip6uplink;
  ip->keyword = defaultkeyword; /* settings for default keyword */
  if(lmsid)
  {
   ip->lmsid = atoi(lmsid);
  }
  lastIP6uplink = ip;
 }
 else
 {
  lastIP6uplink = NULL;
 }

 if_exists(ip, listhead, eq(ip->addr,ipaddr));
 else
 {
  TheIP(ipaddr, (listhead==networks));
 }
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
 struct IP *network;
 int pktratio;

 parse(hosts)
 {
  str = _;

  if(*str < '0' or *str > '9')
  {
   /* any line starting with non-number is comment ...*/
   continue;
  }

  ptr = strchr(str,'\r'); /* fore unix-style end of line */
  if(ptr)
  {
   *ptr = 0;
  }
 
  /* first, expand (rewrite) any predefined macros, if found*/
  for_each(macro, macros)
  {
   substring = strstr(str, macro->rewrite_from);
   if(substring)
   {
    int l1, l3;
    *substring = 0;
    substring += strlen(macro->rewrite_from);
    l1 = strlen(str);
    l3 = strlen(substring);
    string(ptr, l1 + strlen(macro->rewrite_to) + l3 + 1);
    strcpy(ptr, str);
    strcat(ptr, macro->rewrite_to);
    strcat(ptr, substring);
    str = ptr;
    /*  printf("REWRITE: %s -> %s\n",_,str); */
   }
  }

  /* Does this IP share QoS class with some other ? */
  substring = strstr(str, "sharing-");
  if(substring)
  { 
   substring += 8; /* "sharing-" */
   parse_and_append_ip(str, ips);
   ip->sharing = substring;
   ip->keyword = defaultkeyword; /* settings for default keyword */
   if(lastIP6range)
   {
    lastIP6range->sharing = substring;
    lastIP6range = NULL;
   }
   if(lastIP6uplink)
   {
    lastIP6uplink->sharing = substring;
    lastIP6uplink = NULL;
   }
   while(*substring and *substring != '\n')
   {
    substring++;
   }
   *substring = 0; 
  }
  else
  {
   substring = strstr(str, "#255.");
   if(substring and not strstr(str, "#255.255.255.255")) /* do not ping /32 uplinks */
   {
    /* netmask detected - save network*/
    unsigned bit;
    unsigned num, mask = 8;
    substring += 5;
    while(substring && *substring)
    {
     ptr = substring;
     substring = strchr(substring, '.');
     if(substring)
     {
      *substring = 0;
      substring += 1;
     }
     num = atoi(ptr);
     for(bit = 1; bit <=128 ; bit<<=1)
     {
      if(bit & num)
      {
       mask++;
      }
     }
    } 
    parse_and_append_ip(str, networks);
    ip->mask = mask;
   }
   else
   {
    /*Do we have to create new QoS class for this IP ? */
    if_exists(keyword,keywords,(substring=strstr(str,keyword->key)))
    {
     parse_and_append_ip(str, ips);
     if(lastIP6range)
     {
      lastIP6range->sharing = ip->name;
      lastIP6range = NULL;
     }
     if(lastIP6uplink)
     {
      lastIP6uplink->sharing = ip->name;
      lastIP6uplink = NULL;
     }
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
      if(ip->max < ip->min)
      {
       ip->max = ip->min;
      }
     }

     /* avg MTU bytes * 8 >> 10 = in bits, max is in kb/s  */
     pktratio = (ip->keyword->allowed_avgmtu*8) >> 10;
     if(pktratio > 0)
     {
      ip->pps_limit = ip->max/pktratio;
      if(ip->pps_limit > 10000) /* this limit seems to be hardcoded in iptables */
      {
       ip->pps_limit = 0; /* do not apply packet limits */
      }
     }

     if(mix_new_hosts)
      for_each(textline, previous_classmap)
      {
       ptr = strchr(textline->str, ' ');
       if(ptr)
       {
        if(!strncmp(ip->addr, textline->str, ptr-textline->str))
        {
         ip->mark = atoi(ptr+1);
         printf("Match class: %s %d\n", ip->addr, ip->mark);
        }
       }      
      }
     
     if(!mix_new_hosts || !ip->mark)
      ip->mark = FIRSTIPCLASS+1+class_count++;
          
     update_network(ip->addr, ip);

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
   }//endif netmask
  }//endif sharing-
 }
 fail
 {
  perror(hosts);
  exit(-1);
 }
 done; /* ugly macro end */
// TheIP("0.0.0.0", TRUE);
// ip->name = "TOTAL";
// ip->mask = 0;
}
