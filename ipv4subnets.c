#include "cll1-0.6.2.h"

/* ====== iptables indexes are used to reduce complexity to log8(N) ===== */

char *very_ugly_ipv4_code(char *inip, int bitmask, int format_as_chainname)
{
 /* warning: this function was debugged only for bitmask values 20,24,28 !!!*/
 int dot=0, n;
 char *ip,*outip,*outptr,*fmt;

 duplicate(inip,ip);
 /* debug printf("(%s,%d) -> ",ip,bitmask); */

 if(ip && *ip && bitmask>=0 && bitmask<=32)
 {
  string(outip,strlen(ip)+10); /*fuck unicode? assertion: 10>strlen("_%d_%d") */
 }
 else 
 {
  /* should never exit here */
  return "undefined";
 }
 outptr=outip;
 while(ip && *ip)
 {
  if(*ip=='.')
  {
   if(dot<(bitmask/8-1)) 
   {
    if(format_as_chainname)
    {
     *outptr='_';
    }
    else
    {
     *outptr='.';
    }
    outptr++;
    dot++;
   }
   else
   {
    char *cutdot=strchr(ip+1,'.'); /*for bitmask<24*/    
    if(cutdot)
    {
     *cutdot = '\0';
    }
    
    if(format_as_chainname)
    {
     fmt = "_%d_%d";
    }
    else
    {
     fmt = ".%d";
    }
    
    if(bitmask%8)
    {
     n = atoi(ip+1)-atoi(ip+1)%(1<<(8-bitmask%8));
    }
    else
    {
     n = 0;
    }

    /*debug  printf("%d/%d => [_%d_%d]\n",atoi(ip+1),bitmask,n,bitmask); */
    sprintf(outptr,fmt,n,bitmask);
    if(!format_as_chainname)
    {
     while(bitmask<24)
     {
      strcat(outip,".0");
      bitmask+=8;
     }
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

char *index_id(char *ip,int bitmask)
{ 
 return very_ugly_ipv4_code(ip,bitmask,1);
}

char *subnet_id(char *ip,int bitmask)
{
 return very_ugly_ipv4_code(ip,bitmask,0);
}
