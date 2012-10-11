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
 int lmsid;
 unsigned long long direct;
 unsigned long long proxy;
 unsigned long long upload;
 unsigned long long traffic;
 unsigned long long credit;
 unsigned long long realquota;
 unsigned long pktsup;
 unsigned long pktsdown;
 struct Keyword *keyword;
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
 
 int asymetry_ratio;        /* ratio for ADSL-like upload */
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
 char *html_color;
 int ip_count;
 char *leaf_discipline;
 
 list(Keyword);
};
