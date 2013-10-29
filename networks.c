/* Modified by: xChaos, 20131029 */

#include "cll1-0.6.2.h"
#include "ipstruct.h"

#define STRLEN 512

extern struct IP *ips, *networks;

void update_network(char *look_for, struct IP* ip)
{
 struct IP *network;
 char *netaddr, *lastnum, *ipaddr;
 int ipnum, netnum;

 duplicate(look_for, ipaddr);
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
    network->group += 1;
    network->min += ip->min;
    network->direct += ip->max<<10; /* sum of Mbps, not kbps*/

    if(ip->max > network->max)
    {
     network->max = ip->max;
    }

    if(network->max > network->min)
    {
     network->desired = network->max;
    }
    else
    {
     network->desired = network->min;
    }
    return;
   }       
  }
 }
}

void analyse_topology(char *traceroute)
{
 char *buf, *netaddr, *ptr, *lastnum, *str;
 int col, gateway, netnum, tracert;
 struct IP *network=NULL, *ip;

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
//#ifdef DEBUG
      printf("via [%s]\n", ptr);
//#endif
      update_network(ptr, ip);
     }
    }
   }
  }
 }
 sort(network, networks, desc_order_by, min);
 sort(network, networks, desc_order_by, desired);

 /*-----------------------------------------------------------------*/
 puts("Requested network parameters are:");
 /*-----------------------------------------------------------------*/
 for_each(ip, networks) if(ip->desired>>10 > 0)
 {
  printf("%s/%d %s REQUESTED=%dM (classes=%d, sum_min=%dk, max_1=%dk, sum_max=%LuM, agreg.=1:%d)\n",
         ip->addr, ip->mask, ip->name, ip->desired>>10, ip->group, ip->min, ip->max, ip->direct,
         (int)((float)(ip->direct)/(ip->desired>>10)));
 }
 exit(-1);
}
