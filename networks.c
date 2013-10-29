/* Modified by: xChaos, 20131029 */

#include "cll1-0.6.2.h"
#include "ipstruct.h"

#define STRLEN 512

extern struct IP *ips, *networks;

struct IP* find_network_for_ip(char *ipaddr_orig)
{
 struct IP *network;
 char *netaddr, *lastnum, *ipaddr;
 int ipnum, netnum;

 duplicate(ipaddr_orig, ipaddr);
 lastnum = strrchr(ipaddr, '.');
 if(lastnum)
 {
  ipnum = atoi(lastnum + 1);
  *lastnum = 0;
 }

 for_each(network, networks)
 {
  duplicate(network->addr, netaddr);
  lastnum = strrchr(netaddr, '.');
  if(lastnum)
  {
   netnum = atoi(lastnum + 1);
   *lastnum = 0;
//   printf("%s/%d + %d\n",network->addr,network->mask,(1<<(32-network->mask)));
   if(     eq(netaddr, ipaddr) 
       and netnum + (1<<(32-network->mask)) > ipnum
       and netnum <= ipnum)
   {
    return network;
   }       
  }
 }
 return NULL;
}

void analyse_topology(char *traceroute)
{
 char *buf, *netaddr, *ptr, *lastnum, *str;
 int col, gateway, netnum, tracert;
 struct IP *network=NULL, *ip;

 for_each(ip, networks)
 {
  printf("%s/%d %s min=%d max=%d sum=%d\n",ip->addr, ip->mask, ip->name, ip->min, ip->max, ip->desired); 
 }

 /*-----------------------------------------------------------------*/
 puts("Analysing network topology ...");
 /*-----------------------------------------------------------------*/
 for_each(ip, networks)
 { 
  printf("%s/%d %s\n",ip->addr, ip->mask, ip->name);
  duplicate(ip->addr, buf);
  lastnum = strrchr(buf, '.');
  if(lastnum)
  {
   gateway = atoi(lastnum + 1) + 1;  /* this is just common rule... */
   *lastnum = 0;
   string(str,STRLEN); 
   sprintf(str, traceroute, buf, gateway);
   shell(str);
   input(str,STRLEN)
   {
    if(    not strstr(str, "traceroute")
       and not strstr(str, "* * *"))
    {
     printf("%s",str);
     duplicate(str, buf);
     valid_columns(ptr, buf, ' ', col)
     if(*ptr=='*')
     {
      col--;
     }
     else if(col==2)
     {
      printf("via [%s]\n", ptr);
      network = find_network_for_ip(ptr);
      if(network)
      {
       network->min += ip->min;
       network->desired += ip->max;
       if(ip->max > network->max)
       {
        network->max = ip->max;
       }
      }      
     }
    }
   }
  }
 }
 sort(network, networks, desc_order_by, min);
 sort(network, networks, desc_order_by, max);
 for_each(ip, networks)
 {
  printf("%s/%d %s min=%d max=%d sum=%d\n",ip->addr, ip->mask, ip->name, ip->min, ip->max, ip->desired); 
 }
 exit(-1);
}
