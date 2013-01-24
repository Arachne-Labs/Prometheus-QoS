#include "cll1-0.6.2.h"

/* ====== iptables indexes are used to reduce complexity to log(N) ===== */

char *very_ugly_ipv6_code(char *inip, int bitmask, int format_as_chainname)
{
 int colon=0, n, h;
 char *ip,*outip,*outptr,*fmt;

 duplicate(inip,ip);
 /* debug printf("(%s,%d) -> ",ip,bitmask); */

 if(ip && *ip && bitmask>=0 && bitmask<=64)
 {
  string(outip,strlen(ip)+10); /* assertion: 10>strlen("_%d_%d") */
 }
 else
 {
  /* should never exit here */
  return "undefined";
 }
 outptr=outip;
 while(ip && *ip)
 {
  if(*ip==':')
  {
   if(colon<(bitmask/16-1))
   {
    if(format_as_chainname)
    {
     *outptr='_';
    }
    else
    {
     *outptr=':';
    }
    outptr++;
    colon++;
   }
   else
   {
    char *cutcolon=strchr(ip+1,':');
    if(cutcolon)
    {
     *cutcolon = '\0';
    }
    
    if(format_as_chainname)
    {
     fmt = "_%x_%d";
    }
    else
    {
     fmt = ":%x";
    }
    
    if(bitmask%16)
    {
     sscanf(ip+1, "%x", &h);
/*    printf("[debug - %s scanned hexa as %x]\n",ip+1,h);*/
     n = h-h%(1<<(16-bitmask%16));
    }
    else
    {
     n = 0;
    }

/*  printf("%d/%d => [_%d_%d]\n",h,bitmask,n,bitmask); */
    sprintf(outptr,fmt,n,bitmask);
    if(!format_as_chainname)
    {
     strcat(outip,"::");  /* ahem :-) */
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

char *index6_id(char *ip,int bitmask)
{ 
 return very_ugly_ipv6_code(ip,bitmask,1);
}

char *subnet6_id(char *ip,int bitmask)
{
 return very_ugly_ipv6_code(ip,bitmask,0);
}
