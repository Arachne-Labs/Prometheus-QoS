/* Modified by: xChaos, 20131029 */

#include "cll1-0.6.2.h"
#include "ipstruct.h"

#define STRLEN 512

extern struct IP *ip, *ips, *networks;

struct IP *locate_network(char *look_for)
{
 struct IP *network;
 char *netaddr, *lastnum, *ipaddr;
 int ipnum, netnum, total;

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
  if(lastnum or total)
  {
   netnum = atoi(lastnum + 1);
   *lastnum = 0;
   if(    eq(netaddr, ipaddr)
      and netnum + (1<<(32-network->mask)) > ipnum
      and netnum <= ipnum)
   {
    return network;
   }       
  }
 }
 return NULL;
}


void update_network_direct(struct IP *network, struct IP *ip)
{
 if(ip and network)
 {
  network->group += 1;
  network->min += ip->min;
  network->direct += ip->max>>10; /* sum of Mbps, not kbps */

  if(ip->max > network->max)
  {
   network->max = ip->max;
  }

  if(network->max > network->min)
  {
   network->desired = network->max>>10;
  }
  else
  {
   network->desired = network->min>>10;
  }
 }
}


void update_network(char *look_for, struct IP* ip)
{
 update_network_direct(locate_network(look_for),ip);
}


void analyse_topology(char *traceroute)
{
 char *buf, *netaddr, *ptr, *lastnum, *str, *redundant;
 int col, gateway, netnum, tracert, distance;
 struct IP *network = NULL, *uplink = NULL;

 string(redundant, STRLEN);
 string(str, STRLEN); 

 /*-----------------------------------------------------------------*/
 puts("Analysing network topology ...");
 /*-----------------------------------------------------------------*/
 for_each(ip, networks) if(ip->group) /* just non-empty networks */
 { 
  printf("%s/%d %s ", ip->addr, ip->mask, ip->name);
  duplicate(ip->addr, buf);
  lastnum = strrchr(buf, '.');
  if(lastnum)
  {
   gateway = atoi(lastnum + 1) + 1;  /* this is just common rule... */
   *lastnum = 0;
   sprintf(str, traceroute, buf, gateway);
   shell(str);
   *redundant = 0;
   distance = 1;
   uplink = NULL;
   input(str,STRLEN)
   {
    if(not strstr(str, "traceroute")) /*skip header */
    {
#ifdef DEBUG
     printf("%s",str);
#endif
     duplicate(str, buf);
     valid_columns(ptr, buf, ' ', col)
     if(*ptr == '*')
     {
      col -= 1;
     }
     else if(col==2 and not strstr(redundant, ptr))
     {
#ifdef DEBUG
      printf("[%s] ", ptr);
#endif
      network = locate_network(ptr);
      if(network)
      {
       printf("[%s/%d] ", network->addr, network->mask);
       network->mark = distance;
       distance += 1;
       if(uplink)
       {
        network->uplink = uplink;
#ifdef DEBUG
        printf("[%d: uplink of %s/%d is %s/%d] ",
               network->mark, network->addr, network->mask, network->uplink->addr, network->uplink->mask);
#endif
       }
       uplink = network;
      }

      if(strlen(redundant) < STRLEN - 17)
      {
       strcat(redundant, ptr);
       strcat(redundant, " ");
      }
     }
    }
   }//end input loop

//   ip->mark = distance;
//   if(uplink)
//   {
//    ip->uplink = uplink;
//   }
   if(distance == 1)
   {
    printf("fail! \n");
   }
   else
   {
    printf("done. \n");
   }
  }
 }//end for_each()

 TheIP("0.0.0.0", TRUE);
 ip->name = "TOTAL";
 ip->mask = 0;

 sort(network, networks, desc_order_by, mark); /* most distant networks first */
 for_each(network, networks)
 {
  if(not network->uplink)
  {
   network->uplink = ip; /* global checksum */  
  }
  printf("(%d) uplink of %s/%d is %s/%d \n",
         network->mark, network->addr, network->mask, network->uplink->addr, network->uplink->mask);
  update_network_direct(network->uplink, network);
 }

 sort(network, networks, desc_order_by, min); /* secondary ordering  */
 sort(network, networks, desc_order_by, desired); /* primary ordering  */

 /*-----------------------------------------------------------------*/
 puts("\nRequested network parameters are:");
 /*-----------------------------------------------------------------*/
 for_each(ip, networks) if(ip->desired)
 {
  printf("%s/%d %s REQUESTED=%dM (classes=%d, sum_min=%dk, max_1=%dk, sum_max=%LuM, agreg.=1:%d)\n",
         ip->addr, ip->mask, ip->name, ip->desired, ip->group, ip->min, ip->max, ip->direct,
         (int)(ip->direct/(float)(ip->desired)+.5));
 }
 exit(-1);
}
