#include "cll1-0.6.2.h"
#define STRLEN 512

/* globals declared in prometheus.c */
extern char *log_dir;
extern const char *version;
extern const char *stats_html_signature;
extern const char *ls;
extern char *html_log_dir;
extern int row_odd_even;
extern char *log_url;
extern char *lms_url;
extern long long int line;

/* function implemented in prometheus.c */
const char *tr_odd_even(void);

struct IpLog
{
 char *name;
 long traffic;
 long guaranted;
 int i;
 int lmsid;
 long l;
 list(IpLog);
} *iplog,*iplogs;

void parse_ip_log(int argc, char **argv) 
{
 char *month, *year, *str, *name="(undefined)", *ptr, *ptr2, *filename;
 long traffic=0l, traffic_month, total=0, guaranted;
 int col, col2, y_ok, m_ok, accept_month, i=1, any_month=0, lmsid;
 char mstr[4], ystr[5];
 FILE *f; 
 string(str,STRLEN);
 string(filename,STRLEN);

 if(argv[1][1]=='l') /* -l */
 {
  if(argc<4)
  {
   puts("Missing parameter(s)!\nUsage: prometheus -l Mmm YYYY (Mmm=Jan-Dec or Year, YYYY=year)");
   exit(-1);
  }
  else
  {
   month=argv[2];
   if(eq(month,"Year")) any_month=1;
   year=argv[3];
  }
 }
 else
 { 
  time_t t = time(NULL) - 3600*24 ; /* yesterday's timestamp*/
  struct tm *timep = localtime(&t);                                           

  if(argv[1][1]=='m') /* -m yestarday - month */
  {
   strftime(mstr, 4, "%b", timep);
   month=mstr;
   strftime(ystr, 5, "%Y", timep);
   year=ystr; 
  }
  else /* -y yesterday - year */
  {
   month="Year";
   any_month=1;
   strftime(ystr, 5, "%Y", timep);
   year=ystr;
  }
 }
 printf("Analysing traffic for %s %s ...\n",month,year);

 /* sorry... next release of C<<1 header file will include for_path_files(name,path) {  } macro */
 sprintf(str,"%s %s/",ls,log_dir);
 shell(str);
 input(str,STRLEN) 
 {
  if(strstr(str,".log"))
  {
    ptr=strrchr(str,'\n');
    if(ptr) *ptr='\0';
    sprintf(filename,"%s/%s",log_dir,str);
    printf("Parsing %s ...",filename);
    accept_month=0;
    traffic_month=0;
    guaranted=0;
    lmsid=-1;
    parse(filename)
    {
     y_ok=m_ok=0;  
     valid_columns(ptr,_,'\t',col) switch(col)
     {
      case 2: name = ptr;break;
      case 3: traffic = atol(ptr);break;
      /* column number   - was 7, now 11...*/
      case 7:
      case 8:
      case 9:
      case 10:
      case 11: if(isalpha(*ptr)) /* character, not numeric string = date, just one*/
               {
                valid_columns(ptr2,ptr,' ',col2) switch(col2)
                {
                 case 2: if(any_month || eq(ptr2,month)) m_ok = 1; break;
                 case 5: if(eq(ptr2,year)) y_ok = 1; break;
                }
               }
               else
               {
                 if(col == 7) guaranted = atol(ptr);
                 if(col == 10) lmsid = atoi(ptr);
               }
     }
     
     if(y_ok && m_ok) 
     {
      traffic_month += traffic;
      accept_month = 1;
     }
    }
    done; /* ugly macro end */ 

    if(accept_month)
    {
     create(iplog,IpLog);
     iplog->name = name;
     iplog->guaranted = guaranted;
     iplog->traffic = traffic_month;
     iplog->lmsid = lmsid;
     insert(iplog,iplogs,desc_order_by,traffic);
     printf(" %ld MB\n",iplog->traffic);
    }
    else
    {
     puts(" no records.");
    }
  }
 }
 sprintf(str,"%s/%s-%s.html",html_log_dir,year,month);
 printf("Writing %s ... ",str);
 f=fopen(str,"w");
 if(f > 0)
 {
  fprintf(f, "<table class=\"decorated last\"><thead>\n\
<tr><th colspan=\"2\">%s %s</th>\n\
<th style=\"text-align: right\">lms</th>\n\
<th colspan=\"2\">Data transfers</th>\n\
<th style=\"text-align: right\">Min.speed</th>\n\
</tr></thead><tbody>\n ",
             month, year);

  row_odd_even = 0;
  for_each(iplog, iplogs)
  {
   if(iplog->traffic)
   {
    fprintf(f, "%s<td style=\"text-align: right\">%d</td>\n\
<td style=\"text-align: left\"><a class=\"blue\" target=\"_blank\" href=\"%s%s.log\">%s</td>\n\
<td style=\"text-align: right\">", 
               tr_odd_even(), i++, log_url, iplog->name, iplog->name);  
    if(iplog->lmsid > 0)
    {
     /*base URL will be configurable soon ... */
     fprintf(f, "<a class=\"blue\" target=\"_blank\" href=\"%s%d\">%04d</a>\n", lms_url, iplog->lmsid, iplog->lmsid);
    }
    else if(iplog->lmsid == 0)
    {
     fputs("-------",f);
    }    
    fprintf(f, "<td style=\"text-align: right\">%ld&nbsp;MB</td>\n\
    <td style=\"text-align: right\"><strong>%ld&nbsp;GB</strong></td>\n\
    <td style=\"text-align: right\">%ld&nbsp;kb/s</th></tr>\n",
               iplog->traffic, iplog->traffic>>10, iplog->guaranted);
    total+=iplog->traffic>>10;
    iplog->i=i;
    iplog->l=total;
   }
  }
  fprintf(f,"</tbody><thead><tr>\
  <th colspan=\"3\" style=\"text-align: left\">Total:</th>\
  <th colspan=\"2\" style=\"text-align: right\"><strong>%ld&nbsp;GB</strong></th>\
  <th style=\"text-align: right\"><strong>%Ld&nbsp;kb/s</strong></th></tr>\n", total, line);
  fputs("</thead></table>\n", f);

  row_odd_even = 0;
  if(i>10)
  {
   fputs("<a name=\"erp\"></a><p><table class=\"decorated last\">\n\
<caption>Enterprise Resource Planning (ERP)</caption>\n\
<thead><tr>\n\
<th>Analytic category</th>\n\
<th colspan=\"2\" style=\"text-align: center\">Active Classes</th>\n\
<th colspan=\"2\" style=\"text-align: center\">Data transfers</th>\n\
</tr></thead><tbody>\n",f);

   if_exists(iplog,iplogs,iplog->l>=total/4)
   {
    fprintf(f,"%s<td>Top 25%% of traffic</td>\n", tr_odd_even());
    fprintf(f,"<td style=\"text-align: right\">%d</td>\n\
<td style=\"text-align: right\">%d %%</td>\n\
<td style=\"text-align: right\">%ld GB</td>\n\
<td style=\"text-align: right\">%d %%</td></tr>\n",
              iplog->i, (100*iplog->i+50)/i, iplog->l, (int)((100*iplog->l+50)/total));
   }
   
   if_exists(iplog,iplogs,iplog->i==10)
   {
    fprintf(f,"%s<td>Top 10 downloaders</td>\n", tr_odd_even());
    fprintf(f,"<td style=\"text-align: right\"><strong>10</strong></td>\n\
<td style=\"text-align: right\">%d %%</td>\n\
<td style=\"text-align: right\">%ld GB</td>\n\
<td style=\"text-align: right\">%d %%</td></tr>\n",
               (100*iplog->i+50)/i, iplog->l, (int)((100*iplog->l+50)/total));
   }

   if_exists(iplog,iplogs,iplog->l>=total/2)
   {
    fprintf(f,"%s<td>Top 50%% of traffic</td>\n", tr_odd_even());
    fprintf(f,"<td style=\"text-align: right\">%d</td>\n\
<td style=\"text-align: right\">%d %%</td>\n\
<td style=\"text-align: right\">%ld GB</td>\n\
<td style=\"text-align: right\"><strong>%d %%</strong></td></tr>\n",
              iplog->i,(100*iplog->i+50)/i,iplog->l,(int)((100*iplog->l+50)/total));
   }

   if_exists(iplog,iplogs,iplog->l>=4*total/5)
   {
    fprintf(f,"%s<td>Top 80%% of traffic</td>\n",tr_odd_even());
    fprintf(f,"<td style=\"text-align: right\">%d</td>\n\
<td style=\"text-align: right\">%d %%</td>\n\
<td style=\"text-align: right\">%ld GB</td>\n\
<td style=\"text-align: right\"><strong>%d %%</strong></td></tr>\n",
              iplog->i, (100*iplog->i+50)/i, iplog->l, (int)((100*iplog->l+50)/total));
   }

   if_exists (iplog,iplogs,iplog->i>=i/5)
   {
    fprintf(f,"%s<td>Top 20%% downloaders</td>\n",tr_odd_even());
    fprintf(f,"<td style=\"text-align: right\">%d</td>\n\
<td style=\"text-align: right\"><strong>%d %%</strong></td>\n\
<td style=\"text-align: right\">%ld GB</td>\n\
<td style=\"text-align: right\">%d %%</td></tr>\n",
              iplog->i, (100*iplog->i+50)/i, iplog->l, (int)((100*iplog->l+50)/total));
   }

   if_exists(iplog,iplogs,iplog->i>=i/4)
   {
    fprintf(f,"%s<td>Top 25%% downloaders</td>\n", tr_odd_even());
    fprintf(f,"<td style=\"text-align: right\">%d</td>\n\
<td style=\"text-align: right\">%d %%</td>\n\
<td style=\"text-align: right\">%ld GB</td>\n\
<td style=\"text-align: right\">%d %%</td></tr>\n",
               iplog->i, (100*iplog->i+50)/i, iplog->l, (int)((100*iplog->l+50)/total));
   }

   if_exists(iplog,iplogs,iplog->i>=i/2)
   {
    fprintf(f,"%s<td>Top 50%% downloaders</td>\n",tr_odd_even());
    fprintf(f,"<td style=\"text-align: right\">%d</td>\n\
<td style=\"text-align: right\"><strong>%d %%</strong></td>\n\
<td style=\"text-align: right\">%ld GB</td>\n\
<td style=\"text-align: right\">%d %%</td></tr>\n",
              iplog->i, (100*iplog->i+50)/i, iplog->l, (int)((100*iplog->l+50)/total));
   }

   if_exists(iplog,iplogs,iplog->i>=4*i/5)
   {
    fprintf(f,"%s<td>Top 80%% downloaders</td>\n",tr_odd_even());
    fprintf(f,"<td style=\"text-align: right\">%d</td>\n\
<td style=\"text-align: right\">%d %%</td>\n\
<td style=\"text-align: right\">%ld GB</td>\n\
<td style=\"text-align: right\">%d %%</td></tr>\n",
              iplog->i, (100*iplog->i+50)/i, iplog->l, (int)((100*iplog->l+50)/total));
   }

   fprintf(f,"</tbody><thead><tr><th><a class=\"blue\" target=\"_blank\" href=\"%sERP.log\">All users, all traffic</a></th>\n", log_url);
   fprintf(f,"<th style=\"text-align: right\">%d</th>\n\
<th style=\"text-align: right\">100 %%</th>\n\
<th style=\"text-align: right\">%ld GB</th>\n\
<th style=\"text-align: right\">100 %%</th></tr>\n",i-1,total);
   fputs("</thead></table>\n", f);
  }

  fprintf(f, stats_html_signature, version);
  fclose(f);
  puts("done.");
 }
 else
 {
  perror(str);
 }
}
