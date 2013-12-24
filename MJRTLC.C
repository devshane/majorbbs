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
 *   MJRTLC.C                                                              *
 *                                                                         *
 *   This is the Major BBS default teleconference handler.                 *
 *                                                                         *
 *                                            - T. Stryker 6/29/86         *
 *                                                                         *
 ***************************************************************************/
 
#include "stdio.h"
#include "majorbbs.h"
#include "usracc.h"
#include "mjrtlc.h"
 
int initlc(),telecn(),dfsthn(),tlchup(),clstlc();
 
#define TLCSTT      01        /* teleconferencing state               */
struct module module01={      /* module interface block               */
     'T',                     /*    main menu select character        */
     "Teleconferencing",      /*    description for main menu         */
     &initlc,                 /*    system initialization routine     */
     NULL,                    /*    user logon supplemental routine   */
     &telecn,                 /*    input routine if selected         */
     &dfsthn,                 /*    status-input routine if selected  */
     &tlchup,                 /*    hangup (lost carrier) routine     */
     NULL,                    /*    midnight cleanup routine          */
     NULL,                    /*    delete-account routine            */
     &clstlc                  /*    finish-up (sys shutdown) routine  */
};
 
static
FILE *tlcmb,*opnmsg();        /* teleconf named-message file block pointer */
static
struct tlc {                  /* teleconference per-user volatile data     */
     int nopage;              /*   incoming-paging-disabled flag           */
     int paged;               /*   intervals-since-last-paged counter      */
     int inpcnt;              /*   input message counter for freeloaders   */
} tlclst[NTERMS];             /* one to a customer                         */
static
int tlccnt;                   /* number of telecon users at the moment     */
 
#define NPAYMX 10             /* max times can talk per session if no pay  */
 
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
 
initlc()
{
     int tlctck();
 
     tlcmb=opnmsg("mjrtlc.mcv");
     rtkick(10,&tlctck);
}
 
telecn()
{
     setmbk(tlcmb);
     switch (usrptr->substt) {
     case 0:
          prfmsg(ENTTLC,usaptr->userid);
          outtlc();
          prfmsg(INTRO);
          tlccnt+=1;
          tlcctx();
          btupmt(usrnum,':');
          usrptr->substt=1;
          break;
     case 1:
          if (margc == 0) {
               tlcctx();
               break;
          }
          if (margc == 1 && (sameas(margv[0],"?")
                || sameas(margv[0],"'?'")
                || sameas(margv[0],"help"))) {
               prfmsg(TLCHLP);
               break;
          }
          if (sameas(margv[0],"page") && margc < 3) {
               page();
               break;
          }
          if (margc == 1 &&
            (sameas(margv[0],"exit") || sameas(margv[0],"x"))) {
               prfmsg(LVITLC,usaptr->userid);
               outtlc();
               prfmsg(EXITLC);
               btupmt(usrnum,0);
               tlccnt-=1;
               return(0);
          }
          if (usrptr->pfnacc > MAXPFN) {
               btucmd(usrnum,"Dp");
               prfmsg(PFNWRD);
               break;
          }
          if (pfnlvl >= 2 && usrptr->pfnacc > WRNPFN) {
               prfmsg(RAUNCH);
               break;
          }
          if (pfnlvl > 2) {
               prfmsg(PFNWRD);
               break;
          }
          if (usrptr->class < PAYING && (tlclst[usrnum].inpcnt)++ >= NPAYMX) {
               prfmsg(NPAYXC);
               break;
          }
          if (sameas(margv[0],"whisper")) {
               whisper();
               break;
          }
          rstrin();
          prf("***\rFrom %s: %s\r",usaptr->userid,input);
          outtlc();
          if (tlccnt == 1) {
               prfmsg(BYSELF);
          }
          else {
               prf("-- Message sent --\r");
          }
          break;
     }
     outprf(usrnum);
     return(1);
}
 
tlcctx()
{
     char *tlsrui(),*onebck,*curguy;
 
     initls();
     switch (tlccnt) {
     case 1:
          prfmsg(BYSELF);
          break;
     case 2:
          prfmsg(ONEOTH,tlsrui());
          break;
     case 3:
          prfmsg(TWOOTH,tlsrui(),tlsrui());
          break;
     default:
          prf("\r");
          onebck=tlsrui();
          while ((curguy=tlsrui()) != NULL) {
               prf("%s, ",onebck);
               onebck=curguy;
          }
          prfmsg(SEVOTH,onebck);
     }
     prfmsg(IROEPI);
}
 
page()
{
     char *heshe();
 
     if (margc != 2) {
          prfmsg(PAGFMT);
     }
     else if (!onsys(margv[1])) {
          if (sameas(margv[1],"on")) {
               tlclst[usrnum].nopage=0;
               prfmsg(PAGTON);
          }
          else if (sameas(margv[1],"off")) {
               tlclst[usrnum].nopage=1;
               prfmsg(PAGTOF);
          }
          else {
               prfmsg(PAGNON,margv[1]);
          }
     }
     else if (othusp->state == TLCSTT) {
          prfmsg(PAGATC,othuap->userid,heshe(othusn));
     }
     else if (tlclst[othusn].nopage) {
          prfmsg(PAGOFF,othuap->userid);
     }
     else if (tlclst[othusn].paged) {
          prfmsg(PAGL2M,othuap->userid);
     }
     else {
          tlclst[othusn].paged=8;
          prfmsg(PAGMSG,usaptr->userid);
          injoth();
          prfmsg(PAGEOK,othuap->userid);
     }
}
 
char *heshe(usr)
int usr;
{
     return(usracc[usr].sex == 'M' ? "he" : "she");
}
 
whisper()
{
     if (margc < 4) {
          prfmsg(WHSFMT);
     }
     else if (!instat(margv[2],TLCSTT)) {
          prfmsg(WHSNHR,margv[2]);
     }
     else {
          rstrin();
          prfmsg(WHSTO,usaptr->userid,margv[3]);
          outprf(othusn);
          prf("-- Message sent only to %s --\r",othuap->userid);
     }
}
 
outtlc()
{
     for (othusn=0,othusp=user ; othusn < NTERMS ; othusn++,othusp++) {
          if (othusn != usrnum && othusp->state == TLCSTT) {
               outprf(othusn);
          }
     }
     clrprf();
}
 
tlctck()
{
     struct tlc *tlcptr;
 
     for (othusn=0,tlcptr=tlclst ; othusn < NTERMS ; othusn++,tlcptr++) {
          if (tlcptr->paged != 0) {
               tlcptr->paged-=1;
          }
     }
     rtkick(15,&tlctck);
}
 
initls()
{
     othusn=-1;
     othusp=user-1;
     othuap=usracc-1;
}
 
char *tlsrui()
{
     while (othusn < NTERMS) {
          othusn+=1;
          othusp+=1;
          othuap+=1;
          if (othusp->state == TLCSTT && othusn != usrnum) {
               return(othuap->userid);
          }
     }
     return(NULL);
}
 
tlchup()
{
     if (usrptr->state == TLCSTT) {
          setmbk(tlcmb);
          prfmsg(TLCHUP,usaptr->userid);
          outtlc();
          tlccnt-=1;
     }
     setmem(&tlclst[usrnum],sizeof(struct tlc),0);
}
 
clstlc()
{
     clsmsg(tlcmb);
}
