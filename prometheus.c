/* =============================================================== */
/* ==    Prometheus QoS - you can "steal fire" from your ISP    == */
/* ==     "fair-per-IP" quality of service (QoS) utility        == */
/* ==     requires Linux 2.4.x or 2.6.x with HTB support        == */
/* ==     Copyright(C) 2005-2008 Michael Polak (xChaos)         == */
/* ==   iptables-restore support Copyright(C) 2007-2008 ludva   == */
/* == Credit: CZFree.Net,Martin Devera,Netdave,Aquarius,Gandalf == */
/* =============================================================== */

/* Modified: xChaos, 20080202
             ludva, 20071227

   Prometheus QoS is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as 
   published by the Free Software Foundation; either version 2.1 of 
   the License, or (at your option) any later version.

   Prometheus QoS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Prometheus Qos; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA 
   
   GNU General Public License is located in file COPYING */

#define STRLEN 256
#define FIRSTGROUPID 1024
#define FIRSTIPCLASS 2048
#undef DEBUG

#include "cll1-0.6.h"

const char *version="0.7.8"; /*0.7.9 will be last development, 0.8.0 first stable */

/* ======= All path names are defined hear (for RPM patch) =======  */

char *tc              = "/sbin/tc"; /* requires tc with HTB support */
char *iptables        = "/sbin/iptables"; /* requires iptables utility */
char *iptablessave    = "/sbin/iptables-save"; /* not yet required */
char *iptablesrestore = "/sbin/iptables-restore";  /* requires iptables-restore */

char *config          = "/etc/prometheus/prometheus.conf"; /* main configuration file */
char *hosts           = "/etc/prometheus/hosts"; /* per-IP bandwidth definition file */

char *iptablesfile    = "/var/spool/prometheus.iptables"; /* temporary file for iptables-restore*/
char *credit          = "/var/lib/misc/prometheus.credit"; /* credit log file */
char *html            = "/var/www/traffic.html"; /* hall of fame filename */
char *preview         = "/var/www/preview.html"; /* hall of fame preview */
char *cmdlog          = "/var/log/prometheuslog"; /* command log filename */
char *log_dir         = "/var/www/logs/"; /* log directory pathname, ended with slash */
char *log_url         = "logs/"; /* log directory relative URI prefix (partial URL) */

/* ======= Help screen is hopefuly self-documenting part of code :-) ======= */

void help(void)
{
 puts("Command line switches:\n\
\n\
-?, --help    this help screen\n\
-v, --version show version number of this utility and exit\n\
-c filename   force alternative /etc/prometheus.conf filename\n\
-h filename   force alternative /etc/hosts filename (overrides hosts keyword)\n\
-f            just flush iptables and tc classes and exit (stop shaping)\n\
-9            emergency iptables flush (do not read data transfer statistics)\n\
-p            just generate preview of data transfer statistics and exit\n\
-n            no delay (overrides qos-free-delay keyword)\n\
-d            dry run (preview tc and iptables commands on stdout)\n\
");
}

/* === Configuraration file values defaults - stored in global variables ==== */

int filter_type=1;                      /*1 mark, 2 classify*/
char *mark="MARK";
char *mark_iptables="MARK --set-mark ";
int dry_run=0;                         /* preview - use puts() instead of system() */
char *iptablespreamble="*mangle\n:PREROUTING ACCEPT [0:0]\n:POSTROUTING ACCEPT [0:0]\n:INPUT ACCEPT [0:0]\n:OUTPUT ACCEPT [0:0]\n:FORWARD ACCEPT [0:0]";
FILE *iptables_file=NULL;
int enable_credit=1;                   /* enable credit file */
int use_credit=0;                      /* use credit file (if enabled)*/
char *title="Hall of Fame - Greatest Suckers"; /* hall of fame title */
int hall_of_fame=1;		               /* enable hall of fame */
char *lan="eth0";                /* LAN interface */
char *lan_medium="100Mbit";      /* 10Mbit/100Mbit ethernet */
char *wan="eth1";                /* WAN/ISP interface */
char *wan_medium="100Mbit";	 /* 10Mbit/100Mbit ethernet */
char *qos_leaf="sfq perturb 5";  /* leaf discipline */
char *qos_free_zone=NULL;      	 /* QoS free zone */
int qos_proxy=1;	         /* include proxy port to QoS */
int include_upload=1;	         /* upload+download=total traffic */
char *proxy_ip="192.168.1.1/32"; /* our IP with proxy port */
int proxy_port=3128;	  /* proxy port number */
long long int line=1024;  /* WAN/ISP download in kbps */
long long int up=1024;    /* WAN/ISP upload in kbps */
int free_min=32;          /* minimum guaranted bandwidth for all undefined hosts */
int free_max=64;          /* maximum allowed bandwidth for all undefined hosts */
int qos_free_delay=0;	  /* seconds to sleep before applying new QoS rules */
int digital_divide=2;     /* controls digital divide weirdness ratio, 1...3 */ 
int max_nesting=3;	  /* maximum nesting of HTB clases, built-in maximum seems to be 4 */
int htb_r2q=1;      
int burst=8;		  /* HTB burst (in kbits) */
int burst_main=64;
int burst_group=32;
int magic_priorities=8;	  /* number of priority levels (soft shaping) */
int magic_treshold=8;     /* reduce ceil by X*magic_treshhold kbps (hard shaping) */
int keywordcount=0;

/* not yet implemented:
 int fixed_packets=0;	maximum number of pps per IP address (not class!) 
 int packet_limit=5;   maximum number of pps to htn CEIL, not rate !!! 
*/
FILE *log_file=NULL;

char *kwd="via-prometheus";           /* /etc/hosts comment, eg. #qos-64-128 */

const int idxtable_treshold1=24;      /* this is no longer configurable */
const int idxtable_treshold2=12;      /* this is no longer configurable */
const int idxtable_bitmask1=3;        /* this is no longer configurable */
const int idxtable_bitmask2=3;        /* this is no longer configurable */


/* not yet implemented:
-s            start shaping! (keep data transfer statistics - but apply shaping)\n\
-r            just reload configuration (...and keep data transfer statistics)\n\
*/

/* ==== This is C<<1 stuff - learn C<<1 first! http://cll1.arachne.cz ==== */

struct IP
{
 char *addr;
 char *name;
 char *sharing;
 int min;
 int desired;
 int max;
 int mark;
 int prio;
 int fixedprio;
 int group;
 unsigned long long direct;
 unsigned long long proxy;
 unsigned long long upload;
 unsigned long long traffic;
 unsigned long long credit;
 unsigned long pktsup;
 unsigned long pktsdown;
 struct Keyword *keyword;
 list(IP);
} *ips=NULL, *ip, *sharedip;

struct Group
{
 int min;
 int count;
 int desired;
 int id;
 list(Group);
} *groups=NULL, *group;

struct Index
{
 char *addr;
 char *id;
 struct Index *parent;
 int bitmask;
 int children;
 list(Index);
} *idxs=NULL, *idx, *metaindex;

struct Keyword
{
 char *key;
 
 int asymetry_ratio;        /* ratio for ADSL-like upload */
 int asymetry_fixed;        /* fixed treshold for ADSL-like upload */
 int data_limit;            /* hard shaping: apply magic_treshold if max*data_limit MB exceeded */
 int data_prio;            /* soft shaping (qos): reduce HTB prio if max*data_prio MB exceeded */
 long fixed_limit;          /* fixed data limit for setting lower HTB ceil */
 long fixed_prio;           /* fixed data lmit for setting lower HTB prio */
 int reserve_min;	    /* bonus for nominal HTB rate bandwidth (in kbps) */
 int reserve_max;	    /* malus for nominal HTB ceil (in kbps) */
// int divide_max;	    /* relative malus: new_ceil=rate+(old_ceil-rate)/divide_max */
// int htb_ceil_bonus_divide; /* relative bonus: new_ceil=old_ceil+old_ceil/htb_ceil_bonus_divide */
 int default_prio;	    /* default HTB priority for this keyword */
 char *html_color;
 int ip_count;
 char *leaf_discipline;
 
 list(Keyword);
} *keyword,*defaultkeyword=NULL,*keywords=NULL;

/* Damned, this must be object oriented! This looks almost like constructor ;-) */

void TheIP(void)
{
 create(ip,IP);
 ip->name="";
 ip->addr="";
 ip->sharing=NULL;
 ip->prio=1;
 ip->fixedprio=0;
 ip->mark=ip->min=ip->max=ip->desired=ip->credit=0;
 ip->upload=ip->proxy=ip->direct=ip->traffic=0;
 ip->pktsup=ip->pktsdown=0;
 ip->keyword=keywords;
 push(ip,ips);
}

/* ====== Iptables indexes are used to reduce complexity to log8(N) ===== */

char *very_ugly_ipv4_code(char *inip,int bitmask,int format_as_chainname)
{
 /* warning: this function was debugged only for bitmask values 20,24,28 !!!*/
 int dot=0,n;
 char *ip,*outip,*outptr,*fmt;

 duplicate(inip,ip);
 /* debug printf("(%s,%d) -> ",ip,bitmask); */

 if(ip && *ip && bitmask>=0 && bitmask<=32)
  string(outip,strlen(ip)+10); /*fuck unicode? assertion: 10>strlen("_%d_%d") */
 else 
  /* should never exit here */
  return "undefined";
 outptr=outip;
 while(ip && *ip)
 {
  if(*ip=='.')
  {
   if(dot<(bitmask/8-1)) 
   {
    if(format_as_chainname)
     *outptr='_';
    else
     *outptr='.';
    outptr++;
    dot++;
   }
   else
   {
    char *cutdot=strchr(ip+1,'.'); /*for bitmask<24*/
    if(cutdot)*cutdot='\0';
    if(format_as_chainname)
     fmt="_%d_%d";
    else
     fmt=".%d";
    if(bitmask%8)
     n=atoi(ip+1)-atoi(ip+1)%(1<<(8-bitmask%8));
    else
     n=0;

    /*debug  printf("%d/%d => [_%d_%d]\n",atoi(ip+1),bitmask,n,bitmask); */
    sprintf(outptr,fmt,n,bitmask);
    if(!format_as_chainname) while(bitmask<24)
    {
     strcat(outip,".0");
     bitmask+=8;
    }
    /* debug printf("[%s]\n",outip); */
    return outip;
   }
  }
  else 
  {
   *outptr=*ip;
   outptr++;
  }
  ip++;
 }
 /*should never exit here*/
 *outptr='\0';
 return outip;
}

char *hash_id(char *ip,int bitmask)
{ return very_ugly_ipv4_code(ip,bitmask,1); }

char *subnet_id(char *ip,int bitmask)
{ return very_ugly_ipv4_code(ip,bitmask,0); }

/* ================= Let's parse configuration file here =================== */

void reject_config_and_exit(char *filename)
{
 printf("Configuration file %s rejected - abnormal exit.",filename);
 exit(-1);
}

void get_config(char *config_filename)
{
 char *cnf="mark";
 
 printf("Configured keywords: ");
 parse(config_filename)
 {
  option("keyword",kwd);
  if(kwd)
  {
   printf("%s ",kwd);

   create(keyword,Keyword);
   keyword->key=kwd;
   keyword->asymetry_ratio=1;          /* ratio for ADSL-like upload */
   keyword->asymetry_fixed=0;          /* fixed treshold for ADSL-like upload */
   keyword->data_limit=8;              /* hard shaping: apply magic_treshold if max*data_limit MB exceeded */
   keyword->data_prio=4;              /* soft shaping (qos): reduce HTB prio if max*data_prio MB exceeded */
   keyword->fixed_limit=0;             /* fixed data limit for setting lower HTB ceil */
   keyword->fixed_prio=0;              /* fixed data limit for setting lower HTB prio */
   keyword->reserve_min=8;	       /* bonus for nominal HTB rate bandwidth (in kbps) */
   keyword->reserve_max=0;	       /* malus for nominal HTB ceil (in kbps) */
/* obsolete:
   keyword->divide_max=0;	       relative malus: new_ceil=rate+(old_ceil-rate)/divide_max
   keyword->htb_ceil_bonus_divide=0;   relative bonus: new_ceil=old_ceil+old_ceil/htb_ceil_bonus_divide
*/
   keyword->default_prio=1;
   keyword->html_color="000000";
   keyword->ip_count=0;
   keyword->leaf_discipline="";

   push(keyword,keywords);
   if(!defaultkeyword) defaultkeyword=keyword;
   keywordcount++;
   
   kwd=NULL;
  }
  else every(keyword,keywords)
  {
   int l=strlen(keyword->key);


   if(!strncmp(keyword->key,_,l) && strlen(_)>l+2)
   {
    char *tmptr=_; /*  <---- l+1 ----> */
    _+=l+1;        /*  via-prometheus-asymetry-ratio, etc. */
    ioption("asymetry-ratio",keyword->asymetry_ratio);
    ioption("asymetry-treshold",keyword->asymetry_fixed);
    ioption("magic-relative-limit",keyword->data_limit);
    ioption("magic-relative-prio",keyword->data_prio);
    loption("magic-fixed-limit",keyword->fixed_limit);
    loption("magic-fixed-prio",keyword->fixed_prio);
    ioption("htb-default-prio",keyword->default_prio);
    ioption("htb-rate-bonus",keyword->reserve_min);
    ioption("htb-ceil-malus",keyword->reserve_max);
/* obsolete:
    ioption("htb-ceil-divide",keyword->divide_max);
    ioption("htb-ceil-bonus-divide",keyword->htb_ceil_bonus_divide);
*/
    option("leaf-discipline",keyword->leaf_discipline);
    option("html-color",keyword->html_color);
    _=tmptr;
    
    if(keyword->data_limit || keyword->fixed_limit || 
       keyword->data_prio || keyword->fixed_prio)
        use_credit=1;
        

   }
  }

  option("tc",tc);
  option("iptables",iptables);
  option("iptables-save",iptablessave); /* new */
  option("iptables-restore",iptablesrestore); /* new */
  option("iptables-file",iptablesfile); /* new */
  option("hosts",hosts);
  option("lan-interface",lan);
  option("wan-interface",wan);
  option("lan-medium",lan_medium);
  option("wan-medium",wan_medium);
  lloption("wan-download",line);
  lloption("wan-upload",up);
  ioption("hall-of-fame-enable",hall_of_fame);
  option("hall-of-fame-title",title);
  option("hall-of-fame-filename",html);
  option("hall-of-fame-preview",preview);
  option("log-filename",cmdlog);
  option("credit-filename",credit);
  ioption("credit-enable",enable_credit);
  option("log-traffic-directory",log_dir);
  option("log-traffic-url-path",log_url);
  option("qos-free-zone",qos_free_zone);
  ioption("qos-free-delay",qos_free_delay);
  ioption("qos-proxy-enable",qos_proxy);
  option("qos-proxy-ip",proxy_ip);
  option("htb-leaf-discipline",qos_leaf);
  ioption("qos-proxy-port",proxy_port);
  ioption("free-rate",free_min);
  ioption("free-ceil",free_max);
  ioption("htb-burst",burst);
  ioption("htb-burst-main",burst_main);
  ioption("htb-burst-group",burst_group);
  ioption("htb-nesting-limit",max_nesting);
  ioption("htb-r2q",htb_r2q);
  ioption("magic-include-upload",include_upload);
  ioption("magic-priorities",magic_priorities);
  ioption("magic-treshold",magic_treshold);
  
  option("filter-type", cnf);
  
/* not yet implemented:
  ioption("magic-fixed-packets",fixed_packets);
  ioption("magic-relative-packets",packet_limit);
*/
 }
 fail
 { 
  perror(config_filename);
  puts("Warning - using built-in defaults instead ...");
 }
 done;
 printf("\n");
 
 /*leaf discipline for keywords*/
 every(keyword,keywords)
 {
    if (!strcmpi(keyword->leaf_discipline, "")){
        keyword->leaf_discipline = qos_leaf;
    }
 }

 if (strcmpi(cnf, "mark")){
    filter_type = 2;
    mark = "CLASSIFY";
    mark_iptables = "CLASSIFY --set-class 1:";
 }else{
    filter_type = 1;
    mark = "MARK";
    mark_iptables = "MARK --set-mark ";
 }

 /* are supplied values meaningful ?*/
 if(line<=0 || up<=0)
 {
  puts("Illegal value of wan bandwidth: 0 kbps.");
  reject_config_and_exit(config_filename);
 }
}

/* ===================== traffic analyser - uses iptables  ================ */ 

void get_traffic_statistics(void)
{
 char *str,*cmd;
 int downloadflag=0;

 textfile(Pipe,str) *line,*lines=NULL;
 string(str,STRLEN);
 string(cmd,STRLEN);

 sprintf(cmd,"%s -L -v -x -n -t mangle",iptables);
 shell(cmd);
 input(str,STRLEN)
 {
  create(line,Pipe);
  line->str=str;
  string(str,STRLEN);
  append(line,lines);
 }

 every(line,lines)
 {
  int col, accept=0,proxyflag=0,valid=1,setchainname=0,commonflag=0; 
  unsigned long long traffic=0;
  unsigned long pkts=0;
  char *ipaddr=NULL,*ptr;
  
  /* debug puts(line->str); */
  valid_columns(ptr,line->str,' ',col) 
  if(valid) switch(col)
  { 
   case 1: if(eq(ptr,"Chain"))
            setchainname=1;
           else if(eq(ptr,"pkts")) 
            valid=0;
           else
            sscanf(ptr,"%lu",&pkts); 
           break;
   case 2: if(setchainname)
           {
            if(!strncmp(ptr,"post_",5) || eq(ptr,"POSTROUTING"))
             downloadflag=1;            
            else 
            if(!strncmp(ptr,"forw_",5) || eq(ptr,"FORWARD"))
             downloadflag=0;
            
            if(eq(ptr,"post_common") || eq(ptr,"forw_common"))
             commonflag=1;
           }
           else
            sscanf(ptr,"%Lu",&traffic); traffic+=(1<<19); traffic>>=20;
           break;
   case 3: if((strncmp(ptr,"post_",5) && strncmp(ptr,"forw_",5)) || commonflag)
            accept=eq(ptr,mark);
            /*if (filter_type==1) accept=eq(ptr,"MARK"); else accept=eq(ptr,"CLASSIFY");*/
           break;
   case 8: if(downloadflag)
           { 
            if(strstr(proxy_ip,ptr))proxyflag=1; 
           }
           else
            ipaddr=ptr; 
            break;
   case 9: if(downloadflag)ipaddr=ptr;break;
  }
  
    if(accept && traffic>0 && ipaddr)
    {
     if(proxyflag)printf("(proxy) ");
     else if(!downloadflag) printf("(upload) ");
     printf("IP %s: %Lu M (%ld pkts)\n", ipaddr, traffic, pkts);
     find(ip,ips,eq(ip->addr,ipaddr)); 
     else 
     {
      TheIP();
      ip->addr=ipaddr;
      if(eq(ip->addr,"0.0.0.0/0"))
      {
       ip->name="(unregistered)";
       ip->min=free_min;
       ip->max=ip->desired=free_max;
      }
     }
     
     if(downloadflag)
     {
      if(proxyflag)
       ip->proxy=traffic;
      else
       ip->traffic+=traffic;
      ip->direct=ip->traffic-ip->upload-ip->proxy;
      ip->pktsdown=pkts;
     }
     else
     {
      ip->upload=traffic;
      ip->pktsup=pkts;
      if(include_upload)
       ip->traffic+=traffic;
      else 
       if(traffic>ip->traffic)
        ip->traffic=traffic;     
     }
    }  
  }


 free(cmd);
}
 
/* ========== This function executes, logs OR ALSO prints command ========== */

void safe_run(char *cmd)
{
 if(dry_run) printf("\n=>%s\n",cmd); else system(cmd);
 if(log_file) fprintf(log_file,"%s\n",cmd);
}

void save_line(char *line)
{
 fprintf(iptables_file,"%s\n",line);
}

void run_restore(void)
{
 char *restor, *str;
 string(restor,STRLEN);

 /*-----------------------------------------------------------------*/
 printf("Running %s <%s ...\n",iptablesrestore,iptablesfile);
 /*-----------------------------------------------------------------*/
 
 save_line("COMMIT");
 fclose(iptables_file);
 if(dry_run) 
 {
    parse(iptablesfile)
    {
        str=_;
        printf("%s\n", str);
    }done;
 }

 sprintf(restor,"%s <%s",iptablesrestore, iptablesfile);
 safe_run(restor);
 
 free(restor);
}

/* == This function strips extra characters after IP address and stores it = */

void parse_ip(char *str)
{
 char *ptr=str,*ipaddr=NULL,*ipname=NULL;;
 
 while(*ptr && *ptr!=' ' && *ptr!=9)
  ptr++;
 
 *ptr=0;
 ipaddr=str;
 ptr++;
 while(*ptr && (*ptr==' ' || *ptr==9))
  ptr++;
 ipname=ptr; 
 while(*ptr && *ptr!=' ' && *ptr!=9)
  ptr++;
 *ptr=0;

 find(ip,ips,eq(ip->addr,ipaddr)); else TheIP();
 ip->addr=ipaddr;
 ip->name=ipname;
}

char *parse_datafile_line(char *str)
{
 char *ptr=strchr(str,' ');

 if(ptr)
 {
  *ptr=0;
  ptr++;
  return ptr;
 } 
 else 
  return NULL;
}

/*-----------------------------------------------------------------*/
/* Are you looking for int main (int argc, char **argv) ? :-))     */
/*-----------------------------------------------------------------*/

program
{
 int i=0;
 FILE *f=NULL;
 char *str, *ptr, *d;
 char *substring;
 int class_count=0,ip_count=0;
 int parent=1;
 int just_flush=0;
 int nodelay=0;
 int just_preview=0;                   /* preview - generate just stats */
 char *chain_forward, *chain_postrouting;
 char *althosts=NULL;
  
 printf("\n\
Prometheus QoS - \"fair-per-IP\" Quality of Service setup utility.\n\
Version %s - Copyright (C)2005-2008 Michael Polak (xChaos)\n\
iptables-restore & burst tunning & classify modification 0.7d by Ludva\n\
Credit: CZFree.Net, Martin Devera, Netdave, Aquarius, Gandalf\n\n",version);

 /*----- Boring... we have to check command line options first: ----*/
   
 arguments
 {
  argument("-c") { nextargument(config); }
  argument("-h") { nextargument(althosts);}
  argument("-d") { dry_run=1; }
  argument("-f") { just_flush=1; }
  argument("-9") { just_flush=9; }
  argument("-p") { just_preview=1; }
  argument("-n") { nodelay=1; }
  argument("-?") { help(); exit(0); }
  argument("--help") { help(); exit(0); }
  argument("-v") { exit(0); } 
  argument("--version") { exit(0); } 
 }

 if(dry_run)
  puts("*** THIS IS JUST DRY RUN ! ***\n");

 date(d); /* this is typical cll1.h macro */

 /*-----------------------------------------------------------------*/
 printf("Parsing configuration file %s ...\n", config);
 /*-----------------------------------------------------------------*/
 get_config(config);

 if(althosts) hosts=althosts;

 if(just_flush<9)
 {
  /*-----------------------------------------------------------------*/
  puts("Parsing iptables verbose output ...");
  /*-----------------------------------------------------------------*/
  get_traffic_statistics();
 }

 /*-----------------------------------------------------------------*/
 printf("Parsing class defintion file %s ...\n", hosts);
 /*-----------------------------------------------------------------*/
 int groupidx = FIRSTGROUPID;
 parse(hosts)
 {
  str=_;

  if(*str<'0' || *str>'9')
   continue;
  
  //Does this IP share QoS class with some other ?
  substring=strstr(str,"sharing-");
  if(substring)
  { 
   substring+=8; //"sharing-"
   parse_ip(str);
   ip_count++;
   ip->sharing=substring;
   ip->keyword=defaultkeyword; /* settings for default keyword */
   while(*substring && *substring!='\n')
    substring++;
   *substring=0; 
  }
  else
  {
   //Do we have to create new QoS class for this IP ?

   find(keyword,keywords,(substring=strstr(str,keyword->key)))
   {
    parse_ip(str);
    ip_count++;
    ip->keyword=keyword;
    keyword->ip_count++;
    ip->prio=keyword->default_prio;
    substring+=strlen(keyword->key)+1;
    ptr=substring;
    while(*ptr && *ptr!='-')
     ptr++;
    if(*ptr=='-')
    {
     *ptr=0;
     ip->max=ip->desired=atoi(ptr+1);
    }
    ip->min=atoi(substring);
    if(ip->min<=0)
    {
     puts("Illegal value of minimum bandwidth: 0 kbps.");
     reject_config_and_exit(hosts);
    }
    if(ip->max<=ip->min)
    {
     ip->fixedprio=1;
     ip->max=ip->min+ip->keyword->reserve_min;
    }
    else 
    {
     ip->max-=ip->keyword->reserve_max;

/*
     if(ip->keyword->divide_max>1)
      ip->max=ip->min+(ip->max-ip->min)/ip->keyword->divide_max;
     if(ip->keyword->htb_ceil_bonus_divide>0)
      ip->max+=ip->max/ip->keyword->htb_ceil_bonus_divide;
*/
     if(ip->max<ip->min)
      ip->max=ip->min;
    }
    ip->mark=FIRSTIPCLASS+1+class_count++;

    find(group,groups,group->min==ip->min) 
    { 
     group->count++;      
     group->desired+=ip->min;
     ip->group = group->id;   
    }
    else
    {
     create(group,Group);
     group->min=ip->min;
     group->id = groupidx++;
     ip->group = group->id;

     if(group->min<8) group->min=8;
     /* Warning - this is maybe because of primitive tc namespace, can be fixed */
     /* it is because class IDs are derived from min. bandwidth. - xCh */
     //if(group->min>MAX_GUARANTED_KBPS) group->min=MAX_GUARANTED_KBPS;
     
     group->count=1;
     group->desired=ip->min;   
     insert(group,groups,desc_order_by,min);
    }
   }//endif keyword-
  }//endif sharing-
 }
 fail
 {
  perror(hosts);
  exit(-1);
 }
 done;

 /*-----------------------------------------------------------------*/
 /* cll1.h - let's allocate brand new character buffer...           */
 /*-----------------------------------------------------------------*/
 string(str,STRLEN); 

 /*-----------------------------------------------------------------*/
 puts("Resolving shared connections ...");
 /*-----------------------------------------------------------------*/
 search(ip,ips,ip->sharing)
 {
  search(sharedip,ips,eq(sharedip->name,ip->sharing))
  {
   sharedip->traffic+=ip->traffic;
   ip->traffic=0;
   ip->mark=sharedip->mark; 
   break;
  }
  if(!sharedip)
   printf("Unresolved shared connection: %s %s sharing-%s\n",ip->addr,ip->name,ip->sharing);
 }

 if(enable_credit && just_flush<9)
 {
  /*-----------------------------------------------------------------*/
  printf("Parsing credit file %s ...\n", credit);
  /*-----------------------------------------------------------------*/
  parse(credit)
  {
   ptr=parse_datafile_line(_);
   if(ptr)
   {
    find(ip,ips,eq(ip->addr,_))
     sscanf(ptr,"%Lu",&(ip->credit));
   }
  }
  done;
 }

 if(!just_preview)
 {
  /*-----------------------------------------------------------------*/
  puts("Initializing iptables and tc classes ...");
  /*-----------------------------------------------------------------*/
  
  iptables_file=fopen(iptablesfile,"w");
  if (iptables_file == NULL) {
    puts("Cannot open iptablesfile!");
    exit(-1);
  }
  
  log_file=fopen(cmdlog,"w");
  if (log_file == NULL) {
    puts("Cannot open logfile!");
    exit(-1);
  }
  
  save_line(iptablespreamble);
  run_restore();
  
  sprintf(str,"%s qdisc del dev %s root 2>/dev/null",tc,lan);
  safe_run(str);

  sprintf(str,"%s qdisc del dev %s root 2>/dev/null",tc,wan);
  safe_run(str);
  
  iptables_file=fopen(iptablesfile,"w");
  save_line(iptablespreamble);

  if(qos_free_zone && *qos_free_zone!='0')
  {
   char *chain;
   
   sprintf(str,"-A FORWARD -d %s -o %s -j ACCEPT", qos_free_zone, wan);
   save_line(str);
   
   if(qos_proxy)
   {
    save_line(":post_noproxy - [0:0]");
    sprintf(str,"-A POSTROUTING -p ! tcp -o %s -j post_noproxy", lan);
    save_line(str);   
    sprintf(str,"-A POSTROUTING -s ! %s -o %s -j post_noproxy", proxy_ip, lan);
    save_line(str);   
    sprintf(str,"-A POSTROUTING -s %s -p tcp --sport ! %d -o %s -j post_noproxy", proxy_ip, proxy_port, lan);
    save_line(str);   

    chain="post_noproxy";    
   }
   else
    chain="POSTROUTING";
    
   sprintf(str,"-A %s -s %s -o %s -j ACCEPT", chain, qos_free_zone, lan);
   save_line(str);
  }
  
  if(ip_count>idxtable_treshold1 && !just_flush)
  {
   int idxcount=0, bitmask=32-idxtable_bitmask1; /* default net mask: 255.255.255.240 */
   char *subnet, *buf;
   /*-----------------------------------------------------------------*/
   printf("Detected %d addresses - indexing iptables rules to improve performance...\n",ip_count);
   /*-----------------------------------------------------------------*/

   save_line(":post_common - [0:0]");
   save_line(":forw_common - [0:0]");

   search(ip,ips,ip->addr && *(ip->addr) && !eq(ip->addr,"0.0.0.0/0"))
   {
    buf=hash_id(ip->addr,bitmask);
    find(idx,idxs,eq(idx->id,buf))
     idx->children++;
    else
    {
     create(idx,Index);
     idx->addr=ip->addr;
     idx->id=buf;
     idx->bitmask=bitmask;
     idx->parent=NULL;
     idx->children=0;
     idxcount++;
     push(idx,idxs);
    }
   }

   /* brutal perfomance optimalization */
   while(idxcount>idxtable_treshold2 && bitmask>2*idxtable_bitmask2)
   {
    bitmask-=idxtable_bitmask2;
    idxcount=0;
    search(idx,idxs,idx->parent==NULL)
    {
     buf=hash_id(idx->addr,bitmask);
     find(metaindex,idxs,eq(metaindex->id,buf))
      metaindex->children++;     
     else
     {
      create(metaindex,Index);
      metaindex->addr=idx->addr;
      metaindex->id=buf;
      metaindex->bitmask=bitmask;
      metaindex->parent=NULL;
      metaindex->children=0;
      idxcount++;
      push(metaindex,idxs);
     }
     idx->parent=metaindex;
    }
   }

   /* this should slightly optimize throughout ... */
   sort(idx,idxs,desc_order_by,children);
   sort(idx,idxs,order_by,bitmask);

   i=0;
   every(idx,idxs)
   {
    subnet=subnet_id(idx->addr,idx->bitmask);
    printf("%d: %s/%d\n",++i,subnet,idx->bitmask);
       
    sprintf(str,":post_%s - [0:0]", idx->id);
    save_line(str);

    sprintf(str,":forw_%s - [0:0]", idx->id);
    save_line(str);

    if(idx->parent)
    {
     string(buf,strlen(idx->parent->id)+6);
     sprintf(buf,"post_%s",idx->parent->id);
    }
    else
     buf="POSTROUTING";

    sprintf(str,"-A %s -d %s/%d -o %s -j post_%s", buf, subnet, idx->bitmask, lan, idx->id);
    save_line(str);

    sprintf(str,"-A %s -d %s/%d -o %s -j post_common", buf, subnet, idx->bitmask, lan);
    save_line(str);

    if(idx->parent)
    {
     string(buf,strlen(idx->parent->id)+6);
     sprintf(buf,"forw_%s",idx->parent->id);
    }
    else
     buf="FORWARD";

    sprintf(str,"-A %s -s %s/%d -o %s -j forw_%s", buf, subnet, idx->bitmask, wan, idx->id);
    save_line(str);

    sprintf(str,"-A %s -s %s/%d -o %s -j forw_common", buf, subnet, idx->bitmask, wan);
    save_line(str);
   }
   printf("Total indexed iptables chains created: %d\n", i);

   sprintf(str,"-A FORWARD -o %s -j forw_common", wan);
   save_line(str);
   
   sprintf(str,"-A POSTROUTING -o %s -j post_common", lan);
   save_line(str);
  }
 
 }

 if(just_flush)
 {
  fclose(iptables_file);
  if (log_file) fclose(log_file);
  puts("Just flushed iptables and tc classes - now exiting ...");
  exit(0);
 }

 if(!just_preview)
 {
  if(!dry_run && !nodelay && qos_free_delay)
  {
   printf("Flushed iptables and tc classes - now sleeping for %d seconds...\n",qos_free_delay);
   sleep(qos_free_delay);
  }

  sprintf(str,"%s qdisc add dev %s root handle 1: htb r2q %d default 1",tc,lan,htb_r2q);
  safe_run(str);

  sprintf(str,"%s class add dev %s parent 1: classid 1:2 htb rate %s ceil %s burst %dk prio 0",tc,lan,lan_medium,lan_medium,burst_main);
  safe_run(str);

  sprintf(str,"%s class add dev %s parent 1:2 classid 1:1 htb rate %Ldkbit ceil %Ldkbit burst %dk prio 0",tc,lan,line,line,burst_main);
  safe_run(str);

  sprintf(str,"%s qdisc add dev %s root handle 1: htb r2q %d default 1",tc,wan,htb_r2q);
  safe_run(str);

  sprintf(str,"%s class add dev %s parent 1: classid 1:2 htb rate %s ceil %s burst %dk prio 0",tc,wan,wan_medium,wan_medium,burst_main);
  safe_run(str);

  sprintf(str,"%s class add dev %s parent 1:2 classid 1:1 htb rate %Ldkbit ceil %Ldkbit burst %dk prio 0",tc,wan,up,up,burst_main);
  safe_run(str);
 }

 /*-----------------------------------------------------------------*/
 puts("Locating suckers and generating root classes ...");
 /*-----------------------------------------------------------------*/
 sort(ip,ips,desc_order_by,traffic);
 

 /*-----------------------------------------------------------------*/
 /* sub-scope - local variables */  
 {
  long long int rate=line;
  long long int max=line;
  int group_count=0;
  FILE *credit_file=NULL;
  
  if(!just_preview && !dry_run && enable_credit) credit_file=fopen(credit,"w");
    
  every(group,groups)
  {
   if(!just_preview)
   {
    
    //download
    sprintf(str,"%s class add dev %s parent 1:%d classid 1:%d htb rate %Ldkbit ceil %Ldkbit burst %dk prio 1 #down desired %d", 
                 tc, lan, parent, group->id, rate, max, burst_group, group->desired);
    safe_run(str);
    
    //upload
    sprintf(str,"%s class add dev %s parent 1:%d classid 1:%d htb rate %Ldkbit ceil %Ldkbit burst %dk prio 1 #up desired %d", 
                 tc, wan, parent, group->id, rate*up/line, max*up/line, burst_group, group->desired);
    safe_run(str);
   }
   
   if(group_count++<max_nesting) parent=group->id;
   
   rate-=digital_divide*group->min;
   if(rate<group->min)rate=group->min;
    
   /*shaping of aggresive downloaders, with credit file support */
   if(use_credit)
   {
    int group_rate=group->min, priority_sequence=magic_priorities+1;
    
    search(ip, ips, ip->min==group->min && ip->max>ip->min)
    {
     if( ip->keyword->data_limit && !ip->fixedprio &&
         ip->traffic>ip->credit+
         (ip->min*ip->keyword->data_limit+(ip->keyword->fixed_limit<<20)) )
     {
      if(group_rate<ip->max) ip->max=group_rate;
      group_rate+=magic_treshold;
      ip->prio=magic_priorities+2;
      if(ip->prio<3) ip->prio=3;
     }
     else
     {
      if( ip->keyword->data_prio && !ip->fixedprio &&
          ip->traffic>ip->credit+
           (ip->min*ip->keyword->data_prio+(ip->keyword->fixed_prio<<20)) )
      {
       ip->prio=priority_sequence--;
       if(ip->prio<2) ip->prio=2;
      }
     
      if(credit_file)
      {
       unsigned long long lcredit=0;
       
       if((ip->min*ip->keyword->data_limit+(ip->keyword->fixed_limit<<20))>ip->traffic) 
        lcredit=(ip->min*ip->keyword->data_limit+(ip->keyword->fixed_limit<<20))-ip->traffic;
       fprintf(credit_file,"%s %Lu\n",ip->addr,lcredit);
      }
     }
    }
        
   }
  }
  if(credit_file)fclose(credit_file);
 }

 if(just_preview)
 {
  f=fopen(preview,"w");
  ptr=preview; 
 }
 else if(!dry_run && !just_flush)
 {
  /*-----------------------------------------------------------------*/
  printf("Writing data transfer database ...\n");
  /*-----------------------------------------------------------------*/
  f=fopen("/var/run/prometheus.previous","w");
  if(f)
  {
   search(ip,ips,ip->traffic || ip->direct || ip->proxy ||ip->upload)
    fprintf(f,"%s %Lu %Lu %Lu %Lu\n",ip->addr,ip->traffic,ip->direct,ip->proxy,ip->upload);
   fclose(f);
  }

  f=fopen(html,"w");
  ptr=html;
 }

 if(f)
 {
  int total=0;
  int count=1;
  i=0;

  /*-----------------------------------------------------------------*/
  printf("Sorting data and generating statistics page %s ...\n",ptr);
  /*-----------------------------------------------------------------*/

  fputs("<table border>\n<tr><th align=\"right\">#</th><th align=\"right\">group</th><th align=\"right\">IPs</th><th align=\"right\">requested</th>\n",f);
  fprintf(f,"<th colspan=\"%d\">data limits</th>\n",keywordcount);
  fputs("</tr>\n",f);
  every(group,groups) 
  { 
#ifdef DEBUG
   printf("%d k group: %d bandwidth requested: %d k\n",group->min,group->count,group->desired);
#endif
   fprintf(f,"<tr><td align=\"right\">%d</td><td align=\"right\">%d k</td>",count,group->min);
   fprintf(f,"<td align=\"right\">%d</td><td align=\"right\">%d k</td>",group->count,group->desired);

   every(keyword,keywords)
    fprintf(f,"<td align=\"right\"><font color=\"#%s\">%d M</font></td>",keyword->html_color,group->min*keyword->data_limit); 
   
   i+=group->desired; 
   total+=group->count;
   count++; 
  }
#ifdef DEBUG
   printf("Total groups: %d Total bandwidth requested: %d k\nAGGREGATION: 1/%d\n",count,i,i/line);
#endif
   fprintf(f,"<tr><th colspan=\"2\" align=\"left\">Line %Ld k</td>",line);
   fprintf(f,"<th align=\"right\">%d</td><th align=\"right\">%d k</td>",total,i);

   every(keyword,keywords)
    fprintf(f,"<th align=\"right\">%d IPs</th>",keyword->ip_count); 

   fprintf(f,"</tr><tr><th colspan=\"4\">Aggregation 1/%d</th>\n",(int)(0.5+i/line));
   fprintf(f,"<th colspan=\"%d\">%d traffic classes</th></tr>\n",keywordcount,total);

   fputs("</table>\n",f);
 }
 else if(!dry_run && !just_flush) 
  perror(html);

 i=1;
 if(f)
 {
  unsigned long long total=0, total_direct=0, total_proxy=0, total_upload=0, tmp_sum=0;
  int active_classes=0;
  int colspan;
  FILE *iplog;
  struct Sum {unsigned long long l; int i; list(Sum);} *sum,*sums=NULL;

  if(qos_proxy)
   colspan=12;
  else 
   colspan=11;
  
  fprintf(f,"<p><table border>\n<tr><th colspan=\"%d\">%s",colspan,title);
  fprintf(f," (%s)</th></tr>\n", d);
  fputs("<tr><td align=\"right\">#</td><td>hostname</td>\
  <td align=\"right\">credit</td>\
  <td align=\"right\">limit</td>\
  <td align=\"right\">total</td>\
  <td align=\"right\">direct</td>\n",f);
  if(qos_proxy)
   fputs("<td align=\"right\">proxy</td>\n",f);
  fputs("<td align=\"right\">upload</td>\
  <td align=\"right\">minimum</td>\
  <td align=\"right\">desired</td>\
  <td align=\"right\">maximum</td>\
  <td>prio</td></tr>\n",f);	

  every(ip,ips)
  {
   char *f1="", *f2="";
   if(ip->max<ip->desired)
   {
    f1="<font color=\"red\">";
    f2="</font>";
   }
   else if(ip->prio>1)
   {
    f1="<font color=\"brown\">";
    f2="</font>";
   }

#ifdef DEBUG
   printf("%03d. %-22s %10Lu (%d/%d)\n",i ,ip->name, ip->traffic, ip->min, ip->max); 
#endif
   fprintf(f,"<tr><td align=\"right\"><a name=\"%s\"></a>%d</td><td><a href=\"%s%s.log\">%s</a></td><td align=\"right\">%Lu M</td>\n",
              ip->name, i, log_url, ip->name, ip->name, ip->credit);
   fprintf(f,"<td align=\"right\"><font color=\"#%s\">%Lu M</font></td>",ip->keyword->html_color,ip->credit+(ip->min*ip->keyword->data_limit+(ip->keyword->fixed_limit<<20)));
   fprintf(f,"<td align=\"right\">%s%Lu M%s</td><td align=\"right\">%Lu M</td>\n", f1, ip->traffic, f2, ip->direct);
   if(qos_proxy)
    fprintf(f,"<td align=\"right\">%Lu M</td>\n", ip->proxy);
   fprintf(f,"<td align=\"right\">%Lu M</td>\n", ip->upload);
   fprintf(f,"<td align=\"right\">%d k</td><td align=\"right\">%d k</td><td align=\"right\">%s%d k%s</td><td>%s%d%s</td></tr>\n",ip->min,ip->desired,f1,ip->max,f2,f1,ip->prio,f2);
   total+=ip->traffic;
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
   
   i++;
   
   if(!just_preview)
   {
    sprintf(str,"%s%s.log",log_dir,ip->name);
    iplog=fopen(str,"a");
    if(iplog)
    {
     fprintf(iplog,"%ld\t%s\t%Lu\t%Lu\t%Lu\t%Lu\t%s",time(NULL),ip->name,ip->traffic, ip->direct, ip->proxy, ip->upload,d);
     fclose(iplog);
    }
   }

  }
  fprintf(f,"<tr><th colspan=\"4 \"align=\"left\">SUMMARY:</td>");
  fprintf(f,"<th align=\"right\">%Lu M</th>\
  <th align=\"right\">%Lu M</th>\n", total, total_direct);
  if(qos_proxy)
   fprintf(f,"<th align=\"right\">%Lu M</th>\n", total_proxy);
  fprintf(f,"<th align=\"right\">%Lu M</th>", total_upload);
  fputs("<td colspan=\"4\"></td></th>\n</table>\n",f);

  if(active_classes>10)
  {
   fputs("<a name=\"erp\"></a><p><table border><tr><th colspan=\"5\">Enterprise Research and Planning (ERP)</th></tr>\n",f);
   fputs("<tr><td>Analytic category</td>\n",f);
   fputs("<td colspan=\"2\" align=\"center\">Active Classes</td><td colspan=\"2\" align=\"center\">Data transfers</td></tr>\n",f);

   find(sum,sums,sum->l>=total/4)
   {
    fprintf(f,"<tr><td>Top 25%% of traffic</td>\n");
    fprintf(f,"<td align=\"right\">%d</td><td align=\"right\">%d %%</td><td align=\"right\">%Lu M</td><td align=\"right\">%Ld %%</td></tr>\n",sum->i,(100*sum->i+50)/active_classes,sum->l,(100*sum->l+50)/total);
   }
   
   find(sum,sums,sum->i==10)
   {
    fprintf(f,"<tr><td>Top 10 downloaders</td>\n");
    fprintf(f,"<th align=\"right\">10</th><td align=\"right\">%d %%</td><td align=\"right\">%Lu M</td><td align=\"right\">%Ld %%</td></tr>\n",(100*sum->i+50)/active_classes,sum->l,(100*sum->l+50)/total);
   }

   find(sum,sums,sum->l>=total/2)
   {
    fprintf(f,"<tr><td>Top 50%% of traffic</td>\n");
    fprintf(f,"<td align=\"right\">%d</td><td align=\"right\">%d %%</td><td align=\"right\">%Lu M</td><th align=\"right\">%Ld %%</th></tr>\n",sum->i,(100*sum->i+50)/active_classes,sum->l,(100*sum->l+50)/total);
   }

   find(sum,sums,sum->l>=4*total/5)
   {
    fprintf(f,"<tr><td>Top 80%% of traffic</td>\n");
    fprintf(f,"<td align=\"right\">%d</td><td align=\"right\">%d %%</td><td align=\"right\">%Lu M</td><th align=\"right\">%Ld %%</th></tr>\n",sum->i,(100*sum->i+50)/active_classes,sum->l,(100*sum->l+50)/total);
   }

   find (sum,sums,sum->i>=(active_classes+1)/5)
   {
    fprintf(f,"<tr><td>Top 20%% downloaders</td>\n");
    fprintf(f,"<td align=\"right\">%d</td><th align=\"right\">%d %%</th><td align=\"right\">%Lu M</td><td align=\"right\">%Ld %%</td></tr>\n",sum->i,(100*sum->i+50)/active_classes,sum->l,(100*sum->l+50)/total);
   }

   find(sum,sums,sum->i>=(active_classes+1)/4)
   {
    fprintf(f,"<tr><td>Top 25%% downloaders</td>\n");
    fprintf(f,"<td align=\"right\">%d</td><td align=\"right\">%d %%</td><td align=\"right\">%Lu M</td><td align=\"right\">%Ld %%</td></tr>\n",sum->i,(100*sum->i+50)/active_classes,sum->l,(100*sum->l+50)/total);
   }

   find(sum,sums,sum->i>=(active_classes+1)/2)
   {
    fprintf(f,"<tr><td>Top 50%% downloaders</td>\n");
    fprintf(f,"<td align=\"right\">%d</td><th align=\"right\">%d %%</th><td align=\"right\">%Lu M</td><td align=\"right\">%Ld %%</td></tr>\n",sum->i,(100*sum->i+50)/active_classes,sum->l,(100*sum->l+50)/total);
   }

   find(sum,sums,sum->i>=4*(active_classes+1)/5)
   {
    fprintf(f,"<tr><td>Top 80%% downloaders</td>\n");
    fprintf(f,"<td align=\"right\">%d</td><td align=\"right\">%d %%</td><td align=\"right\">%Lu M</td><td align=\"right\">%Ld %%</td></tr>\n",sum->i,(100*sum->i+50)/active_classes,sum->l,(100*sum->l+50)/total);
   }

   fprintf(f,"<tr><td>All users, all traffic</td>\n");
   fprintf(f,"<th align=\"right\">%d</th><th align=\"right\">100 %%</th><th align=\"right\">%Lu M</th><th align=\"right\">100 %%</th></tr>\n",active_classes,total);
   fputs("</table>\n",f);
  }
  fprintf(f,"<small>Statistics generated by Prometheus QoS version %s<br>GPL+Copyright(C)2005-2008 Michael Polak, <a href=\"http://www.arachne.cz/\">Arachne Labs</a></small>\n",version);
  fclose(f);
 }

 if(just_preview)
 {
  puts("Statistics preview generated (-p switch) - now exiting ...");
  exit(0);
 }
  
 /*-----------------------------------------------------------------*/
 puts("Generating iptables and tc classes ...");
 /*-----------------------------------------------------------------*/

 i=0;
 printf("%-22s %-15s mark\n","name","ip");
 search(ip,ips,ip->mark>0)
 { 
  
  if(idxs)
  {
   char *buf;
   duplicate(ip->addr,buf);
   buf=hash_id(ip->addr,32-idxtable_bitmask1); 
   
   string(chain_forward,6+strlen(buf));
   strcpy(chain_forward,"forw_");
   strcat(chain_forward,buf);

   string(chain_postrouting,6+strlen(buf));
   strcpy(chain_postrouting,"post_");
   strcat(chain_postrouting,buf);
   
   free(buf);
  }
  else
  {
   chain_forward="FORWARD";
   chain_postrouting="POSTROUTING";
  }

  printf("%-22s %-16s %04d ", ip->name, ip->addr, ip->mark); 

  /* -------------------------------------------------------- mark download */
  
  sprintf(str,"-A %s -d %s/32 -o %s -j %s%d",chain_postrouting,ip->addr,lan,mark_iptables,ip->mark);
  /*sprintf(str,"-A %s -d %s/32 -o %s -j MARK --set-mark %d",chain_postrouting,ip->addr,lan,ip->mark);*/
  /* -m limit --limit 1/s */  
  save_line(str);

  if(qos_proxy)
  {
   sprintf(str,"-A %s -s %s -p tcp --sport %d -d %s/32 -o %s -j %s%d",chain_postrouting,proxy_ip,proxy_port,ip->addr,lan,mark_iptables,ip->mark);
   /*sprintf(str,"-A %s -s %s -p tcp --sport %d -d %s/32 -o %s -j MARK --set-mark %d",chain_postrouting,proxy_ip,proxy_port,ip->addr,lan,ip->mark);*/
   save_line(str);
  }

  sprintf(str,"-A %s -d %s/32 -o %s -j ACCEPT",chain_postrouting,ip->addr,lan);
  save_line(str);

  /* -------------------------------------------------------- mark upload */
  sprintf(str,"-A %s -s %s/32 -o %s -j %s%d",chain_forward,ip->addr,wan,mark_iptables,ip->mark);
  /*  sprintf(str,"-A %s -s %s/32 -o %s -j MARK --set-mark %d",chain_forward,ip->addr,wan,ip->mark);*/
  save_line(str);

  sprintf(str,"-A %s -s %s/32 -o %s -j ACCEPT",chain_forward,ip->addr,wan);
  save_line(str);

  if(ip->min)
  {
   /* -------------------------------------------------------- download class */
   printf("(down: %dk-%dk ", ip->min, ip->max); 

   sprintf(str,"%s class add dev %s parent 1:%d classid 1:%d htb rate %dkbit ceil %dkbit burst %dk prio %d", tc, lan, ip->group, ip->mark,ip->min,ip->max, burst, ip->prio);
   safe_run(str);

   if (strcmpi(ip->keyword->leaf_discipline, "none")){
    sprintf(str,"%s qdisc add dev %s parent 1:%d handle %d %s", tc, lan, ip->mark, ip->mark, ip->keyword->leaf_discipline); /*qos_leaf*/
    safe_run(str);
   }
   
   if (filter_type == 1){
    sprintf(str,"%s filter add dev %s parent 1:0 protocol ip handle %d fw flowid 1:%d", tc, lan, ip->mark, ip->mark);
    safe_run(str);
   }

   /* -------------------------------------------------------- upload class */
   printf("up: %dk-%dk)\n", (int)((ip->min/ip->keyword->asymetry_ratio)-ip->keyword->asymetry_fixed), 
                            (int)((ip->max/ip->keyword->asymetry_ratio)-ip->keyword->asymetry_fixed));

   sprintf(str,"%s class add dev %s parent 1:%d classid 1:%d htb rate %dkbit ceil %dkbit burst %dk prio %d",
                tc, wan, ip->group, ip->mark,
                (int)((ip->min/ip->keyword->asymetry_ratio)-ip->keyword->asymetry_fixed),
                (int)((ip->max/ip->keyword->asymetry_ratio)-ip->keyword->asymetry_fixed), burst, ip->prio);
   safe_run(str);
   
   if (strcmpi(ip->keyword->leaf_discipline, "none")){
    sprintf(str,"%s qdisc add dev %s parent 1:%d handle %d %s",tc, wan, ip->mark, ip->mark, ip->keyword->leaf_discipline); /*qos_leaf*/
    safe_run(str);
   }
   
   if (filter_type == 1){
    sprintf(str,"%s filter add dev %s parent 1:0 protocol ip handle %d fw flowid 1:%d",tc, wan, ip->mark, ip->mark);
    safe_run(str);
   }
  }
  else
   printf("(sharing %s)\n", ip->sharing);
  i++;
 }


 if(idxs)
 {
  chain_forward="forw_common";
  chain_postrouting="post_common";
 }
 else
 {
  chain_forward="FORWARD";
  chain_postrouting="POSTROUTING";
 }

 /* --------------------------------------------------------  mark download */

 if(qos_proxy)
 {
  sprintf(str,"-A %s -s %s -p tcp --sport %d -o %s -j MARK --set-mark 3",chain_postrouting,proxy_ip,proxy_port,lan);
  save_line(str);
  sprintf(str,"-A %s -s %s -p tcp --sport %d -o %s -j ACCEPT",chain_postrouting,proxy_ip,proxy_port,lan);
  save_line(str);
 }
 sprintf(str,"-A %s -o %s -j MARK --set-mark 3",chain_postrouting,lan);
 save_line(str);
 sprintf(str,"-A %s -o %s -j ACCEPT",chain_postrouting,lan);
 save_line(str);

 /* --------------------------------------------------------  mark upload */
 sprintf(str,"-A %s -o %s -j MARK --set-mark 3",chain_forward,wan);
 save_line(str);
 sprintf(str,"-A %s -o %s -j ACCEPT",chain_forward,wan);
 save_line(str);

 printf("Total IP count: %d\n", i);

 /*-----------------------------------------------------------------*/
 puts("Generating free bandwith classes ...");
 /*-----------------------------------------------------------------*/

 /* ---------------------------------------- tc - free bandwith shared class */
 sprintf(str,"%s class add dev %s parent 1:%d classid 1:3 htb rate %dkbit ceil %dkbit burst %dk prio 2",tc,lan,parent,free_min,free_max,burst);
 safe_run(str);

 sprintf(str,"%s class add dev %s parent 1:%d classid 1:3 htb rate %dkbit ceil %dkbit burst %dk prio 2",tc,wan,parent,free_min,free_max,burst);
 safe_run(str);

 /* tc SFQ */
 if (strcmpi(qos_leaf, "none")){
  sprintf(str,"%s qdisc add dev %s parent 1:3 handle 3 %s",tc,lan,qos_leaf);
  safe_run(str);
 
  sprintf(str,"%s qdisc add dev %s parent 1:3 handle 3 %s",tc,wan,qos_leaf);
  safe_run(str);
 }
 
 /* tc handle 1 fw flowid */
 sprintf(str,"%s filter add dev %s parent 1:0 protocol ip handle 3 fw flowid 1:3",tc,lan);
 safe_run(str);

 sprintf(str,"%s filter add dev %s parent 1:0 protocol ip handle 3 fw flowid 1:3",tc,wan);
 safe_run(str);

 run_restore();
 
 if (log_file) fclose(log_file);
 return 0;

 /* that's all folks, thank you for reading it all the way up to this point ;-) */
 /* bad luck C<<1 is not yet finished, I promise no sprintf() next time... */
}
