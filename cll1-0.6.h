
/* C<<1 header file v0.6.1 - style sheet for ANSI C  */
/* Please pronounce as "cee-shift-left-by-one" :)  */

/* Copyright (G) 2004-2007 Michael xChaos Polak, x(at)n.cz

   The C<<1 header file is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The C<<1 header file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to 
   Michael Polak, Svojsikova 7, 169 00 Praha 6 Czech Republic */


#ifndef __CLL1H__
#define __CLL1H__

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <malloc.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/* Section For Dummies part 1, updated 2004-05-07 by xCh. */

#define not !
#define TRUE 1
#define FALSE 0
#define loop while(1)
#define iterate(VAR,FROM,TO) for(VAR=FROM; VAR <= TO; VAR++)
#define repeat(N) iterate(_i,1,N)

/* Dynamic list macros & sequences, updated 2003-05-29 by xCh. */

#define list(T) struct T *_next 
#define create(A,T) (A=(struct T *)malloc(sizeof(struct T)),A->_next=NULL)
#define push(A,B) { if(A && A!=B) A->_next=B; B=A; }
#define append(A,B) { if(B) { void *N=A; A->_next=NULL; search(A,B,!A->_next) {A->_next=N; break;}} else push(A,B); }
#define remove(A,B,C) { void **_D=NULL; search(A,B,C) { if(_D)*_D=A->_next; else B=A->_next; free(A); } else _D=(void *)&(A->_next); }
#define drop(A,B) { for( A=B; A ; B=A, A=A->_next, free(B)); B=NULL; }

/* Dynamic list iterations and sequences, updated 2003-05-29 by xCh. */

#define every(A,B) for( A=B; A; A=A->_next)
#define search(A,B,C) every(A,B) if(C)
#define find(A,B,C) search(A,B,C) break; if(A)

/* EXP macros for Dummysort sequences, updated 2003-05-29 by xCh. */

#define order_by(K1,K2) (K1>K2)
#define desc_order_by(K1,K2) (K1<K2)
#define sort_by(K1,K2) (strcasecmp(K1, K2)>0)
#define desc_sort_by(K1,K2) (strcasecmp(K1, K2)<0)
#define ascii_by(K1,K2) (strcmp(K1, K2)>0)
#define desc_ascii_by(K1,K2) (strcmp(K1, K2)<0)

/* Dummysort sequences, updated 2003-05-29 by xCh. */

#define insert(A,B,EXP,K) { if(B) { void **_L=NULL, *H=B; search(B,H,EXP(B->K,A->K)) { if(_L) {*_L=A; A->_next=B; } else push(A,H); break; } else _L=(void *)&(B->_next); if(!B)*_L=A; B=H; } else push(A,B); }
#define sort(A,B,EXP,K) { void *_C; A=B; B=NULL; do { _C=A->_next; A->_next=NULL; insert(A,B,EXP,K); A=_C; } while(_C); }

/* String macros & sequences, updated 2004-04-19 by xCh. */

#define eq(A,B) !strcmp(A,B)
#define strcmpi(A,B) strcasecmp(A,B)
#define strlwr(A) {char *_S=A; while(_&&*_S){*_S=tolower(*_S);_S++;}}
#define strupr(A) {char *_S=A; while(_&&*_S){*_S=toupper(*_S);_S++;}}
#define string(S,L) (S=(char *)malloc(L),*S=0)
#define duplicate(A,B) if(A) { string(B,strlen(A)+1); strcpy(B,A); }
#define concatenate(A,B,C) if (A && B) { string(C,strlen(A)+strlen(B)+1); strcpy(C,A); strcat(C,B); }
#define suffix(A,B,C) (((A=strrchr(B,C))&&!(*(A++)=0))||(A=B))
#define prefix(A,B,C) ((A=B)&&(((B=strchr(B,C))&&!(*(B++)=0))||(B=A)))
#define gotoalpha(CHAR) if(CHAR)while(*CHAR && !isalpha(*CHAR))CHAR++
#define goto_alpha(CHAR) if(CHAR)while(*CHAR && !isalpha(*CHAR) && *CHAR!='_')CHAR++
#define gotoalnum(CHAR) if(CHAR)while(*CHAR && !isalnum(*CHAR))CHAR++
#define goto_alnum(CHAR) if(CHAR)while(*CHAR && !isalnum(*CHAR) && *CHAR!='_')CHAR++
#define skipalpha(CHAR) if(CHAR)while(*CHAR && isalpha(*CHAR))CHAR++
#define skip_alpha(CHAR) if(CHAR)while(*CHAR && (isalpha(*CHAR) || *CHAR=='_'))CHAR++
#define skipalnum(CHAR) if(CHAR)while(*CHAR && isalnum(*CHAR))CHAR++
#define skip_alnum(CHAR) if(CHAR)while(*CHAR && (isalnum(*CHAR) || *CHAR=='_'))CHAR++
#define skipspaces(CHAR) if(CHAR)while(*CHAR==' ')CHAR++
#define cutspaces(CHAR) if(CHAR){int _L=strlen(CHAR); while(--_L>0 && CHAR[_L]==' ')CHAR[_L]=0;}
#define gotochr(CHAR,C) if(CHAR)while(*CHAR && *CHAR!=C)CHAR++
#define tr(CHAR,B,C) {char *_S=CHAR; while(*_S){ if(*_S==B)*_S=C; _S++; }}
#define strswitch(CHAR) {char *_K=CHAR; FILE *_F=NULL; {{ 
#define stroption(STR) if(eq(STR,_K))
#define match(KEY,VAL) {char *_K=KEY, *_V=VAL; FILE *_F=NULL; {{ 
#define assign(STR,SETVAR) stroption(STR) SETVAR=_V

/* Section For Dummies part 2, updated 2004-05-07 by xCh. */

#define program int _I; int main(int argc, char **argv)
#define arguments if(argc>1) for(_I=1;_I<argc;_I++) 
#define argument(A) if(eq(argv[_I],A))
#define thisargument(S) (S=argv[_I])
#define nextargument(S) if(_I+1<argc && (S=argv[++_I]))

/* I/O iterations, updated 2004-04-19 by xCh. */

#define fparse(S,L,F) for(fgets(S,L,F);*S && !feof(F);fgets(S,L,F))
#define input(S,L) fparse(S,L,stdin)
#define fstring(S,F) { int _C=0,_L=0; fpos_t _P; fgetpos(F,&_P); while(_C!='\n' && !feof(F)){ _C=fgetc(F); _L++; } string(S,_L); fsetpos(F,&_P);fgets(S,_L,F);fgetc(F);}
#define parses(S,F) {FILE *_F=fopen(F,"r"); if(_F) { while(!feof(_F)) { fstring(S,_F);  
#define parse(F) {char *_; FILE *_F=fopen(F,"r"); if(_F) { while(!feof(_F)) { fstring(_,_F);  
#define fail }} else {{
#define done }} if(_F)fclose(_F);}
#define option(STR,SETVAR) if(_){char *_K,*_V,*_O,*_Q; duplicate(_,_Q); _O=_Q; tr(_O,'\t',' '); prefix(_K,_O,' '); if(eq(STR,_K)) {skipspaces(_O); prefix(_V,_O,'#'); cutspaces(_V); SETVAR=_V; _=NULL;} else free(_Q);}
#define ioption(STR,SETVAR) if(_){char *_K,*_V,*_O,*_Q; duplicate(_,_Q); _O=_Q; tr(_O,'\t',' '); prefix(_K,_O,' '); if(eq(STR,_K)) {skipspaces(_O); prefix(_V,_O,'#'); cutspaces(_V); SETVAR=atoi(_V); _=NULL;} free(_Q);}
#define loption(STR,SETVAR) if(_){char *_K,*_V,*_O,*_Q; duplicate(_,_Q); _O=_Q; tr(_O,'\t',' '); prefix(_K,_O,' '); if(eq(STR,_K)) {skipspaces(_O); prefix(_V,_O,'#');  cutspaces(_V); SETVAR=atol(_V); _=NULL;} free(_Q);}
#define lloption(STR,SETVAR) if(_){char *_K,*_V,*_O,*_Q; duplicate(_,_Q); _O=_Q; tr(_O,'\t',' '); prefix(_K,_O,' '); if(eq(STR,_K)) {skipspaces(_O); prefix(_V,_O,'#');  cutspaces(_V); SETVAR=atoll(_V); _=NULL;} free(_Q);}

/* Dynamic list advanced I/O, updated 2003-05-30 by xCh. */

#define load(A,B,F,T,K) {char *_S; parses(_S,F) { create(A,T); A->K=_S; A->_eoln=TRUE; append(A,B);} done; A->_eoln=FALSE;}
#define save(A,B,F,K) {FILE *_F=fopen(F,"w"); if(_F) { every(A,B) {fputs(A->K,_F); if(A->_eoln) fputc('\n',_F);} fclose(_F);}}
     
/* I/O sequences, updated 2003-05-29 by xCh. */

#define nullreopen(F) F=freopen("/dev/null","r",F)
#define stdinredir(CMD) {int _r[2];pipe(_r);if(fork()==0){dup2(_r[1],1);close(_r[0]);CMD;exit(0);}nullreopen(stdin);dup2(_r[0],0);close(_r[1]);}
#define shell(CMD) stdinredir(system(CMD))
#define paste(STR) stdinredir(fputs(STR,stdout))

/* String iterations, updated 2003-06-19 by xCh. */

#define split(A,B,C) for(prefix(A,B,C);A;(A!=B)&&prefix(A,B,C)||(A=NULL))
#define valid_split(A,B,C) split(A,B,C) if(*A)
#define columns(A,B,C,V) for(V=0,prefix(A,B,C);A;((A!=B)&&prefix(A,B,C)||(A=NULL)),V++)
#define valid_columns(A,B,C,V) for(V=0,prefix(A,B,C);A;((A!=B)&&prefix(A,B,C))||(A=NULL)) if(*A&&++V)
#define column(A,B,C,V) { int _V; columns(A,B,C,_V) if(_V==V) break; }
#define valid_column(A,B,C,V) { int _V; valid_columns(A,B,C,_V) if(_V==V) break; }

/* Useful structures, updated 2003-05-29 by xCh. */

#define hashtable(TYPE,NAME,VALUE) struct TYPE { char *NAME; char *VALUE; list(TYPE); }
#define textfile(TYPE,LINE) struct TYPE { char *LINE; char _eoln; list(TYPE); }
#define date(S) { time_t _T; _T=time(NULL); duplicate(ctime(&_T),S); }

#endif
