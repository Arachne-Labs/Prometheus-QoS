/* Modified by: xChaos, 20121011 */

#include "cll1-0.6.2.h"
#include "ipstruct.h"

extern struct IP *ips, *ip;

void write_json_traffic(char *json)
{
  FILE *f=fopen(json, "w");
  if(f > 0)
  {
   int jsoncount=0;
   fprintf(f, "{\n");

   for_each(ip, ips)
   {
    if(      ip->lmsid > 0 
        and (ip->traffic or ip->direct or ip->proxy or ip->upload))
    {
     if(jsoncount)
     {
      fprintf(f, ",\n");
     }
     fprintf(f, 
             " \"%s\":{ \"lms\": %d, \"ip\":\"%s\", \"total\":%Lu, \"down\":%Lu, \
             \"proxy\":%Lu, \"up\":%Lu, \"min\":%d, \"max\":%d, \"limit\":%d, \
             \"pktsup\":%Lu, \"pktsdown\":%Lu, \"realquota\":%Lu, \"credit\":%Lu, \"dailyquota\":%ld }",
             ip->name, ip->lmsid, ip->addr, ip->traffic, ip->direct, ip->proxy, 
             ip->upload, ip->min, ip->desired, ip->max, ip->pktsup, ip->pktsdown,
             ip->realquota, ip->credit, (ip->min*ip->keyword->data_limit+(ip->keyword->fixed_limit<<20)));
     jsoncount++;
    }
   }
   fprintf(f, "}\n");
   fclose(f);
   puts("done.");
  }
  else
  {
   perror(json);
  }
}