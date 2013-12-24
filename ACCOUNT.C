/***************************************************************************
 *                                                                         *
 *       THE FOLLOWING SOURCE CODE IS PROVIDED FREE OF CHARGE BY           *
 *                                                                         *
 *    GALACTICOMM, INC., 11360 TARA DRIVE, PLANTATION FL 33325 U.S.A.      *
 *                                                                         *
 *   WE HOPE YOU FIND IT HELPFUL IN DETERMINING WHETHER OR NOT OUR         *
 *   MULTI-USER HARDWARE AND SOFTWARE PRODUCTS ARE RIGHT FOR YOU.          *
 *   PLEASE FEEL FREE TO USE THIS SOFTWARE IN WHATEVER WAY YOU MAY FEEL    *
 *   IS APPROPRIATE -- WE ONLY ASK THAT YOU KEEP THIS COMMENT BLOCK WITH   *
 *   THE CODE AT ALL TIMES, AND THAT YOU GIVE CREDIT WHERE CREDIT IS DUE   *
 *   WHEN INCORPORATING PORTIONS OF THIS CODE INTO YOUR OWN PROGRAMS.      *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *   ACCOUNT.C                                                             *
 *                                                                         *
 *   This is the Major BBS online accounting utility.                      *
 *                                                                         *
 *                                            - T. Stryker 7/6/86          *
 *                                                                         *
 ***************************************************************************/
 
#include "stdio.h"
#include "ctype.h"
#include "majorbbs.h"
#include "usracc.h"
#include "btvstf.h"
#include "account.h"
 
FILE *acnmb,*opnmsg();        /* accounting named-message file block ptr   */
 
static struct usracc acctmp,  /* temporary user account storage area       */
                    *accptr;  /* general purpose pointer to user account   */
 
extern
struct usracc usracc[NTERMS], /* user accounting block array               */
              *usaptr;        /* user accounting block pointer for usrnum  */
extern
int usrnum;                   /* global user-number (channel) in effect    */
extern
struct user *usrptr,          /* global pointer to user data in effect     */
            user[NTERMS];     /* user volatile-data structure array        */
extern
char input[INPSIZ],           /* raw user input data buffer                */
     *margv[INPSIZ/2];        /* array of ptrs to word starts, a la argv[] */
extern
int margc,                    /* number of words in margv[], a la argc     */
    inplen,                   /* overall raw input string length           */
    status,                   /* raw status from btusts, where appropriate */
    pfnlvl;                   /* profanity level of current input (0-3)    */
extern
struct module *module[NMODS]; /* module access block pointer table         */
 
extern int othusn;            /* general purpose other-user channel number */
extern struct user *othusp;   /* gen purp other-user user structure ptr    */
extern struct usracc *othuap; /* gen purp other-user accounting data ptr   */
 
extern
BTVFILE *accbb;               /* user accounts btrieve data file           */
extern
struct sysvbl sv;             /* sys-variables record instance for updates */
 
iniacc()
{
     int decevy();
 
     acnmb=opnmsg("account.mcv");
     rtkick(15,&decevy);
}
 
account(clr)
int clr;
{
     static int acnstt;
     static char keyuid[UIDSIZ];
     static char tckstg[10];
     static int real;
     int c;
 
     if (!sameas(usaptr->userid,"sysop") || sameas(margv[0],"x")) {
          return(0);
     }
     setmbk(acnmb);
     if (clr) {
          acnstt=-1;
     }
     if (margc == 0) {
          prfmsg(acnstt);
     }
     else {
          switch (acnstt) {
          case -1:
               prfmsg(acnstt=ACNUID);
               break;
          case ACNUID:
               makhdl();
               setbtv(accbb);
               if (!acqbtv(&acctmp,input,0)) {
                    prfmsg(NSUCHUID,input);
                    prfmsg(ACNUID);
                    break;
               }
               movmem(input,keyuid,UIDSIZ);
               accptr=(onsys(keyuid) ? othuap : &acctmp);
               shwusr(accptr,1);
               prfmsg(acnstt=HMTCK);
               break;
          case HMTCK:
               cpykey(tckstg,margv[0],10);
               prfmsg(acnstt=PORF);
               break;
          case PORF:
               if ((c=tolower(*margv[0])) == 'p') {
                    real=1;
               }
               else if (c == 'f') {
                    real=0;
               }
               else {
                    prfmsg(PORF);
                    break;
               }
               prfmsg(HEREAR,tckstg,(real ? "PAID" : "FREE"),keyuid);
               prfmsg(acnstt=ISOKQ);
               break;
          case ISOKQ:
               if ((c=tolower(*margv[0])) == 'n') {
                    prfmsg(acnstt=ACNUID);
                    break;
               }
               else if (c != 'y') {
                    prfmsg(ISOKQ);
                    break;
               }
               switch (addcrd(keyuid,tckstg,real)) {
               case -1:
                    prfmsg(MYSDEL,keyuid);
                    break;
               case 0:
                    break;
               case 1:
                    if (othusn == usrnum) {
                         prf("You got it.\r");
                    }
                    else {
                         prfmsg(YOUCRD,tckstg);
                         injoth();
                         prfmsg(NOTIF,keyuid);
                    }
                    break;
               }
               prfmsg(acnstt=ACNUID);
               break;
          }
     }
     return(1);
}
 
addcrd(keyuid,tckstg,real)
char *keyuid,*tckstg;
int real;
{
     int ison;
     long ticks,atol();
     char accrec[80];
 
     setbtv(accbb);
     if (!acqbtv(&acctmp,keyuid,0)) {
          return(-1);
     }
     accptr=((ison=onsys(keyuid)) ? othuap : &acctmp);
     ticks=atol(tckstg);
     accptr->tcktot+=ticks;
     accptr->tckavl+=ticks;
     if (real) {
          if (accptr->tckpai == 0 && ticks > 0) {
               sv.numpai+=1;
          }
          accptr->tckpai+=ticks;
          sv.paidytd+=ticks;
          sv.paidmtd+=ticks;
          sv.paiddtd+=ticks;
     }
     updbtv(accptr);
     if (ison && accptr->tckavl > 0) {
          othusp->class=PAYING;
     }
     shocst(1,"CREDITED %-9s%7s %s",keyuid,tckstg,(real ? "(PAID)" : "(FREE)"));
     return(ison);
}
 
saycrd(tckstg)
char *tckstg;
{
     setmbk(acnmb);
     prfmsg(YOUCRD,tckstg);
     injoth();
}
 
cpykey(dest,src,len)
char *dest,*src;
int len;
{
     int dstlen;
 
     movmem(src,dest,len);
     *(dest+len-1)='\0';
     dstlen=strlen(dest);
     setmem(dest+dstlen,len-dstlen,0);
}
 
decevy()
{
     int bemean;
     static int altbnf;
 
     setmbk(acnmb);
     bemean=atcapa();
     othuap=usracc;
     for (othusn=0,othusp=user ; othusn < NTERMS ; othusn++,othusp++) {
          switch (othusp->class) {
          case VACANT:
               break;
          case ONLINE:
          case SUPIPG:
               othusp->minut4+=1;
               if ((othusp->usetmr)++ >= (bemean ? 4 : 8)) {
                    usrnum=othusn;
                    byenow(SEEYEZ);
               }
               break;
          case FRELOA:
               othusp->minut4+=1;
               othuap->frescu+=15;
               sv.usedytd+=15;
               sv.usedmtd+=15;
               sv.useddtd+=15;
               if ((othusp->usetmr)++ >= (bemean ? 30 : 80)) {
                    usrnum=othusn;
                    byenow(SYSFUL);
               }
               break;
          case PAYING:
               othusp->minut4+=1;
               sv.usedytd+=15;
               sv.usedmtd+=15;
               sv.useddtd+=15;
               sv.liveytd+=15;
               sv.livemtd+=15;
               sv.livedtd+=15;
               if ((othuap->tckavl-=15) <= 0) {
                    othuap->tckavl=0;
                    othusp->class=FRELOA;
                    othusp->usetmr=0;
                    prfmsg(SW2FRE);
                    injoth();
               }
               break;
          }
          if ((altbnf&1) && othusp->pfnacc != 0) {
               othusp->pfnacc-=1;
          }
          othuap+=1;
     }
     altbnf+=1;
     rtkick(15,&decevy);
}
 
clsacc()
{
     clsmsg(acnmb);
}
 
atcapa()
{
     return(nliniu() >= 4);
}
 
nliniu()
{
     int othusn,retval;
     struct user *othusp;
 
     retval=0;
     for (othusn=0,othusp=user ; othusn < NTERMS ; othusn++,othusp++) {
          if (othusp->class != VACANT) {
               retval+=1;
          }
     }
     return(retval);
}
 
accmcu()
{
     char *curdat();
     int curmon,curday,lstmon,accmon,accday;
 
     usrnum=-2;
     sscanf(curdat(2),"%d/%d",&curmon,&curday);
     sv.useddtd=sv.livedtd=sv.paiddtd=0;
     if (curday == 1) {
          sv.usedmtd=sv.livemtd=sv.paidmtd=0;
          if (curmon == 1) {
               sv.usedytd=sv.liveytd=sv.paidytd=0;
          }
     }
     lstmon=(curmon-1+11)%12+1;
     sv.numact=sv.numpai=0;
     setbtv(accbb);
     if (qlobtv(0)) {
          do {
               gcrbtv(&acctmp,0);
               accmon=accday=0;
               sscanf(acctmp.usedat,"%d/%d",&accmon,&accday);
               if (accmon != curmon && accmon != lstmon && accday <= curday
                 && acctmp.tckavl <= 0) {
                    delacct(acctmp.userid);
               }
               else {
                    sv.numact+=1;
                    if (acctmp.tckpai != 0L) {
                         sv.numpai+=1;
                    }
               }
          } while (qnxbtv());
     }
}
 
delacct(userid)
char *userid;
{
     int i,(*rouptr)();
 
     setbtv(accbb);
     if (!acqbtv(&acctmp,userid,0)) {
          return(-1);
     }
     delbtv();
     for (i=1 ; i < NMODS ; i++) {
          if ((rouptr=module[i]->dlarou) != NULL) {
               (*rouptr)(userid);
          }
     }
     shocst(1,"DELETED %-9s (Cr: %ld)",userid,acctmp.tckavl);
     setbtv(accbb);
     return(onsys(userid));
}
 
