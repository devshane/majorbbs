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
 *   SIGNUP.C                                                              *
 *                                                                         *
 *   This is the Major BBS signup module.                                  *
 *                                                                         *
 *                                            - T. Stryker 6/24/86         *
 *                                                                         *
 ***************************************************************************/
 
#include "stdio.h"
#include "ctype.h"
#include "majorbbs.h"
#include "usracc.h"
#include "btvstf.h"
#include "signup.h"
 
struct usracc usracc[NTERMS], /* user accounting block array               */
              *usaptr;        /* user accounting block pointer for usrnum  */
 
FILE *supmb;                  /* signup named-message file pointer         */
BTVFILE *accbb;               /* user accounts btrieve data file           */
 
extern
int usrnum;                   /* global user-number (channel) in effect    */
extern
struct user *usrptr;          /* global pointer to user data in effect     */
extern
char input[INPSIZ];           /* raw user input data buffer                */
extern
int inplen,                   /* overall raw input string length           */
    pfnlvl;                   /* profanity level of current input (0-3)    */
extern
struct sysvbl sv;             /* sys-variables record instance for updates */
 
inisup()
{
     FILE *opnmsg();
 
     supmb=opnmsg("signup.mcv");
     accbb=opnbtv("usracc.dat",sizeof(struct usracc));
}
 
signup()
{
     int newstt;
 
     setmbk(supmb);
     newstt=usrptr->substt;
     switch (newstt) {
     case 0:
          prfmsg(INTRO);
          prfmsg(newstt=GSCNWID);
          usrptr->usetmr=0;
          break;
     case GSCNWID:
          if (unumok(GSCNWID,20,136)) {
               btutsw(usrnum,atoi(input));
               usaptr->scnwid=atoi(input);
               prfmsg(SWDEPI);
               prfmsg(newstt=GUSRNAM);
               usrptr->usetmr=0;
          }
          break;
     case GUSRNAM:
          if (uinfok(GUSRNAM,5,NADSIZ)) {
               strcpy(usaptr->usrnam,input);
               prfmsg(newstt=GUSRAD1);
               usrptr->usetmr=0;
          }
          break;
     case GUSRAD1:
          if (uinfok(GUSRAD1,0,NADSIZ)) {
               strcpy(usaptr->usrad1,input);
               prfmsg(newstt=GUSRAD2);
               usrptr->usetmr=0;
          }
          break;
     case GUSRAD2:
          if (uinfok(GUSRAD2,5,NADSIZ)) {
               strcpy(usaptr->usrad2,input);
               prfmsg(newstt=GUSRAD3);
               usrptr->usetmr=0;
          }
          break;
     case GUSRAD3:
          if (uinfok(GUSRAD3,5,NADSIZ)) {
               strcpy(usaptr->usrad3,input);
               prfmsg(newstt=GUSRPHO);
               usrptr->usetmr=0;
          }
          break;
     case GUSRPHO:
          if (uinfok(GUSRPHO,7,PHOSIZ)) {
               strcpy(usaptr->usrpho,input);
               prfmsg(newstt=GSYSTYP);
               usrptr->usetmr=0;
          }
          break;
     case GSYSTYP:
          if (unumok(GSYSTYP,0,7)) {
               usaptr->systyp=atoi(input);
               prfmsg(newstt=GAGE);
               usrptr->usetmr=0;
          }
          break;
     case GAGE:
          if (unumok(GAGE,5,99)) {
               usaptr->age=atoi(input);
               prfmsg(newstt=GSEX);
               usrptr->usetmr=0;
          }
          break;
     case GSEX:
          if (usexok()) {
               usaptr->sex=atoi(input);
               prfmsg(PREUID);
               prfmsg(newstt=GUSERID);
               usrptr->usetmr=0;
          }
          break;
     case GUSERID:
          if (hdlok()) {
               movmem(input,usaptr->userid,UIDSIZ);
               prfmsg(newstt=UIDOK,input);
               usrptr->usetmr=0;
          }
          break;
     case UIDOK:
          if (tolower(input[0]) == 'y') {
               prfmsg(PSWIRO,usaptr->userid);
               prfmsg(newstt=GPSWORD);
          }
          else {
               prfmsg(newstt=GUSERID);
          }
          usrptr->usetmr=0;
          break;
     case GPSWORD:
          if (uinfok(GPSWORD,1,PSWSIZ)) {
               strcpy(usaptr->psword,input);
               prfmsg(newstt=PSWEPI,usaptr->userid,input);
               usrptr->usetmr=0;
          }
          break;
     case PSWEPI:
          prfmsg(WELCOME);
          date(usaptr->credat,2);
          setbtv(accbb);
          insbtv(usaptr);
          usrptr->usetmr=0;
          sv.numact+=1;
          redisp();
          return(0);
     }
     btuclo(usrnum);
     outprf(usrnum);
     usrptr->substt=newstt;
     return(1);
}
 
hdlok()
{
     char *inpptr;
     static int plnmsg[3]={PL1UID,PL2UID,PL3UID};
 
     for (inpptr=input ; isalpha(*inpptr) ; inpptr++) {
     }
     if (*inpptr != '\0') {
          prfmsg(NAAUID);
     }
     else if (pfnlvl != 0) {
          prfmsg(plnmsg[pfnlvl-1]);
     }
     else if (inplen < 4) {
          prfmsg(SMLUID,input);
     }
     else if (inplen >= UIDSIZ) {
          prfmsg(BIGUID,UIDSIZ-1);
     }
     else {
          makhdl();
          setbtv(accbb);
          if (qeqbtv(input,0) || uinsys(input)) {
               prfmsg(UIDINU,input);
          }
          else {
               return(1);
          }
     }
     prfmsg(GUSERID);
     return(0);
}
 
uinfok(pmtmsn,minlen,maxsiz)
int pmtmsn,minlen,maxsiz;
{
     stripb();
     if (inplen < minlen) {
     }
     else if (inplen >= maxsiz) {
          prfmsg(TOOBIG,maxsiz-1);
     }
     else if (pfnlvl > 1) {
          prfmsg(UINPFN);
     }
     else {
          return(1);
     }
     prfmsg(pmtmsn);
     return(0);
}
 
unumok(pmtmsn,minnum,maxnum)
int pmtmsn,minnum,maxnum;
{
     char *inpptr;
     int val;
 
     stripb();
     if (inplen != 0) {
          for (inpptr=input ; isdigit(*inpptr) ; inpptr++) {
          }
          if (*inpptr == '\0') {
               if ((val=atoi(input)) >= minnum && val <= maxnum) {
                    return(1);
               }
          }
     }
     prfmsg(NUMOOR,minnum,maxnum);
     prfmsg(pmtmsn);
     return(0);
}
 
usexok()
{
     input[0]=toupper(input[0]);
     if (input[0] == 'M') {
          strcpy(input,"77");      /* 'M' */
          return(1);
     }
     else if (input[0] == 'F') {
          strcpy(input,"70");      /* 'F' */
          return(1);
     }
     else if (inplen != 0) {
          prfmsg(MRFPLS);
     }
     prfmsg(GSEX);
     return(0);
}
 
stripb()
{
     char *inpptr;
 
     for (inpptr=input+inplen-1 ; inpptr >= input && isspace(*inpptr) ; ) {
          *inpptr--='\0';
     }
     inplen=strlen(input);
}
 
makhdl()
{
     char *inpptr;
 
     stripb();
     input[0]=toupper(input[0]);
     for (inpptr=input+1 ; *inpptr != '\0' ; inpptr++) {
          *inpptr=tolower(*inpptr);
     }
     while (++inpptr-input < UIDSIZ) {
          *inpptr='\0';
     }
}
 
loadup()
{
     makhdl();
     setbtv(accbb);
     if (acqbtv(usaptr,input,0)) {
          btutsw(usrnum,usaptr->scnwid);
          return(1);
     }
     return(0);
}
 
updacc()
{
     setbtv(accbb);
     geqbtv(NULL,usaptr->userid,0);
     updbtv(usaptr);
}
 
finsup()
{
     clsmsg(supmb);
     clsbtv(accbb);
}
