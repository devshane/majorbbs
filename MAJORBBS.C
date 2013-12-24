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
 *   MAJORBBS.C                                                            *
 *                                                                         *
 *   This is the Major BBS mainline.                                       *
 *                                                                         *
 *                                            - T. Stryker 6/24/86         *
 *                                                                         *
 ***************************************************************************/
 
#include "stdio.h"
#include "ctype.h"
#include "setjmp.h"
#include "dos.h"
#include "majorbbs.h"
#include "usracc.h"
#include "majormsg.h"
 
int mainu(),dfsthn(),loscar(),midnit(),mjrfin();
 
struct module module00={      /* module interface block               */
     0,                       /*    main menu select character        */
     "Main menu",             /*    description for main menu         */
     NULL,                    /*    system initialization routine     */
     NULL,                    /*    user logon supplemental routine   */
     &mainu,                  /*    input routine if selected         */
     &dfsthn,                 /*    status-input routine if selected  */
     &loscar,                 /*    hangup (lost carrier) routine     */
     &midnit,                 /*    midnight cleanup routine          */
     NULL,                    /*    delete-account routine            */
     &mjrfin                  /*    finish-up (sys shutdown) routine  */
};
 
extern struct module          module01,module02,module03,module04,
                     module05,module06,module07,module08,module09,
                     module10,module11,module12,module13,module14,
                     module15,module16,module17,module18,module19;
struct module *module[NMODS]={
                     module00,module01,module02,module03,module04,
                     module05,module06,module07,module08,module09,
                     module10,module11,module12,module13,module14,
                     module15,module16,module17,module18,module19
};
 
int usrnum;                   /* global user-number (channel) in effect    */
struct user *usrptr,          /* global pointer to user data in effect     */
            user[NTERMS];     /* user volatile-data structure array        */
 
char input[INPSIZ],           /* raw user input data buffer                */
     *margv[INPSIZ/2],        /* array of ptrs to word starts, a la argv[] */
     *margn[INPSIZ/2];        /* array of ptrs to word ends, for rstrin()  */
 
int margc,                    /* number of words in margv[], a la argc     */
    inplen,                   /* overall raw input string length           */
    status,                   /* raw status from btusts, where appropriate */
    pfnlvl;                   /* profanity level of current input (0-3)    */
 
int othusn;                   /* general purpose other-user channel number */
struct user *othusp;          /* gen purp other-user user structure ptr    */
struct usracc *othuap;        /* gen purp other-user accounting data ptr   */
 
FILE *mjrmb;                  /* executive named-message file block ptr    */
int mbdone;                   /* main-loop exit flag                       */
int horiz;                    /* horizontal function keys flag             */
int color;                    /* color display adapter in use flag         */
 
int _stack=4096;              /* allow a little extra stack space          */
 
extern jmp_buf disaster;      /* master error-recovery longjmp save block  */
extern int ticker;            /* seconds-ticker from brkthu                */
extern
struct usracc usracc[NTERMS], /* user accounting block array               */
              *usaptr;        /* user accounting block pointer for usrnum  */
extern
struct sysvbl sv;             /* sys-variables record instance for updates */
extern
int stthue,                   /* system status display attribute code      */
    lblhue,                   /* soft-key labels display attribute code    */
    dsphue,                   /* data display box attribute code           */
    chshue,                   /* channel "special" display attribute code  */
    dshhue,                   /* channel dashes display attribute code     */
    onlhue,                   /* channel online display attribute code     */
    manhue,                   /* monitor-all normal display attribute code */
    mashue,                   /* monitor-all "special" disp attribute code */
    emthue,                   /* emulation text display attribute code     */
    emshue;                   /* emulation status display attribute code   */
 
#define RING  1               /* btusts() return for ringing/lost carrier  */
#define CMDOK 2               /* btusts() return for command completed ok  */
#define CRSTG 3               /* btusts() return for CR-term'd string avail*/
 
#define SAMPLN 3              /* top sample line user number               */
#define MCUMIN 35             /* max grace period minutes on 3AM cleanup   */
 
char *curtim(),*curdat();
 
main(argc,argv)
int argc;
char *argv[];
{
     char *getml();
     char *frzseg();
     long btulsz();
     FILE *opnmsg();
     int bsport,i;
     int ckmcu();
 
     if (setjmp(&disaster)) {
          (*module00.finrou)();
     }
     inirtk();
     horiz=(argc >= 2 && sameas(argv[1],"h"));
     color=(frzseg() == ((char *)0xB8000000L));
     if (argc >= 3 && sameas(argv[2],"i")) {
          color=!color;
     }
     bsport=BPORT;
     if (argc >= 4) {
          if (sscanf(argv[3],"%x",&bsport) != 1 || bsport < 0x200) {
               catastro("BASE PORT SPECIFICATION ERROR");
          }
     }
     inisho();
     rtkick(50,&ckmcu);
     inimod();
     mjrmb=opnmsg("majormsg.mcv");
     btuitz(getml(btulsz(NTERMS,INPSIZ,OUTSIZ)));
     for (i=0 ; i < NTERMS ; i+=16) {
          btudef(i,bsport+i/8,16);
          shocar(i/16);
     }
     for (usrnum=0 ; usrnum < NTERMS ; usrnum++) {
          rstchn();
     }
     inisup();
     iniacc();
     while (!mbdone) {
          if ((usrnum=btuscn()) >= 0) {
               if ((status=btusts(usrnum)) != 3) {
                    shomal();
               }
               usrptr=&user[usrnum];
               usaptr=&usracc[usrnum];
               switch (usrptr->class) {
               case VACANT:
                    if (status == RING) {
                         shochn(" ANSWER  ",chshue);
                         btucmd(usrnum,"Ap");
                         usrptr->class=ONLINE;
                    }
                    else {
                         rstchn();
                         shocst(0,"VACANT, GENERATED STATUS %d",status);
                    }
                    break;
               case ONLINE:
                    switch (status) {
                    case RING:
                         rstchn();
                         break;
                    case CMDOK:
                         shochn(" CARRIER ",chshue);
                         setmbk(mjrmb);
                         prfmsg(HELLO,curtim(2),curdat(7));
                         if (strlen(sv.lonmsg) != 0) {
                              prf("\r%s\r",sv.lonmsg);
                         }
                         prfmsg(usrnum > SAMPLN ? LOGUID : ENTUID);
                         outprf(usrnum);
                         break;
                    case CRSTG:
                         setmbk(mjrmb);
                         switch (usrptr->substt) {
                         case 0:
                              getin();
                              if (margc == 0) {
                                   prfmsg(usrnum > SAMPLN ? LOGUID : ENTUID);
                                   outprf(usrnum);
                              }
                              else if (usrnum <= SAMPLN
                                   && (sameas(margv[0],"new")
                                    || sameas(margv[0],"\"new\""))) {
                                   shochn(" SIGNUP  ",chshue);
                                   usrptr->class=SUPIPG;
                                   signup();
                              }
                              else if (uinsys(margv[0])) {
                                   prfmsg(usrnum > SAMPLN ? LOGDON : ALRDON);
                                   outprf(usrnum);
                              }
                              else if (loadup()) {
                                   if (usaptr->tckavl <= 0 && usrnum > SAMPLN) {
                                        shochn("DEADBEAT ",chshue+0x80);
                                        byenow(MEMONL);
                                   }
                                   else {
                                        shochn("  LOGON  ",chshue);
                                        prfmsg(ENTPSW);
                                        outprf(usrnum);
                                        usrptr->usetmr=0;
                                        btuech(usrnum,0);
                                        usrptr->substt=1;
                                        usrptr->countr=0;
                                   }
                              }
                              else if (++(usrptr->countr) < 3) {
                                   if (usrnum > SAMPLN) {
                                        prfmsg(LOGNOG);
                                   }
                                   else {
                                        prfmsg(UIDNOG);
                                        prfmsg(ENTUID);
                                   }
                                   outprf(usrnum);
                              }
                              else {
                                   byenow(STRUKO);
                              }
                              break;
                         case 1:
                              btuinp(usrnum,input);
                              if (sameas(input,usaptr->psword)) {
                                   strcpy(input,"<password>");
                                   shomal();
                                   shochn(usaptr->userid,onlhue);
                                   btuech(usrnum,1);
                                   if (usaptr->tckavl > 0) {
                                        usrptr->class=PAYING;
                                   }
                                   else {
                                        usrptr->usetmr=0;
                                        usrptr->class=FRELOA;
                                   }
                                   prfmsg(HITHAR,usaptr->userid);
                                   lonstf();
                                   usrptr->substt=-1;
                                   (*(module00.sttrou))();
                              }
                              else {
                                   shomal();
                                   if (++(usrptr->countr) < 3) {
                                        prfmsg(PSWNOG);
                                        prfmsg(ENTPSW);
                                        outprf(usrnum);
                                   }
                                   else {
                                        byenow(STRUKO);
                                   }
                              }
                              break;
                         }
                         break;
                    default:
                         rstchn();
                    }
                    break;
               case SUPIPG:
                    switch (status) {
                    case CRSTG:
                         paccin();
                         if (!signup()) {
                              shochn(usaptr->userid,onlhue);
                              usrptr->class=FRELOA;
                              lonstf();
                              usrptr->substt=-1;
                              (*(module00.sttrou))();
                         }
                         break;
                    default:
                         rstchn();
                    }
                    break;
               default:
                    switch (status) {
                    case RING:
                         (*(module00.huprou))();
                         break;
                    case CRSTG:
                         getin();
                         hdlinp();
                         break;
                    default:
                         (*(module[usrptr->state]->stsrou))();
                    }
               }
          }
          dwopr();
          while (ticker) {
               ticker-=1;
               prcrtk();
          }
     }
     (*(module00.finrou))();
}
 
hdlinp()
{
     if (!(*(module[usrptr->state]->sttrou))()) {
          usrptr->substt=0;
          (*(module00.sttrou))();
     }
}
 
char *curtim(mode)
int mode;
{
     static char ctstg[9];
 
     time(ctstg,mode);
     return(ctstg);
}
 
char *curdat(mode)
int mode;
{
     static char cdstg[30];
 
     date(cdstg,mode);
     return(cdstg);
}
 
onsys(uid)
char *uid;
{
     othuap=usracc;
     for (othusn=0,othusp=user ; othusn < NTERMS ; othusn++,othusp++) {
          if (othusp->class > SUPIPG && sameas(uid,othuap->userid)) {
               return(1);
          }
          othuap+=1;
     }
     return(0);
}
 
uinsys(uid)
char *uid;
{
     int i;
 
     for (i=0 ; i < NTERMS ; i++) {
          if (i != usrnum && sameas(uid,usracc[i].userid)) {
               return(1);
          }
     }
     return(0);
}
 
instat(uid,qstate)
char *uid;
int qstate;
{
     othuap=usracc;
     for (othusn=0,othusp=user ; othusn < NTERMS ; othusn++,othusp++) {
          if (othusp->state == qstate && sameto(uid,othuap->userid)) {
               return(1);
          }
          othuap+=1;
     }
     return(0);
}
 
sameto(shorts,longs)
char *shorts,*longs;
{
     while (*shorts != '\0') {
          if (tolower(*shorts) != tolower(*longs)) {
               return(0);
          }
          shorts+=1;
          longs+=1;
     }
     return(1);
}
 
char *spr(ctlstg,parm1,parm2,parm3)
char *ctlstg,*parm1,*parm2,*parm3;
{
     static char result[80];
 
     sprintf(result,ctlstg,parm1,parm2,parm3);
     return(result);
}
 
inimod()
{
     int i,(*rouptr)();
 
     for (i=1 ; i < NMODS ; i++) {
          if ((rouptr=module[i]->inirou) != NULL) {
               (*rouptr)();
          }
     }
}
 
ckmcu()
{
     union REGS regs;
     int i;
     static int mcuctr=0;
 
     regs.h.ah=SVC_TIME;
     int86(0x21,&regs,&regs);
     if (regs.h.ch == 3) {
          setmbk(mjrmb);
          if (mcuctr <= MCUMIN) {
               if (nliniu() == 0 || mcuctr == MCUMIN) {
                    mcuctr=MCUMIN;
                    hupall();
                    (*(module00.mcurou))();
               }
               else if (mcuctr >= MCUMIN-5) {
                    for (i=0 ; i < NTERMS ; i++) {
                         if (user[i].class > SUPIPG) {
                              othusn=i;
                              prfmsg(GOINGD,MCUMIN-mcuctr,
                                    (mcuctr == MCUMIN-1 ? "" : "S"));
                              injoth();
                         }
                    }
               }
               mcuctr+=1;
          }
     }
     else {
          mcuctr=0;
     }
     rtkick(60,&ckmcu);
}
 
midnit()
{
     int i,(*rouptr)();
 
     accmcu();
     for (i=1 ; i < NMODS ; i++) {
          if ((rouptr=module[i]->mcurou) != NULL) {
               (*rouptr)();
          }
     }
}
 
getin()
{
     char *inpptr;
 
     paccin();
     margc=0;
     inpptr=input-1;
     while (1) {
          while (*++inpptr == ' ') {
          }
          if (*inpptr == '\0') {
               break;
          }
          margv[margc]=inpptr;
          while (*++inpptr != ' ') {
               if (*inpptr == '\0') {
                    margn[margc++]=inpptr;
                    return;
               }
          }
          *inpptr='\0';
          margn[margc++]=inpptr;
     }
}
 
paccin()
{
     inplen=btuinp(usrnum,input);
     shomal();
     usrptr->pfnacc+=(pfnlvl=profan(input));
}
 
rstrin()
{
     int i;
 
     for (i=0 ; i < margc-1 ; i++) {
          *margn[i]=' ';
     }
}
 
byenow(msgnum)
int msgnum;
{
     btulok(usrnum,1);
     btucli(usrnum);
     btuclo(usrnum);
     prfmsg(msgnum);
     outprf(usrnum);
     btucmd(usrnum,"D");
}
 
rstchn()
{
     shochn((bturst(usrnum) ? " ------- " : ""),dshhue);
     setmem(&user[usrnum],sizeof(struct user),0);
     setmem(&usracc[usrnum],sizeof(struct usracc),0);
     btuscr(usrnum,'\n');
}
 
sameas(stg1,stg2)
char *stg1,*stg2;
{
     while (*stg1 != '\0') {
          if (tolower(*stg1) != tolower(*stg2)) {
               return(0);
          }
          stg1+=1;
          stg2+=1;
     }
     return(*stg2 == '\0');
}
 
injoth()
{
     int savusn;
     struct user *savusp;
     struct usracc *savuap;
     FILE *savmb,*curmbk();
 
     outprf(othusn);
     input[0]='\0';
     inplen=0;
     margc=0;
     savusn=usrnum;
     savusp=usrptr;
     savuap=usaptr;
     savmb=curmbk();
     usrnum=othusn;
     usrptr=&user[othusn];
     usaptr=&usracc[othusn];
     hdlinp();
     usrnum=savusn;
     usrptr=savusp;
     usaptr=savuap;
     setmbk(savmb);
}
 
lonstf()
{
     int i,(*rouptr)();
 
     for (i=1 ; i < NMODS ; i++) {
          if ((rouptr=module[i]->lonrou) != NULL) {
               (*rouptr)();
          }
     }
}
 
mainu(first)
int first;
{
     int i,c,newstt;
 
     setmbk(mjrmb);
     newstt=1;
     switch (usrptr->substt) {
     case -1:
          mbmenu();
          break;
     case 0:
          shrtmu();
          break;
     case 1:
          if (sameas(margv[0],"account") && account(1)) {
               newstt=2;
               break;
          }
          switch (margc) {
          case 0:
               shrtmu();
               break;
          case 1:
               if (margv[0][1] == '\0') {
                    if ((c=margv[0][0]) == '?') {
                         prfmsg(HLPMSG);
                         mbmenu();
                    }
                    else {
                         for (i=1 ; i < NMODS ; i++) {
                              if (tolower(c) == tolower(module[i]->select)) {
                                   usrptr->state=i;
                                   usrptr->substt=0;
                                   return((*(module[i]->sttrou))());
                              }
                         }
                         prfmsg(NOTINL,c);
                         shrtmu();
                    }
               }
               else {
                    prfmsg(JSTONE);
                    shrtmu();
               }
               break;
          default:
               prfmsg(JSTONE);
               shrtmu();
          }
          break;
     case 2:
          if (account(0)) {
               newstt=2;
          }
          else {
               shrtmu();
          }
          break;
     }
     outprf(usrnum);
     usrptr->state=0;
     usrptr->substt=newstt;
     return(1);
}
 
shrtmu()
{
     int i,c;
 
     prf("\rPlease select a letter (");
 
     for (i=1 ; i < NMODS ; i++) {
          if ((c=module[i]->select) != 0) {
               prf("%c, ",c);
          }
     }
     prf("or ? for help): ");
}
 
mbmenu()
{
     int i,c;
 
     prf("\rThe following services are available:\r\r");
     for (i=1 ; i < NMODS ; i++) {
          if ((c=module[i]->select) != 0) {
               prf("   %c ... %s\r",c,module[i]->descrp);
          }
     }
     prf("\rPlease select one of the letters shown, and then press RETURN: ");
}
 
dfsthn()
{
     switch (status) {
     case 251:
     case 252:
     case 253:
          break;
     default:
          loscar();
     }
}
 
loscar()
{
     int i,(*rouptr)();
 
     for (i=1 ; i < NMODS ; i++) {
          if ((rouptr=module[i]->huprou) != NULL) {
               (*rouptr)();
          }
     }
     date(usaptr->usedat,2);
     updacc();
     rstchn();
}
 
hupall()
{
     for (usrnum=0 ; usrnum < NTERMS ; usrnum++) {
          usrptr=&user[usrnum];
          if (usrptr->class > SUPIPG) {
               usaptr=&usracc[usrnum];
               status=RING;
               (*(module00.huprou))();
          }
          else {
               rstchn();
          }
     }
}
 
mjrfin()
{
     int i,(*rouptr)();
 
     hupall();
     for (i=1 ; i < NMODS ; i++) {
          if ((rouptr=module[i]->finrou) != NULL) {
               (*rouptr)();
          }
     }
     btuend();
     clsmsg(mjrmb);
     finsup();
     clsacc();
     finsho();
     exit();
}
