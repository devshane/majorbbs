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
 *   CLASSI.C                                                              *
 *                                                                         *
 *   This is the Major BBS classified-ad service handler.                  *
 *                                                                         *
 *                                            - T. Stryker 7/2/86          *
 *                                                                         *
 ***************************************************************************/
 
#include "stdio.h"
#include "ctype.h"
#include "majorbbs.h"
#include "usracc.h"
#include "btvstf.h"
#include "classi.h"
 
int inicsi(),classi(),dfsthn(),csimcu(),clscsi();
 
#define CSISTT      10        /* classified ad state                  */
struct module module10={      /* module interface block               */
     'C',                     /*    main menu select character        */
     "Classified ads",        /*    description for main menu         */
     &inicsi,                 /*    system initialization routine     */
     NULL,                    /*    user logon supplemental routine   */
     &classi,                 /*    input routine if selected         */
     &dfsthn,                 /*    status-input routine if selected  */
     NULL,                    /*    hangup (lost carrier) routine     */
     &csimcu,                 /*    midnight cleanup routine          */
     NULL,                    /*    delete-account routine            */
     &clscsi                  /*    finish-up (sys shutdown) routine  */
};
 
static
FILE *csimb,*opnmsg();        /* classi named-message file block pointer   */
static
BTVFILE *adbb,                /* btrieve ad text file block pointer        */
        *rspbb;               /* btrieve response text file block pointer  */
 
#define TPCSIZ 20             /* maximum ad topic length                   */
#define HDRSIZ 40             /* maximum ad header-line length             */
#define ADLSIZ 80             /* maximum ad body line length               */
#define RSLSIZ 40             /* maximum response text length              */
 
struct csiad {                     /* classified ad btrieve record layout  */
     unsigned adno;                /*   ad number from numero()            */
     char topic[TPCSIZ];           /*   ad topic                           */
     char advrtr[UIDSIZ];          /*   advertiser's user-id               */
     char header[HDRSIZ];          /*   ad header line                     */
     char line1[ADLSIZ];           /*   body of ad lines                   */
     char line2[ADLSIZ];
     char line3[ADLSIZ];
     char line4[ADLSIZ];
     char credat[DATSIZ];          /*   ad creation date                   */
};
 
struct csirsp {                    /* response btrieve file record layout  */
     char resper[UIDSIZ];          /*   responder's user-id                */
     unsigned adno;                /*   ad number this is in response to   */
     char advrtr[UIDSIZ];          /*   user-id who placed that ad         */
     char rspdat[DATSIZ];          /*   date of response                   */
     char rsptxt[RSLSIZ];          /*   text of response                   */
};
static
struct csiusr {                    /* classified ad user data block        */
     long fpos;                    /*   current file position              */
     struct csiad adblk;           /*   ad data block                      */
     struct csirsp rspblk;         /*   response data block                */
} *csiusr,                    /* dynamically allocated user data array     */
  *csiptr;                    /* ptr to classi ad data for current user    */
static
int largv0,                   /* length of margv[0], first word in input   */
    newstt;                   /* new substate for this state               */
 
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
 
inicsi()
{
     char *alcmem();
 
     csiusr=(struct csiusr *)alcmem(NTERMS*sizeof(struct csiusr));
     csimb=opnmsg("classi.mcv");
     adbb=opnbtv("classads.dat",sizeof(struct csiad));
     rspbb=opnbtv("classrsp.dat",sizeof(struct csirsp));
}
 
classi()
{
     setmbk(csimb);
     newstt=usrptr->substt;
     if (margc == 0 && newstt != PRSRET) {
          prfmsg(newstt);
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
          csiptr=&(csiusr[usrnum]);
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
                    case 'S':
                         prfmsg(newstt=SCORDA);
                         break;
                    case 'P':
                         place();
                         break;
                    case 'M':
                         prfmsg(newstt=MODIFA);
                         break;
                    case 'D':
                         prfmsg(newstt=DELADN);
                         break;
                    case 'C':
                         crr1();
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
          case SCORDA:
               setbtv(adbb);
               if (sameas(margv[0],"?")) {
                    errmsg(SCOHLP);
                    break;
               }
               if (margc != 1 || !isdigit(*margv[0])) {
                    rstrin();
                    cpykey(csiptr->adblk.topic,margv[0],TPCSIZ);
                    if (qgebtv(csiptr->adblk.topic,1)
                     || qltbtv(csiptr->adblk.topic,1)) {
                         gcrbtv(&(csiptr->adblk),1);
                    }
                    else {
                         blwoff(NOADS);
                         break;
                    }
               }
               else {
                    csiptr->adblk.adno=atoi(margv[0]);
                    if (!acqbtv(&(csiptr->adblk),&(csiptr->adblk.adno),0)) {
                         errmsg(NSUCHN);
                         break;
                    }
               }
               showad();
               prfmsg(newstt=SCOSCN);
               break;
          case SCOSCN:
               if (margc == 1 && largv0 == 1) {
                    setbtv(adbb);
                    gabbtv(&(csiptr->adblk),csiptr->fpos,1);
                    switch (toupper(*margv[0])) {
                    case 'F':
                         if (!qnxbtv()) {
                              errmsg(SCOEND);
                         }
                         else {
                              gcrbtv(&(csiptr->adblk),1);
                              showad();
                              prfmsg(SCOSCN);
                         }
                         break;
                    case 'B':
                         if (!qprbtv()) {
                              errmsg(SCOBEG);
                         }
                         else {
                              gcrbtv(&(csiptr->adblk),1);
                              showad();
                              prfmsg(SCOSCN);
                         }
                         break;
                    case 'R':
                         prf("\r%s\r%s\r%s\r%s\r",csiptr->adblk.line1,
                                                  csiptr->adblk.line2,
                                                  csiptr->adblk.line3,
                                                  csiptr->adblk.line4);
                         if (usrptr->class < PAYING) {
                              prfmsg(SCOAFR);
                              break;
                         }
                         movmem(usaptr->userid,csiptr->rspblk.resper,UIDSIZ);
                         csiptr->rspblk.adno=csiptr->adblk.adno;
                         setbtv(rspbb);
                         if (acqbtv(&csiptr->rspblk,csiptr->rspblk.resper,0)) {
                              prfmsg(SCOCRR,csiptr->rspblk.rsptxt);
                              prfmsg(newstt=SCORCQ);
                         }
                         else {
                              prfmsg(newstt=SCORSQ);
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
          case SCORCQ:
               switch (tolower(*margv[0])) {
               case 'y':
                    prfmsg(newstt=SCORSC);
                    break;
               case 'n':
                    prfmsg(SCOAFR);
                    newstt=SCOSCN;
                    break;
               default:
                    errmsg(YORN);
               }
               break;
          case SCORSQ:
               switch (tolower(*margv[0])) {
               case 'y':
                    prfmsg(newstt=SCORSP);
                    break;
               case 'n':
                    prfmsg(SCOAFR);
                    newstt=SCOSCN;
                    break;
               default:
                    errmsg(YORN);
               }
               break;
          case SCORSC:
               setbtv(rspbb);
               getbtv(NULL,csiptr->rspblk.resper,0);
               rstrin();
               cpykey(csiptr->rspblk.rsptxt,margv[0],RSLSIZ);
               date(csiptr->rspblk.rspdat,2);
               updbtv(&(csiptr->rspblk));
               prfmsg(SCOAFR);
               newstt=SCOSCN;
               break;
          case SCORSP:
               setbtv(rspbb);
               movmem(usaptr->userid,csiptr->rspblk.resper,UIDSIZ);
               csiptr->rspblk.adno=csiptr->adblk.adno;
               movmem(csiptr->adblk.advrtr,csiptr->rspblk.advrtr,UIDSIZ);
               date(csiptr->rspblk.rspdat,2);
               rstrin();
               cpykey(csiptr->rspblk.rsptxt,margv[0],RSLSIZ);
               insbtv(&(csiptr->rspblk));
               prfmsg(SCOAFR);
               newstt=SCOSCN;
               break;
          case PLCAD:
               rstrin();
               cpykey(csiptr->adblk.topic,margv[0],TPCSIZ);
               prfmsg(newstt=ADHDR);
               break;
          case ADHDR:
               rstrin();
               cpykey(csiptr->adblk.header,margv[0],HDRSIZ);
               prfmsg(newstt=TXTAD1);
               break;
          case TXTAD1:
               rstrin();
               cpykey(csiptr->adblk.line1,margv[0],ADLSIZ);
               prfmsg(newstt=TXTAD2);
               break;
          case TXTAD2:
               rstrin();
               cpykey(csiptr->adblk.line2,margv[0],ADLSIZ);
               prfmsg(newstt=TXTAD3);
               break;
          case TXTAD3:
               rstrin();
               cpykey(csiptr->adblk.line3,margv[0],ADLSIZ);
               prfmsg(newstt=TXTAD4);
               break;
          case TXTAD4:
               rstrin();
               cpykey(csiptr->adblk.line4,margv[0],ADLSIZ);
               csiptr->adblk.adno=numero();
               dispad(ADREPT);
               prfmsg(newstt=RUSATS);
               break;
          case RUSATS:
               switch (tolower(*margv[0])) {
               case 'y':
                    makad();
                    prfmsg(newstt=REPRMT);
                    break;
               case 'n':
                    makad();
                    prfmsg(INDLCH);
                    newstt=RATH14;
                    break;
               default:
                    errmsg(YORN);
               }
               break;
          case MODIFA:
               setbtv(adbb);
               csiptr->adblk.adno=atoi(margv[0]);
               if (!acqbtv(&(csiptr->adblk),&(csiptr->adblk.adno),0)) {
                    blwoff(NSUCHN);
                    break;
               }
               if (!(sameas(csiptr->adblk.advrtr,usaptr->userid)
                  || sameas("Sysop",usaptr->userid))) {
                    blwoff(UDONTO,csiptr->adblk.adno);
                    break;
               }
               dispad(MODCHC);
               prfmsg(INDLCH);
               newstt=RATH14;
               break;
          case RATH14:
               if (margc == 1 && largv0 == 1) {
                    switch (tolower(*margv[0])) {
                    case 'r':
                         dispad(MODCHC);
                         prfmsg(RATH14);
                         break;
                    case 'a':
                         blwoff(ACCPTD);
                         break;
                    case 't':
                         prfmsg(newstt=MODTPC);
                         break;
                    case 'h':
                         prfmsg(newstt=MODHDR);
                         break;
                    case '1':
                         prfmsg(newstt=MODLN1);
                         break;
                    case '2':
                         prfmsg(newstt=MODLN2);
                         break;
                    case '3':
                         prfmsg(newstt=MODLN3);
                         break;
                    case '4':
                         prfmsg(newstt=MODLN4);
                         break;
                    default:
                         errmsg(CNOTIL);
                    }
               }
               else {
                    errmsg(JSTONE);
               }
               break;
          case MODTPC:
               modutl(csiptr->adblk.topic,TPCSIZ);
               break;
          case MODHDR:
               modutl(csiptr->adblk.header,HDRSIZ);
               break;
          case MODLN1:
               modutl(csiptr->adblk.line1,ADLSIZ);
               break;
          case MODLN2:
               modutl(csiptr->adblk.line2,ADLSIZ);
               break;
          case MODLN3:
               modutl(csiptr->adblk.line3,ADLSIZ);
               break;
          case MODLN4:
               modutl(csiptr->adblk.line4,ADLSIZ);
               break;
          case DELADN:
               setbtv(adbb);
               csiptr->adblk.adno=atoi(margv[0]);
               if (!acqbtv(&(csiptr->adblk),&(csiptr->adblk.adno),0)) {
                    blwoff(NSUCHN);
                    break;
               }
               if (!(sameas(csiptr->adblk.advrtr,usaptr->userid)
                  || sameas("Sysop",usaptr->userid))) {
                    blwoff(UCANTD,csiptr->adblk.adno);
                    break;
               }
               if (confli()) {
                    blwoff(USING);
                    break;
               }
               delbtv();
               blwoff(OKGONE);
               break;
          case PRSRET:
               rsputl(" << more >> ");
               break;
          case CLRRDR:
               switch (tolower(*margv[0])) {
               case 'y':
                    zaprdr();
                    blwoff(RSPCLD);
                    break;
               case 'n':
                    blwoff(RSPNCL);
                    break;
               default:
                    errmsg(YORN);
               }
               break;
          default:
               catastro("CLASSI: usrptr->substt is %d for user %d",
                    usrptr->substt,usrnum);
          }
     }
     outprf(usrnum);
     usrptr->substt=newstt;
     return(1);
}
 
makad()
{
     setbtv(adbb);
     date(csiptr->adblk.credat,2);
     movmem(usaptr->userid,csiptr->adblk.advrtr,UIDSIZ);
     insbtv(&(csiptr->adblk));
     usaptr->csicnt+=1;
}
 
confli()
{
     long recpos;
 
     recpos=absbtv();
     for (othusn=0,othusp=user ; othusn < NTERMS ; othusn++,othusp++) {
          if (othusn != usrnum
           && othusp->class > SUPIPG
           && othusp->state == CSISTT
           && othusp->substt != REPRMT
           && csiusr[othusn].fpos == recpos) {
               return(1);
          }
     }
     return(0);
}
 
errmsg(msgnum)
int msgnum;
{
     prfmsg(msgnum);
     prfmsg(usrptr->substt);
}
 
blwoff(msgnum,parm)
int msgnum;
long parm;
{
     prfmsg(msgnum,parm);
     prfmsg(newstt=REPRMT);
}
 
showad()
{
     long absbtv();
 
     setbtv(adbb);
     prf("Ad #%u: %s ... %s\r",csiptr->adblk.adno,
                               csiptr->adblk.topic,
                               csiptr->adblk.header);
     csiptr->fpos=absbtv();
}
 
dispad(msgnum)
int msgnum;
{
     prfmsg(msgnum,csiptr->adblk.adno,
                   csiptr->adblk.topic,csiptr->adblk.header,
                   csiptr->adblk.line1,csiptr->adblk.line2,
                   csiptr->adblk.line3,csiptr->adblk.line4);
}
 
modutl(dest,size)
char *dest;
int size;
{
     rstrin();
     setbtv(adbb);
     geqbtv(&(csiptr->adblk),&(csiptr->adblk.adno),0);
     cpykey(dest,margv[0],size);
     updbtv(&(csiptr->adblk));
     prfmsg(newstt=RATH14);
}
 
place()
{
     if (usaptr->tcktot == 0) {
          blwoff(NOLIVE);
     }
     else if (usaptr->csicnt != 0
       && (usaptr->tcktot)/(usaptr->csicnt) <= 9000) {
          blwoff(QUOTEX);
     }
     else {
          prfmsg(newstt=PLCAD);
     }
}
 
crr1()
{
     setbtv(rspbb);
     if (!acqbtv(&(csiptr->rspblk),usaptr->userid,1)) {
          blwoff(NORDRS);
     }
     else {
          prfmsg(RDRRSP);
          rsputl(NULL);
     }
}
 
rsputl(morstg)
char *morstg;
{
     int i;
 
     setbtv(rspbb);
     if (morstg != NULL) {
          gabbtv(&(csiptr->rspblk),csiptr->fpos,1);
     }
     for (i=0 ; i < 4 ; i++) {
          prf(" From %-9s RE:#%u on %s: %s\r",csiptr->rspblk.resper,
              csiptr->rspblk.adno,csiptr->rspblk.rspdat,csiptr->rspblk.rsptxt);
          if (!aqnbtv(&(csiptr->rspblk))) {
               prfmsg(newstt=CLRRDR);
               return;
          }
     }
     if (morstg != NULL) {
          prf(morstg);
     }
     else {
          prfmsg(PRSRET);
     }
     csiptr->fpos=absbtv();
     newstt=PRSRET;
}
 
zaprdr()
{
     setbtv(rspbb);
     if (!acqbtv(NULL,usaptr->userid,1)) {
          catastro("ZAPRDR: NO FIRST ENTRY");
     }
     delbtv();
     while (aqnbtv(NULL)) {
          delbtv();
     }
}
 
csimcu()
{
     char *curdat();
     int curmon,curday,admon,adday;
 
     sscanf(curdat(2),"%d/%d",&curmon,&curday);
     setbtv(adbb);
     if (qlobtv(0)) {
          do {
               gcrbtv(&csiusr->adblk,0);
               admon=adday=0;
               sscanf(csiusr->adblk.credat,"%d/%d",&admon,&adday);
               if ((admon == curmon && adday < curday-7)
                 || (admon != curmon && adday < curday+31-7)) {
                    delbtv();
               }
          } while (qnxbtv());
     }
     setbtv(rspbb);
     if (qlobtv(0)) {
          do {
               gcrbtv(&csiusr->rspblk,0);
               admon=adday=0;
               sscanf(csiusr->rspblk.rspdat,"%d/%d",&admon,&adday);
               if ((admon == curmon && adday < curday-14)
                 || (admon != curmon && adday < curday+31-14)) {
                    delbtv();
               }
          } while (qnxbtv());
     }
}
 
 
clscsi()
{
     clsmsg(csimb);
     clsbtv(adbb);
     clsbtv(rspbb);
}
