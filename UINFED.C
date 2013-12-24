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
 *   UINFED.C                                                              *
 *                                                                         *
 *   This is the Major BBS demo board user-info editor.                    *
 *                                                                         *
 *                                            - T. Stryker 7/13/86         *
 *                                                                         *
 ***************************************************************************/
 
#include "stdio.h"
#include "ctype.h"
#include "majorbbs.h"
#include "usracc.h"
#include "signup.h"
#include "btvstf.h"
 
int uinfed(),dfsthn();
 
#define UEDSTT      12        /* system info requesting state         */
struct module module12={      /* module interface block               */
     'U',                     /*    main menu select character        */
     "User info display/edit",/*    description for main menu         */
     NULL,                    /*    system initialization routine     */
     NULL,                    /*    user logon supplemental routine   */
     &uinfed,                 /*    input routine if selected         */
     &dfsthn,                 /*    status-input routine if selected  */
     NULL,                    /*    hangup (lost carrier) routine     */
     NULL,                    /*    midnight cleanup routine          */
     NULL,                    /*    delete-account routine            */
     NULL                     /*    finish-up (sys shutdown) routine  */
};
 
#define MAXTKC 9              /* maximum char count of tick xfer request   */
 
char *sysstg[]={              /* strings corresponding to sys type codes   */
     "(nonstandard)",
     "IBM PC or compatible",
     "Apple Macintosh",
     "Apple other than Macintosh",
     "Commodore Amiga",
     "Atari",
     "Radio Shack",
     "CP/M system"
};
 
extern
BTVFILE *accbb;               /* user accounts btrieve data file           */
extern
FILE *supmb;                  /* uses the signup module's message file     */
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
    status;                   /* raw status from btusts, where appropriate */
extern
struct sysvbl sv;             /* sys-variables record instance for updates */
 
extern int othusn;            /* general purpose other-user channel number */
extern struct user *othusp;   /* gen purp other-user user structure ptr    */
extern struct usracc *othuap; /* gen purp other-user accounting data ptr   */
 
uinfed()
{
     int c,newstt,online;
     long atol();
     char *ltoa();
     static char tranfid[NTERMS][UIDSIZ];
     static long tikreq[NTERMS];
     struct usracc acctmp,*accptr;
     long treq,ptreq;
     char *tfid;
     static int prmts[]={
          NEWUSN,NEWAD1,NEWAD2,NEWAD3,NEWPHO,
          NEWSYT,NEWSWD,NEWAGE,NEWSEX,NEWPSW,CREDTR
     };
 
     setmbk(supmb);
     newstt=usrptr->substt;
     if (margc == 0) {
          prfmsg(newstt);
     }
     else if (margc == 1 && sameas(margv[0],"x")) {
          return(0);
     }
     else {
          rstrin();
          if (margc == 1 && (sameas(margv[0],"?") || sameas(margv[0],"help"))) {
               newstt=0;
          }
          switch (newstt) {
          case 0:
               shwusr(usaptr,0);
               prfmsg(newstt=CHGORX);
               break;
          case CHGORX:
               if (margc == 1 && (c=atoi(margv[0])) >= 1 && c <= 11) {
                    if (c == 11 && usaptr->tckavl <= 0) {
                         prfmsg(NOCRDX);
                         prfmsg(CHGORX);
                    }
                    else {
                         prfmsg(newstt=prmts[c-1]);
                    }
               }
               else {
                    prfmsg(ONLERR);
                    prfmsg(CHGORX);
               }
               break;
          case NEWUSN:
               if (uinfok(NEWUSN,5,NADSIZ)) {
                    strcpy(usaptr->usrnam,input);
                    prfmsg(newstt=CHGORX);
               }
               break;
          case NEWAD1:
               if (uinfok(NEWAD1,0,NADSIZ)) {
                    strcpy(usaptr->usrad1,input);
                    prfmsg(newstt=CHGORX);
               }
               break;
          case NEWAD2:
               if (uinfok(NEWAD2,5,NADSIZ)) {
                    strcpy(usaptr->usrad2,input);
                    prfmsg(newstt=CHGORX);
               }
               break;
          case NEWAD3:
               if (uinfok(NEWAD3,5,NADSIZ)) {
                    strcpy(usaptr->usrad3,input);
                    prfmsg(newstt=CHGORX);
               }
               break;
          case NEWPHO:
               if (uinfok(NEWPHO,7,PHOSIZ)) {
                    strcpy(usaptr->usrpho,input);
                    prfmsg(newstt=CHGORX);
               }
               break;
          case NEWSYT:
               if (unumok(NEWSYT,0,7)) {
                    usaptr->systyp=atoi(input);
                    prfmsg(newstt=CHGORX);
               }
               break;
          case NEWSWD:
               if (unumok(NEWSWD,20,136)) {
                    btutsw(usrnum,atoi(input));
                    usaptr->scnwid=atoi(input);
                    prfmsg(newstt=CHGORX);
               }
               break;
          case NEWAGE:
               if (unumok(NEWAGE,5,99)) {
                    usaptr->age=atoi(input);
                    prfmsg(newstt=CHGORX);
               }
               break;
          case NEWSEX:
               if (usexok()) {
                    usaptr->sex=atoi(input);
                    prfmsg(newstt=CHGORX);
               }
               break;
          case NEWPSW:
               if (uinfok(GPSWORD,1,PSWSIZ)) {
                    strcpy(usaptr->psword,input);
                    prfmsg(PSWWRN);
                    prfmsg(newstt=CHGORX);
               }
               break;
          case CREDTR:
               if (usaptr->tckavl <= 0) {
                    prfmsg(RANOUT);
                    prfmsg(newstt=CHGORX);
                    break;
               }
               if (!numeok()) {
                    prfmsg(NUMBAD,ltoa(usaptr->tckavl-1));
                    prfmsg(CREDTR);
                    break;
               }
               else if ((tikreq[usrnum]=atol(margv[0])) >= usaptr->tckavl) {
                     prfmsg(NOTENO,ltoa(usaptr->tckavl-1));
                     prfmsg(CREDTR);
                     break;
               }
               prfmsg(newstt=UIDEN);
               break;
          case UIDEN:
               if (usaptr->tckavl <= 0) {
                    prfmsg(RANOUT);
                    prfmsg(newstt=CHGORX);
                    break;
               }
               makhdl();
               setbtv(accbb);
               if (sameas(usaptr->userid,input,0)) {
                    prfmsg(NOTSELF);
                    prfmsg(UIDEN);
                    break;
               }
               else if (!acqbtv(&acctmp,input,0)) {
                    prfmsg(UIDBAD);
                    prfmsg(UIDEN);
                    break;
               }
               movmem(input,tranfid[usrnum],UIDSIZ);
               prfmsg(QYNPRE,ltoa(tikreq[usrnum]),usaptr->userid,input);
               prfmsg(newstt=QUERYN);
               break;
          case QUERYN:
               if (usaptr->tckavl <= 0) {
                    prfmsg(RANOUT);
                    prfmsg(newstt=CHGORX);
                    break;
               }
               if (tolower(*margv[0]) == 'y') {
                    setbtv(accbb);
                    tfid=tranfid[usrnum];
                    if (!acqbtv(&acctmp,tfid,0)) {
                         prfmsg(ACCDEL,tfid);
                         prfmsg(newstt=CHGORX);
                         break;
                    }
                    treq=tikreq[usrnum];
                    if (usaptr->tckavl <= treq) {
                         treq=usaptr->tckavl-1;
                         prfmsg(NOENOT,ltoa(treq));
                    }
                    accptr=((online=onsys(tfid)) ? othuap : &acctmp);
                    usaptr->tckavl-=treq;
                    usaptr->tcktot-=treq;
                    ptreq=min(usaptr->tckpai,treq);
                    usaptr->tckpai-=ptreq;
                    accptr->tckavl+=treq;
                    accptr->tcktot+=treq;
                    accptr->tckpai+=ptreq;
                    updbtv(accptr);
                    prfmsg(DATATR,ltoa(treq),usaptr->userid,tfid);
                    outprf(usrnum);
                    if (online && accptr->tckavl > 0) {
                         othusp->class=PAYING;
                         prfmsg(SENDUS,usaptr->userid,ltoa(treq));
                         injoth();
                         prfmsg(NOTICE,tfid);
                    }
                    if (accptr->tckpai == treq) {
                         sv.numpai+=1;
                    }
                    shocst(1,"XF %-9s%9s->%-9s",ltoa(treq),usaptr->userid,tfid);
               }
               else {
                    prfmsg(CDXABO);
               }
               prfmsg(newstt=CHGORX);
               break;
          }
     }
     outprf(usrnum);
     usrptr->substt=newstt;
     return(1);
}
 
shwusr(usaptr,sysop)
struct usracc *usaptr;
int sysop;
{
     FILE *savmb,*curmbk();
     char *ltoa();
 
     savmb=curmbk();
     setmbk(supmb);
     prfmsg(UINFEDI,usaptr->userid,
                  usaptr->credat,
                  usaptr->usedat,
                  usaptr->usrnam,
                  usaptr->usrad1,
                  usaptr->usrad2,
                  usaptr->usrad3,
                  usaptr->usrpho,
                  sysstg[usaptr->systyp],
                  usaptr->scnwid,
                  usaptr->age,
                  usaptr->sex,
                  (sysop ? usaptr->psword :
                           "<not displayed, for security reasons>"),
                  ltoa(usaptr->tckavl));
     setmbk(savmb);
}
 
char *ltoa(longin)
long longin;
{
     static char tkastg[12];
 
     sprintf(tkastg,"%ld",longin);
     return(tkastg);
}
 
numeok()
{
     char *inpptr;
     long atol();
 
     if (margc > 1 || strlen(margv[0]) > MAXTKC) {
          return(0);
     }
     for (inpptr=margv[0] ; isdigit(*inpptr) ; inpptr++) {
     }
     if (*inpptr != '\0') {
          return(0);
     }
     if (atol(margv[0]) < 2L) {
          return(0);
     }
     return(1);
}
 
