/* Modified by: xChaos, 20131119 */

#include "cll1-0.6.2.h"
#include "ipstruct.h"
#define STRLEN 512

extern int row_odd_even;
extern int use_jquery_popups;
extern struct IP *ips, *ip, *sharedip;
extern struct Group *groups, *group;
extern struct Keyword *keyword, *defaultkeyword, *keywords;
extern const int highest_priority;
extern const char *version;
extern const char *stats_html_signature;
extern char *jquery_url;
extern int keywordcount;
extern int dry_run;
extern int qos_proxy;
extern char *title;
extern char *log_url;
extern int found_code;
extern char *lms_url;
extern char *log_dir;
extern char *ip6prefix;
extern int traffic_detected;

const char *tr_odd_even(void);
/* implemented in prometheus.c, shared with parselogs.c */

void append_log(struct IP *self) /*using global variables*/
{
 char *d, *str;
 FILE *f; 

 date(d); /* this is typical cll1.h macro - prints current date */ 
 string(str, STRLEN); 
 sprintf(str, "%s/%s.log", log_dir, self->name);

 /*-----------------------------------------------------------------*/
 printf("Writing traffic log %s ...\n", str);
 /*-----------------------------------------------------------------*/
 f = fopen(str, "a");
 if(f > 0)
 {
  fprintf(f, "%ld\t%s\t%Lu\t%Lu\t%Lu\t%Lu\t%d\t%d\t%d\t%s\t%s",
             time(NULL), self->name, self->traffic, self->direct, self->proxy,
             self->upload, self->min, self->max, self->desired, self->code, d); /* d = date*/
  fclose(f);
 }
 else
 {
  perror(str);
 }
}

void write_htmlandlogs(char *html, char *d, int total, int just_preview)
{
 int i;
 char *str;
 FILE *f=fopen(html, "w");

 string(str,STRLEN); 
 if(f > 0)
 {
  int count=1;
  i=0;

  if(use_jquery_popups)
  {
   fprintf(f,"<script type=\"text/javascript\" src=\"%s\"></script>\n", jquery_url);
  }
  fputs("<table class=\"decorated last\">\n\
<caption>Bandwidth classes</caption>\n\
<thead><tr>\n\
<th style=\"text-align: right\">#</th>\n\
<th style=\"text-align: right\">group</th>\n\
<th style=\"text-align: right\">IPs</th>\n\
<th style=\"text-align: right\">requested</th>\n",f);
  fprintf(f,"<th colspan=\"%d\">data limits</th>\n", keywordcount);
  fputs("</tr></thead><tbody>\n",f);

  row_odd_even = 0;
  for_each(group, groups) 
  { 
#ifdef DEBUG
   printf("%d kb/s group: %d bandwidth requested: %d kb/s\n",group->min,group->count,group->desired);
#endif
   fprintf(f, "%s<td style=\"text-align: right\">%d</td><td style=\"text-align: right\">%d&nbsp;kb/s</td>",
              tr_odd_even(), count, group->min);
   fprintf(f, "<td style=\"text-align: right\">%d</td><td style=\"text-align: right\">%d&nbsp;kb/s</td>",
              group->count, group->desired);

   for_each(keyword, keywords) if(keyword->ip_count)
   {
    fprintf(f, "<td style=\"text-align: right\"><span style=\"color:#%s\">%d&nbsp;MB</span></td>",
               keyword->html_color, group->min*keyword->data_limit);
   }   
   i += group->desired; 
   total += group->count;
   count++; 
  }
#ifdef DEBUG
   printf("Total groups: %d Total bandwidth requested: %d kb/s\n",
          count, i, i);
#endif
   fprintf(f,"</tr></tbody>\n\
<thead><tr>\n\
<th colspan=\"2\" style=\"text-align: left\"></td>");
   fprintf(f,"<th style=\"text-align: right\">%d</td><th style=\"text-align: right\">%d kb/s</td>",total,i);

   for_each(keyword, keywords) if(keyword->ip_count)
   {
    fprintf(f,"<th style=\"text-align: right\">%d IPs</th>",keyword->ip_count);
   }
   fprintf(f,"</tr><tr><th colspan=\"4\"></th>\n");
   fprintf(f,"<th colspan=\"%d\">%d traffic classes</th></tr>\n", keywordcount, total);

   fputs("</thead></table>\n",f);
 }
 else
 {
  perror(html);
 }

 i=0;
 if(f > 0)
 {
  unsigned long long total_traffic = 1 /* prevent divide by zero */ , total_direct=0, total_proxy=0, total_upload=0, tmp_sum = 0;
  unsigned long long total_pktup = 0, total_pktdown = 0;
  int active_classes = 0;
//  int colspan = 14;
  struct Sum {unsigned long long l; int i; list(Sum);} *sum,*sums = NULL;
  int agreg_count = 0, limit_count = 0, prio_count = 0;
  int popup_button = 0;
  /* IPv6 vs. IPv4 stats */
  unsigned long long pkts4 = 1, pkts6 = 1 /*prevent divide by zero */, bytes4 = 0, bytes6 = 0;
  int count4 = 0, count6 = 0;
  int mpkts;
  double perc6;

/*
  if(qos_proxy)
  {
   colspan++;
  }
*/  
  if(use_jquery_popups)
  {
    fprintf(f,"<script type=\"text/javascript\">\
function show_section(n) {\
$(\'#sharing_\'+n).show();\
$(\'#download_'+n).show();\
$(\'#pktsdown_\'+n).show();\
$(\'#upload_'+n).show();\
$(\'#pktsup_\'+n).show();\
} </script>");
  }

  fprintf(f,"<p><table class=\"decorated last\">\n<caption>%s",title);
  fprintf(f," (%s)</caption>\n", d);
  fputs("<thead><tr>\n<th colspan=\"3\">&nbsp;</th>\n",f);
  fputs("<th style=\"text-align: right\">cred.</th>\n\
<th style=\"text-align: right\">FUP</th>\n\
<th style=\"text-align: right\">total</th>\n\
<th style=\"text-align: center\" colspan=\"2\">upload</th>\n",f);
/*
  if(qos_proxy)
  {
   fputs("<th style=\"text-align: right\">proxy</th>\n",f);
  }
*/
  fputs("<th style=\"text-align: center\" colspan=\"2\">download</th>\n\
<th style=\"text-align: right\">min</th>\n\
<th style=\"text-align: right\">max</th>\n\
<th style=\"text-align: right\">limit</th>\n\
<th>&nbsp;</th>\n\
</tr><tr>\n\
<th style=\"text-align: right\">#</th>\n\
<th>hostname [+sharing]</th>\n\
<th style=\"text-align: right\">LMS</th>\n\
<th style=\"text-align: right\">MB</th>\n\
<th style=\"text-align: right\">MB</th>\n\
<th style=\"text-align: right\">MB</th>\n\
<th style=\"text-align: right\">MB</th>\n\
<th style=\"text-align: right\">pkt</th>\n\
<th style=\"text-align: right\">MB</th>\n\
<th style=\"text-align: right\">pkt</th>\n\
<th style=\"text-align: right\">kb/s</th>\n\
<th style=\"text-align: right\">kb/s</th>\n\
<th style=\"text-align: right\">kb/s</th>\n\
<th>!</th>\n\
</tr></thead><tbody>\n",f);	

  row_odd_even = 0;
  for_each(ip,ips) if(!use_jquery_popups || !ip->sharing)
  {
   char *f1="", *f2="";
   i++;

   if(ip->aggregated > 1)
   {
    f1 = "<span style=\"color:green\">";
    f2 = "</span>";   
    agreg_count++;
   }
   else if(ip->max < ip->desired) 
   {
    f1 = "<span style=\"color:red\">";
    f2 = "</span>"; 
    limit_count++; 
   }
   else if(ip->prio > highest_priority+1)
   {
    f1 = "<span style=\"color:brown\">";
    f2 = "</span>";
    prio_count++; 
   }       

#ifdef DEBUG
   printf("%03d. %-22s %10Lu (%d/%d)\n",i ,ip->name, ip->traffic, ip->min, ip->max); 
#endif
   /* hostnames -------------------------------------- */
   fprintf(f,"%s<td style=\"text-align: right\"><a name=\"%s\"></a>%d</td>\
<td><a class=\"blue\" target=\"_blank\" href=\"%s%s.log\">%s</a>\n", 
              tr_odd_even(), ip->name, i, log_url, ip->name, ip->name);

   if(use_jquery_popups)
   {
     fprintf(f, "<span id=\"sharing_%d\" style=\"display:none\">",i);
     popup_button=0;

     for_each(sharedip, ips) if(eq(ip->name, sharedip->sharing) && !sharedip->v6) /* IPv4 only */
     {
      fprintf(f, "<br /><a class=\"blue\" target=\"_blank\" href=\"%s%s.log\">%s</a>\n", 
                 log_url, sharedip->name, sharedip->name);
      popup_button++;
     }

     for_each(sharedip, ips) if(eq(ip->name, sharedip->sharing) && sharedip->v6) /* IPv6 only */
     {
      fprintf(f, "<br /><a class=\"blue\" target=\"_blank\" href=\"%s%s.log\">%s/%d</a>\n", 
                 log_url, sharedip->addr, sharedip->addr, sharedip->mask);
      popup_button++;
     }

     fputs("</span>\n",f);
     if(popup_button)
     {
      fprintf(f, "<span>[<a class=\"blue\" href=\"#\" \
onClick=\"$(this).parent().hide();show_section(\'%d\');return(false);\" \
style=\"cursor: pointer;\">+%d</a>]</span>",
                 i, popup_button);
     }
   }
   fputs("</td>\n",f);
   /* ----------------------------------------------- */

   if(found_code)
   {
    fputs("<td style=\"text-align: right\">",f);
    if(ip->code[0])
    {
     fprintf(f, "<a class=\"blue\" target=\"_blank\" href=\"%s%s\">%s</a>\n",
                lms_url, ip->code, ip->code);
    }
    else if(ip->code[0] == 0)
    {
     fputs("------",f);
    }
    fputs("</td>\n",f);
   }
   fprintf(f,"<td style=\"text-align: right\">%Lu</td>\n", ip->credit);
   fprintf(f,"<td style=\"text-align: right\"><span style=\"color:#%s\">%Lu</span></td>",
             ip->keyword->html_color, ip->realquota);
   fprintf(f,"<td style=\"text-align: right\">%s%Lu%s</td>", f1, ip->traffic, f2);

   /* upload --------------------------------------- */
   fprintf(f,"<td style=\"text-align: right\">%Lu", ip->upload);
   if(use_jquery_popups)
   {
     fprintf(f,"<span id=\"upload_%d\" style=\"display:none\">", i);
     for_each(sharedip, ips) if(eq(ip->name, sharedip->sharing) && !sharedip->v6) /* IPv4 only */
     {
      fprintf(f,"<br />%Lu", sharedip->upload);
     }
     for_each(sharedip, ips) if(eq(ip->name, sharedip->sharing) && sharedip->v6) /* IPv6 only */
     {
      fprintf(f,"<br />%Lu", sharedip->upload);
     }
     fputs("</span>\n",f);
   }
   fputs("</td>\n",f);

   /* pkts up ----------------------------------- */
   mpkts = ip->pktsup>>20;
   total_pktup += mpkts;
   if(mpkts == 0)
   {
    mpkts = 1; /* prevent divide by zero*/
   }
   fprintf(f,"<td style=\"text-align: right\"><span style=\"color: gray\">%d</span>", ip->upload/mpkts);
   if(use_jquery_popups)
   {
     fprintf(f,"<span id=\"pktsup_%d\" style=\"display:none\">", i);
     for_each(sharedip, ips) if(eq(ip->name, sharedip->sharing) && !sharedip->v6) /* IPv4 only */
     {
      mpkts = sharedip->pktsup>>20;
      if(mpkts == 0)
      {
       mpkts = 1; /* prevent divide by zero*/
      }   
      fprintf(f,"<br /><span style=\"color: gray\">%d</span>", sharedip->upload/mpkts);
     }
     for_each(sharedip, ips) if(eq(ip->name, sharedip->sharing) && sharedip->v6) /* IPv6 only */
     {
      mpkts = sharedip->pktsup>>20;
      if(mpkts == 0)
      {
       mpkts = 1; /* prevent divide by zero*/
      }   
      fprintf(f,"<br /><span style=\"color: gray\">%d</span>", sharedip->upload/mpkts);
     }
     fputs("</span>\n",f);
   }
   fputs("</td>\n",f);

/*
   if(qos_proxy)
   {
    fprintf(f,"<td style=\"text-align: right\">%Lu</td>\n", ip->proxy);
   }
*/
   /* download ---------------------------------------- */
   fprintf(f,"<td style=\"text-align: right\">%Lu", ip->direct);
   if(use_jquery_popups)
   {
     fprintf(f,"<span id=\"download_%d\" style=\"display:none\">", i);
     for_each(sharedip,ips) if(eq(ip->name, sharedip->sharing) && !sharedip->v6) /* IPv4 only */
     {
      fprintf(f,"<br />%Lu", sharedip->direct);
     }
     for_each(sharedip,ips) if(eq(ip->name, sharedip->sharing) && sharedip->v6) /* IPv6 only */
     {
      fprintf(f,"<br />%Lu", sharedip->direct);
     }
     fputs("</span>\n",f);
   }
   fputs("</td>\n",f);

   /* pkts down ---------------------------------------- */
   mpkts = ip->pktsdown>>20;
   total_pktdown += mpkts;
   if(mpkts == 0)
   {
    mpkts = 1; /* prevent divide by zero*/
   }
   fprintf(f,"<td style=\"text-align: right\"><span style=\"color: gray\">%d</span>", ip->direct/mpkts);
   if(use_jquery_popups)
   {
     fprintf(f,"<span id=\"pktsdown_%d\" style=\"display:none\">", i);
     for_each(sharedip,ips) if(eq(ip->name, sharedip->sharing) && !sharedip->v6) /* IPv4 only */
     {
      mpkts = sharedip->pktsdown>>20;
      if(mpkts == 0)
      {
       mpkts = 1; /* prevent divide by zero*/
      }   
      fprintf(f,"<br /><span style=\"color: gray\">%d</span>", sharedip->direct/mpkts);
     }
     for_each(sharedip,ips) if(eq(ip->name, sharedip->sharing) && sharedip->v6) /* IPv6 only */
     {
      mpkts = sharedip->pktsdown>>20;
      if(mpkts == 0)
      {
       mpkts = 1; /* prevent divide by zero*/
      }   
      fprintf(f,"<br /><span style=\"color: gray\">%d</span>", sharedip->direct/mpkts);
     }
     fputs("</span>\n",f);
   }
   fputs("</td>\n",f);
   /* ----------------------------------------------- */

   fprintf(f, "<td style=\"text-align: right\">%d</td>\n\
<td style=\"text-align: right\">%d</td>\n\
<td style=\"text-align: right\">%s%d%s</td>\n\
<td>%s%d%s</td></tr>\n",
              ip->min, ip->desired, 
              f1, ip->max, f2, 
              f1, ip->prio, f2);

   total_traffic+=ip->traffic;
   total_direct+=ip->direct;
   total_proxy+=ip->proxy;
   total_upload+=ip->upload;
   if(ip->traffic>0)
   {
    active_classes++;
    tmp_sum+=ip->traffic;
    create(sum,Sum);
    sum->l=tmp_sum;
    sum->i=active_classes;
    insert(sum,sums,order_by,i);
   }

   if(!just_preview && traffic_detected)
   {
    append_log(ip);
    for_each(sharedip,ips) if(eq(ip->name, sharedip->sharing))
    {
     append_log(sharedip);
    }
   }
   else
   {
    printf(".");
   }
  }
  fprintf(f, "</tbody><thead><tr>\n\
<th colspan=\"5\" style=\"text-align: left\">%d CLASSES</th>", i);
  fprintf(f, "<th style=\"text-align: right\">%Lu</th><th style=\"text-align: right\">%Lu</th>\
<th style=\"text-align: right\">%d</th>\n",
             total_traffic, total_upload, total_pktup/i);
/*
  if(qos_proxy)
  {
   fprintf(f," <th style=\"text-align: right\">%Lu</th>\n", total_proxy);
  }
*/
  fprintf(f, "<th style=\"text-align: right\">%Lu</th><th style=\"text-align: right\">%d</th>",
             total_direct, total_pktdown/i);
  fprintf(f, "<th colspan=\"6\">\
<span style=\"color:green\">AGR %dx</span> \
<span style=\"color:red\">FUP %dx</span> \
<span style=\"color:brown\">PRIO %dx</span></th></tr>\n</thead></table>\n",
             agreg_count, limit_count, prio_count);

  if(ip6prefix)
  { 
   for_each(ip, ips)
   { 
    if(ip->v6)
    {
     bytes6 += ip->upload + ip->direct;
     pkts6 += ip->pktsdown + ip->pktsup;
     count6++;
    }
    else
    {
     bytes4 += ip->upload + ip->direct;
     pkts4 += ip->pktsdown + ip->pktsup;
     count4++;
    }
   }

   perc6=(double)(bytes6)/(bytes4+bytes6)*100;
   fputs("<p><table class=\"decorated last\"><caption>IP protocols usage</caption>\n",f);
   fprintf(f, "%s<td>Total %d IPv4 addreses</td><td style=\"text-align: right\">%Lu MB (%.2f %%)</td><td style=\"text-align: right\">%Lu packets (%.2f %%)</td></tr>\n",
              tr_odd_even(), count4, bytes4, (double)(bytes4)/(bytes4+bytes6)*100, pkts4, (float)(100*pkts4)/(pkts4+pkts6));
   fprintf(f, "%s<td>Total %d IPv6 /64 ranges</td><td style=\"text-align: right\">%Lu MB (%.2f %%)</td><td style=\"text-align: right\">%Lu packets (%.2f %%)</td></tr>\n",
              tr_odd_even(), count6, bytes6, perc6, pkts6, (float)(100*pkts6)/(pkts4+pkts6));
   fputs("</table></p>\n", f);
  }

  row_odd_even = 0;
  if(active_classes>10)
  {
   int top20_count=0,top20_perc1=0;
   long long top20_perc2=0;
   unsigned long long top20_sum=0l;
  
   fputs("<a name=\"erp\"></a><p><table class=\"decorated last\"><caption>Enterprise Resource Planning (ERP)</caption>\n",f);
   fputs("<thead><tr>\n\
<th>Analytic category</th>\n\
<th colspan=\"2\" style=\"text-align: center\">Active Classes</th>\n\
<th colspan=\"2\" style=\"text-align: center\">Data transfers</th>\n\
</tr></thead><tbody>\n",f);

   if_exists(sum,sums,sum->l >= total_traffic/4)
   {
    fprintf(f,"%s<td>Top 25%% of traffic</td>\n", tr_odd_even());
    fprintf(f,"<td style=\"text-align: right\">%d</td>\n\
<td style=\"text-align: right\">%d %%</td>\n\
<td style=\"text-align: right\">%Lu MB</td>\n\
<td style=\"text-align: right\">%Ld %%</td></tr>\n",
              sum->i, (100*sum->i+50)/active_classes, sum->l, (100*sum->l+50)/total_traffic);
   }
   
   if_exists(sum,sums,sum->i == 10)
   {
    fprintf(f,"%s<td>Top 10 downloaders</td>\n", tr_odd_even());
    fprintf(f,"<td style=\"text-align: right\"><strong>10</strong></td>\n\
<td style=\"text-align: right\">%d %%</td>\n\
<td style=\"text-align: right\">%Lu MB</td>\n\
<td style=\"text-align: right\">%Ld %%</td></tr>\n",
              (100*sum->i+50)/active_classes, sum->l, (100*sum->l+50)/total_traffic);
   }

   if_exists(sum,sums,sum->l >= total_traffic/2)
   {
    fprintf(f,"%s<td>Top 50%% of traffic</td>\n", tr_odd_even());
    fprintf(f,"<td style=\"text-align: right\">%d</td>\n\
<td style=\"text-align: right\">%d %%</td>\n\
<td style=\"text-align: right\">%Lu MB</td>\n\
<td style=\"text-align: right\"><strong>%Ld %%</strong></td></tr>\n",
              sum->i,(100*sum->i+50)/active_classes,sum->l,(100*sum->l+50)/total_traffic);
   }

   if_exists(sum,sums,sum->l >= 4*total_traffic/5)
   {
    fprintf(f,"%s<td>Top 80%% of traffic</td>\n", tr_odd_even());
    fprintf(f,"<td style=\"text-align: right\">%d</td>\n\
<td style=\"text-align: right\">%d %%</td>\n\
<td style=\"text-align: right\">%Lu MB</td>\n\
<td style=\"text-align: right\"><strong>%Ld %%</strong></td></tr>\n",
              sum->i,(100*sum->i+50)/active_classes,sum->l,(100*sum->l+50)/total_traffic);
   }

   if_exists(sum,sums,sum->i >= (active_classes+1)/5)
   {
    fprintf(f,"%s<td>Top 20%% downloaders</td>\n", tr_odd_even());
    top20_count=sum->i;
    top20_perc1=(100*sum->i+50)/active_classes;
    top20_sum=sum->l;
    top20_perc2=(100*sum->l+50)/total_traffic;
    fprintf(f,"<td style=\"text-align: right\">%d</td>\n\
<td style=\"text-align: right\"><strong>%d %%</strong></td>\n\
<td style=\"text-align: right\">%Lu MB</td>\n\
<td style=\"text-align: right\">%Ld %%</td></tr>\n",
              top20_count,top20_perc1,top20_sum,top20_perc2);
   }

   if_exists(sum,sums,sum->i >= (active_classes+1)/4)
   {
    fprintf(f,"%s<td>Top 25%% downloaders</td>\n", tr_odd_even());
    fprintf(f,"<td style=\"text-align: right\">%d</td>\n\
<td style=\"text-align: right\">%d %%</td>\n\
<td style=\"text-align: right\">%Lu MB</td>\n\
<td style=\"text-align: right\">%Ld %%</td></tr>\n",
              sum->i,(100*sum->i+50)/active_classes,sum->l,(100*sum->l+50)/total_traffic);
   }

   if_exists(sum,sums,sum->i>=(active_classes+1)/2)
   {
    fprintf(f,"%s<td>Top 50%% downloaders</td>\n", tr_odd_even());
    fprintf(f,"<td style=\"text-align: right\">%d</td>\n\
<td style=\"text-align: right\"><strong>%d %%</strong></td>\n\
<td style=\"text-align: right\">%Lu MB</td>\n\
<td style=\"text-align: right\">%Ld %%</td></tr>\n",
              sum->i,(100*sum->i+50)/active_classes,sum->l,(100*sum->l+50)/total_traffic);
   }

   if_exists(sum,sums,sum->i >= 4*(active_classes+1)/5)
   {
    fprintf(f,"%s<td>Top 80%% downloaders</td>\n", tr_odd_even());
    fprintf(f,"<td style=\"text-align: right\">%d</td>\n\
<td style=\"text-align: right\">%d %%</td>\n\
<td style=\"text-align: right\">%Lu MB</td>\n\
<td style=\"text-align: right\">%Ld %%</td></tr></tbody>\n",
              sum->i,(100*sum->i+50)/active_classes,sum->l,(100*sum->l+50)/total_traffic);
   }

   fprintf(f,"<thead><tr><th><a class=\"blue\" target=\"_blank\" href=\"%sERP.log\">All users, all traffic</a></th>\n", log_url);
   fprintf(f,"<th style=\"text-align: right\">%d</th>\n\
<th style=\"text-align: right\">100 %%</th>\n\
<th style=\"text-align: right\">%Lu MB</th>\n\
<th style=\"text-align: right\">100 %%</th></tr>\n",active_classes,total_traffic);
   fputs("</thead></table></p>\n", f);

   /* write basic ERP data to log directory */
   if(!just_preview)
   {
    FILE *iplog;
    sprintf(str,"%s/ERP.log",log_dir);
    iplog=fopen(str,"a");
    if(iplog)
    {
     fprintf(iplog, "%ld\t%d\t%d %%\t%Lu M\t%Ld %%\tACTIVE %d\tTRAFFIC %Lu M\tCLASSES %d\tFUP-LIMIT %d\tLOW-PRIO %d\tIPv6 %Lu M\t%.2f %%\t%s",
                    time(NULL), top20_count, top20_perc1, top20_sum, top20_perc2, 
                    active_classes, total_traffic, i, limit_count, prio_count,
                    bytes6, perc6, d); /* d = date*/
     fclose(iplog);
    }
    else
    {
     perror(str);
    }
   }
  }

  fprintf(f, stats_html_signature, version);
  fclose(f);
 }
}