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
 *   EMAIL.C                                                               *
 *                                                                         *
 *   This is the Major BBS electronic mail service handler.                *
 *                                                                         *
 *                                            - T. Stryker 9/23/86         *
 *                                                                         *
 ***************************************************************************/
 
#include "stdio.h"
#include "ctype.h"
#include "majorbbs.h"
#include "usracc.h"
#include "btvstf.h"
#include "email.h"
 
int inieml(),ealert(),email(),dfsthn(),emlmcu(),clseml();
 
#define EMLSTT      11        /* EMAIL state                          */
struct module module11={      /* module interface block               */
     'E',                     /*    main menu select character        */
     "Electronic Mail",       /*    description for main menu         */
     &inieml,                 /*    system initialization routine     */
     &ealert,                 /*    user logon supplemental routine   */
     &email,                  /*    input routine if selected         */
     &dfsthn,                 /*    status-input routine if selected  */
     NULL,                    /*    hangup (lost carrier) routine     */
     &emlmcu,                 /*    midnight cleanup routine          */
     NULL,                    /*    delete-account routine            */
     &clseml                  /*    finish-up (sys shutdown) routine  */
};
 
static
FILE *emlmb,*opnmsg();        /* email named-message file block pointer    */
static
BTVFILE *emlbb;               /* email btrieve message file block pointer  */
 
#define TPCSIZ 41             /* email message maximum topic size          */
#define MAXVBT 927            /* maximum variable-length text size         */
#define EMLTCK 150            /* "ticks" (credits) eaten per message       */
 
struct emlstf {               /* email message record structure on disk    */
     unsigned msgno;          /*   message number (from numero())          */
     char whofrm[UIDSIZ];     /*   user-id of creator                      */
     char whoto[UIDSIZ];      /*   user-id of recipient, or "ALL"          */
     char topic[TPCSIZ];      /*   message topic                           */
     char credat[DATSIZ];     /*   message creation date                   */
     char vbltxt[MAXVBT];     /*   variable-length message text            */
};
static
struct emlusr {               /* email user data                           */
     long fpos;               /*   file position                           */
     int keynum;              /*   key number in use                       */
     int vtxlen;              /*   size of variable text area during build */
     int lineno;              /*   line number to be modified              */
     struct emlstf eblk;      /*   email message data block                */
} *emlusr,                    /* dynamically allocated emlusr array        */
  *emlptr;                    /* ptr to email data for current user        */
static
int largv0,                   /* length of margv[0], first word in input   */
    newstt;                   /* new substate for this state               */
 
extern
BTVFILE *accbb;               /* user account btrieve file block pointer   */
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
 
extern int othusn;            /* general purpose other-user channel number */
extern struct user *othusp;   /* gen purp other-user user structure ptr    */
extern struct usracc *othuap; /* gen purp other-user accounting data ptr   */
 
inieml()
{
     char *alcmem();
 
     emlusr=(struct emlusr *)alcmem(NTERMS*sizeof(struct emlusr));
     emlmb=opnmsg("email.mcv");
     emlbb=opnbtv("email.dat",sizeof(struct emlstf));
}
 
ealert()
{
     setbtv(emlbb);
     if (qeqbtv(usaptr->userid,2)) {
          prf("\7There is E-MAIL waiting for you!\r");
     }
}
 
email()
{
     int tl;
     char *point1,*point2,*startof();
     static char allstg[UIDSIZ]={"ALL"};
 
     setmbk(emlmb);
     setbtv(emlbb);
     newstt=usrptr->substt;
     emlptr=&(emlusr[usrnum]);
     if (margc == 0) {
          rfrshs(newstt);
     }
     else if (margc == 1 && sameas(margv[0],"x")) {
          if (newstt == REPRMT) {
               prfmsg(MAINEX);
               return(0);
          }
          prfmsg(EX2MAI);
          prfmsg(newstt=REPRMT);
     }
     else if (usrptr->pfnacc > MAXPFN) {
          btucmd(usrnum,"D");
          errmsg(PFNMSG);
     }
     else if (pfnlvl > 2) {
          errmsg(PFNMSG);
     }
     else if (pfnlvl == 2) {
          errmsg(RAUNCH);
     }
     else {
          largv0=strlen(margv[0]);
          switch (newstt) {
          case 0:
               prfmsg(INTRO);
               newstt=REPRMT;
               break;
          case REPRMT:
               if (margc == 1 && largv0 == 1) {
                    switch (toupper(*margv[0])) {
                    case 'G':
                         errmsg(GENINF);
                         break;
                    case 'R':
                         prfmsg(newstt=REWHAT);
                         break;
                    case 'W':
                         if (usaptr->tckavl <= EMLTCK) {
                              blwoff(NOLIVE);
                         }
                         else {
                              usaptr->tckavl-=EMLTCK;
                              prfmsg(newstt=WHOTO);
                         }
                         break;
                    case 'M':
                         if (usrptr->class < PAYING) {
                              blwoff(NOMOD);
                         }
                         else {
                              prfmsg(newstt=MODWHT);
                         }
                         break;
                    case 'D':
                         prfmsg(newstt=DELMSN);
                         break;
                    case '?':
                         prfmsg(INTRO);
                         break;
                    default:
                         errmsg(CNOTIL);
                    }
               }
               else {
                    errmsg(JSTONE);
               }
               break;
          case REWHAT:
               if (margc != 1) {
                    errmsg(REWPLS);
                    break;
               }
               if (isdigit(*margv[0])) {
                    emlptr->eblk.msgno=atoi(margv[0]);
                    if (!acqbtv(&(emlptr->eblk),&(emlptr->eblk.msgno),0)) {
                         errmsg(NSUCHN);
                         break;
                    }
                    if (!(sameas(emlptr->eblk.whofrm,usaptr->userid)
                       || sameas(emlptr->eblk.whoto ,usaptr->userid)
                       || sameas(emlptr->eblk.whoto ,"ALL")
                       || sameas("Sysop",usaptr->userid))) {
                         errmsg(NOREAD,emlptr->eblk.msgno);
                         break;
                    }
                    prf("\r");
                    sumams();
                    prf("%s\r",emlptr->eblk.vbltxt);
                    prfmsg(newstt=REPRMT);
               }
               else if (largv0 == 1) {
                    switch (toupper(*margv[0])) {
                    case 'T':
                         bgnrd(usaptr->userid,2,NOMSGT);
                         break;
                    case 'F':
                         bgnrd(usaptr->userid,1,NOMSGF);
                         break;
                    case 'A':
                         bgnrd(allstg,2,NOMSGA);
                         break;
                    default:
                         errmsg(REWPLS);
                    }
               }
               else {
                    errmsg(REWPLS);
               }
               break;
          case SCOSCN:
               if (margc == 1 && largv0 == 1) {
                    estabc();
                    switch (toupper(*margv[0])) {
                    case 'N':
                         nxteml();
                         break;
                    case 'R':
                         prf("%s\r",emlptr->eblk.vbltxt);
                         if (sameas(usaptr->userid,emlptr->eblk.whofrm)
                          || sameas(usaptr->userid,emlptr->eblk.whoto)) {
                              prfmsg(newstt=DELGUY);
                         }
                         else {
                              prf("\r");
                              prfmsg(SCOSCN);
                         }
                         break;
                    default:
                         errmsg(CNOTIL);
                    }
               }
               else {
                    errmsg(JSTONE);
               }
               break;
          case DELGUY:
               switch (toupper(*margv[0])) {
               case 'D':
                    prfmsg(DELING);
                    estabc();
                    if (confli()) {
                         prfmsg(USING);
                         prfmsg(KEEPIN);
                    }
                    else {
                         delbtv();
                    }
                    nxteml();
                    break;
               case 'K':
                    prfmsg(KEEPIN);
                    estabc();
                    nxteml();
                    break;
               default:
                    prfmsg(DELGUY);
               }
               break;
          case WHOTO:
               makhdl();
               if (sameas(input,"All")) {
                    strcpy(input,"ALL");
               }
               setbtv(accbb);
               if (sameas(input,"ALL") || qeqbtv(input,0)) {
                    movmem(input,emlptr->eblk.whoto,UIDSIZ);
                    prfmsg(newstt=GIVTOP,TPCSIZ-1);
               }
               else {
                    errmsg(UIDNXI);
               }
               break;
          case GIVTOP:
               rstrin();
               input[TPCSIZ-1]='\0';
               strcpy(emlptr->eblk.topic,input);
               prfmsg(ENTTXT,MAXVBT-1);
               newstt=CONTXT;
               emlptr->eblk.vbltxt[0]='\0';
               emlptr->vtxlen=0;
               break;
          case CONTXT:
               rstrin();
               if (!sameas(input,"OK")) {
                    if (inplen >= (tl=MAXVBT-(emlptr->vtxlen)-2)) {
                         prfmsg(LLTRUN,MAXVBT-1);
                         input[tl]='\0';
                         ademln();
                    }
                    else if (inplen >= tl-5) {
                         ademln();
                    }
                    else {
                         ademln();
                         return(1);
                    }
               }
               if (emlptr->vtxlen != 0) {
                    prfmsg(newstt=RUSAT);
               }
               else {
                    blwoff(NOMSGE);
               }
               break;
          case RUSAT:
               switch (tolower(*margv[0])) {
               case 'y':
                    makeml();
                    prfmsg(WCONFM,emlptr->eblk.msgno);
                    prfmsg(newstt=REPRMT);
                    break;
               case 'n':
                    makeml();
                    dmsg4m();
                    emlptr->fpos=absbtv();
                    prfmsg(INDLCH);
                    newstt=RAFTLN;
                    break;
               default:
                    errmsg(YORN);
               }
               break;
          case MODWHT:
               emlptr->eblk.msgno=atoi(margv[0]);
               if (!acqbtv(&(emlptr->eblk),&(emlptr->eblk.msgno),0)) {
                    blwoff(NSUCHN);
                    break;
               }
               if (!(sameas(emlptr->eblk.whofrm,usaptr->userid)
                  || sameas("Sysop",usaptr->userid))) {
                    blwoff(UDONTO,emlptr->eblk.msgno);
                    break;
               }
               dmsg4m();
               emlptr->fpos=absbtv();
               prfmsg(INDLCH);
               newstt=RAFTLN;
               break;
          case RAFTLN:
               if (margc == 1) {
                    switch (tolower(*margv[0])) {
                    case 'r':
                         dmsg4m();
                         prfmsg(RAFTLN);
                         break;
                    case 'a':
                         gabbtv(NULL,emlptr->fpos,0);
                         updbtv(&emlptr->eblk);
                         blwoff(WCONFM,emlptr->eblk.msgno);
                         break;
                    case 'f':
                         prfmsg(newstt=MODRCP);
                         break;
                    case 't':
                         prfmsg(newstt=MODTPC);
                         break;
                    default:
                         if (isdigit(*margv[0])) {
                              emlptr->lineno=atoi(margv[0]);
                              prfmsg(newstt=MODLIN,emlptr->lineno,
                                   emlptr->vtxlen=nallow(emlptr->lineno));
                         }
                         else {
                              errmsg(CNOTIL);
                         }
                    }
               }
               else {
                    errmsg(JSTONE);
               }
               break;
          case MODRCP:
               makhdl();
               if (sameas(input,"All")) {
                    strcpy(input,"ALL");
               }
               setbtv(accbb);
               if (sameas(input,"ALL") || qeqbtv(input,0)) {
                    movmem(input,emlptr->eblk.whoto,UIDSIZ);
                    prfmsg(newstt=RAFTLN);
               }
               else {
                    errmsg(MODUNX);
               }
               break;
          case MODTPC:
               rstrin();
               input[TPCSIZ-1]='\0';
               strcpy(emlptr->eblk.topic,input);
               prfmsg(newstt=RAFTLN);
               break;
          case MODLIN:
               rstrin();
               if (inplen > emlptr->vtxlen) {
                    input[inplen=emlptr->vtxlen]='\0';
                    prfmsg(MODTOB);
               }
               point1=startof(emlptr->lineno);
               point2=startof(emlptr->lineno+1);
               movmem(point2,point1+inplen+1,strlen(point2)+1);
               *point1='\r';
               movmem(input,point1+1,inplen);
               prfmsg(newstt=RAFTLN);
               break;
          case DELMSN:
               emlptr->eblk.msgno=atoi(margv[0]);
               if (!acqbtv(&(emlptr->eblk),&(emlptr->eblk.msgno),0)) {
                    blwoff(NSUCHN);
                    break;
               }
               if (!(sameas(emlptr->eblk.whofrm,usaptr->userid)
                  || sameas(emlptr->eblk.whoto ,usaptr->userid)
                  || sameas("Sysop",usaptr->userid))) {
                    blwoff(UCANTD,emlptr->eblk.msgno);
                    break;
               }
               if (confli()) {
                    blwoff(USING);
                    break;
               }
               delbtv();
               blwoff(OKGONE,emlptr->eblk.msgno);
               break;
          default:
               catastro("EMAIL: usrptr->substt is %d for user %d",
                    usrptr->substt,usrnum);
          }
     }
     outprf(usrnum);
     usrptr->substt=newstt;
     return(1);
}
 
bgnrd(keyval,keynum,nfndn)
char *keyval;
int keynum,nfndn;
{
     if (!acqbtv(&(emlptr->eblk),keyval,keynum)) {
          blwoff(nfndn);
     }
     else {
          emlptr->keynum=keynum;
          sumams();
          prfmsg(newstt=SCOSCN);
     }
}
 
estabc()
{
     gabbtv(&(emlptr->eblk),emlptr->fpos,emlptr->keynum);
}
 
nxteml()
{
     if (!aqnbtv(&(emlptr->eblk))) {
          blwoff(SCOEND);
     }
     else {
          sumams();
          prfmsg(newstt=SCOSCN);
     }
}
 
sumams()
{
     prf("#%05u %5.5s %-9s to %-9s RE: %s\n",
          emlptr->eblk.msgno,emlptr->eblk.credat,
          emlptr->eblk.whofrm,emlptr->eblk.whoto,emlptr->eblk.topic);
     emlptr->fpos=absbtv();
}
 
ademln()
{
     emlptr->eblk.vbltxt[emlptr->vtxlen]='\r';
     strcpy(&emlptr->eblk.vbltxt[emlptr->vtxlen+1],input);
     emlptr->vtxlen+=1+inplen;
}
 
makeml()
{
     emlptr->eblk.msgno=numero();
     setbtv(emlbb);
     date(emlptr->eblk.credat,2);
     movmem(usaptr->userid,emlptr->eblk.whofrm,UIDSIZ);
     insbtv(&(emlptr->eblk));
}
 
dmsg4m()
{
     char *msgptr,*linptr;
     int lineno,tmpchr;
 
     prfmsg(MODCHC,emlptr->eblk.msgno,emlptr->eblk.whoto,emlptr->eblk.topic);
     for (lineno=0,msgptr=emlptr->eblk.vbltxt ; *msgptr != '\0' ; ) {
          for (linptr=msgptr ; *linptr != '\r' && *linptr != '\0' ; linptr++) {
          }
          tmpchr=*linptr;
          *linptr='\0';
          prf("%2d. %s\r",lineno++,msgptr);
          if ((*linptr=tmpchr) == '\0') {
               break;
          }
          msgptr=linptr+1;
     }
     prf("%2d.\r",lineno);
}
 
nallow(lineno)
int lineno;
{
     int i,c,count;
     char *msgptr;
 
     for (i=0,msgptr=emlptr->eblk.vbltxt ; i < lineno ; i++) {
          while ((c=*msgptr++) != '\0' && c != '\r') {
          }
          if (c == '\0') {
               return(min(MAXVBT-(msgptr-(emlptr->eblk.vbltxt))-1,INPSIZ-1));
          }
     }
     for (count=0 ; (c=*msgptr++) != '\0' && c != '\r' ; count++) {
     }
     return(min(MAXVBT-(msgptr-(emlptr->eblk.vbltxt))-1+count,INPSIZ-1));
}
 
char *startof(lineno)
int lineno;
{
     int i,c;
     char *msgptr;
 
     msgptr=emlptr->eblk.vbltxt;
     if (lineno < 2) {
          return(msgptr);
     }
     for (i=0 ; i < lineno ; i++) {
          while ((c=*msgptr++) != '\0' && c != '\r') {
          }
          if (c == '\0') {
               break;
          }
     }
     return(msgptr-1);
}
 
static
confli()
{
     long recpos;
 
     recpos=absbtv();
     for (othusn=0,othusp=user ; othusn < NTERMS ; othusn++,othusp++) {
          if (othusn != usrnum
           && othusp->class > SUPIPG
           && othusp->state == EMLSTT
           && othusp->substt != REPRMT
           && emlusr[othusn].fpos == recpos) {
               return(1);
          }
     }
     return(0);
}
 
static
errmsg(msgnum,parm)
int msgnum;
long parm;
{
     prfmsg(msgnum,parm);
     rfrshs(usrptr->substt);
}
 
rfrshs(state)
int state;
{
     if (state == MODLIN) {
          prfmsg(MODLIN,emlptr->lineno,emlptr->vtxlen);
     }
     else {
          prfmsg(state);
     }
}
 
static
blwoff(msgnum,parm)
int msgnum;
long parm;
{
     prfmsg(msgnum,parm);
     prfmsg(newstt=REPRMT);
}
 
emlmcu()
{
     char *curdat();
     int curmon,curday,admon,adday;
 
     sscanf(curdat(2),"%d/%d",&curmon,&curday);
     setbtv(emlbb);
     if (qlobtv(0)) {
          do {
               gcrbtv(&emlusr->eblk,0);
               admon=adday=0;
               sscanf(emlusr->eblk.credat,"%d/%d",&admon,&adday);
               if ((admon == curmon && adday < curday-14)
                 || (admon != curmon && adday < curday+31-14)) {
                    delbtv();
               }
          } while (qnxbtv());
     }
}
 
clseml()
{
     clsmsg(emlmb);
     clsbtv(emlbb);
}
