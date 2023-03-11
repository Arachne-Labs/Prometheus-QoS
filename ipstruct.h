/* Modified by: xChaos, 20200104 */

#define MONITORINGTRHU_CTU

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
 int aggregated;
 int fixedprio;
 int group;
 char *code;
 int pps_limit;
 unsigned long long direct;
 unsigned long long proxy;
 unsigned long long traffic_down;
 unsigned long long traffic_up;
 unsigned long long upload;
 unsigned long long traffic;
 unsigned long long credit;
 unsigned long long realquota;
 unsigned long pktsup;
 unsigned long pktsdown;
 struct Keyword *keyword;
#ifdef MONITORINGTRHU_CTU
 char *technology_str;
 char *ruian_id_str;
#endif
 int v6;
 int mask;
 struct IP *uplink;
 list(IP);
};

struct Group
{
 int min;
 int count;
 int desired;
 int id;
 list(Group);
};

struct Keyword
{
 char *key;
 
 float asymetry_ratio;      /* ratio for ADSL-like upload */
 int asymetry_fixed;        /* fixed treshold for ADSL-like upload */
 int data_limit;            /* hard shaping: apply magic_treshold if max*data_limit MB exceeded */
 int data_prio;             /* soft shaping (qos): reduce HTB prio if max*data_prio MB exceeded */
 long fixed_limit;          /* fixed data limit for setting lower HTB ceil */
 long fixed_prio;           /* fixed data lmit for setting lower HTB prio */
 int reserve_min;	    /* bonus for nominal HTB rate bandwidth (in kbps) */
 int reserve_max;	    /* malus for nominal HTB ceil (in kbps) */
// int divide_max;	    /* relative malus: new_ceil=rate+(old_ceil-rate)/divide_max */
// int htb_ceil_bonus_divide; /* relative bonus: new_ceil=old_ceil+old_ceil/htb_ceil_bonus_divide */
 int default_prio;	    /* default HTB priority for this keyword */
 int download_aggregation;  /* apply agregation with -s start_shaping switch */
 int upload_aggregation;    /* apply agregation with -s start_shaping switch */
 char *html_color;
 int ip_count;
 char *leaf_discipline;
 int allowed_avgmtu;        /* this is for calculating packet limits, 0 = no limit*/ 
 list(Keyword);
};

struct Macro
{
 char *rewrite_from;
 char *rewrite_to;
 list(Macro);
};

struct QosFreeInterface
{
 char *name;
 int _eoln;
 list(QosFreeInterface);
};

struct Index
{
 char *addr;
 char *id;
 struct Index *parent;
 int bitmask;
 int children;
 int ipv6;
 list(Index);
};

struct Interface 
{
 char *name;
 long long speed;
 int is_upstream;
 char *chain;
 char *idxprefix;
 list(Interface);
};

void TheIP(char *ipaddr, int is_network);
/* function implemented in parsehosts.c */

#ifdef MONITORINGTRHU_CTU
struct Technology
{
 char *filename;
 list(Technology);
};

extern struct Technology *technologies, *technology;
#endif
