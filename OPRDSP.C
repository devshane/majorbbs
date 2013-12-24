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
 *   OPRDSP.C                                                              *
 *                                                                         *
 *   This collection of routines runs the operator display and keyboard.   *
 *                                                                         *
 *                                            - T. Stryker 10/9/86         *
 *                                                                         *
 ***************************************************************************/
 
#include "stdio.h"
#include "ctype.h"
#include "majorbbs.h"
#include "usracc.h"
#include "btvstf.h"
#include "fkcode.h"
 
char *maiscn,                 /* main-screen current address (or NULL)     */
     *maisav,                 /* main-screen off-screen ram image ptr      */
     *malscn,                 /* monitor-all current address (or NULL)     */
     *malsav,                 /* monitor-all off-screen ram image ptr      */
     *emuscn,                 /* emulate-channel current address (or NULL) */
     *emusav,                 /* emulate-channel off-screen ram image ptr  */
     *actscn;                 /* detail-account off-screen ram image ptr   */
int syssyc;                   /* system status rectangle low y coor        */
int cmwxul,cmwylr;            /* command window x-left and y-bottom bdys   */
#define CMWYUL 20             /* command window y-top boundary             */
#define CMWXLR 77             /* command window x-right boundary           */
int dsppag;                   /* display page in effect                    */
int quiefl;                   /* quiet soft-key legends just put in place  */
int chn2em,                   /* channel to be emulated                    */
    chnemd,                   /* channel currently being emulated          */
    chn2kl,                   /* channel to kill                           */
    emux,emuy;                /* channel emulation screen (x,y) coords     */
static int c;                 /* char received from keyboard if any        */
BTVFILE *audbb;               /* Btrieve file block ptr for audit trail    */
#define AUDSIZ 65             /* size of audit trail records               */
BTVFILE *svbb;                /* Btrieve file block ptr for sys variables  */
struct sysvbl sv;             /* sys-variables record instance for updates */
 
#define CMDPAG 0              /*   command display page                    */
#define MALPAG 1              /*   monitor all modem activity page         */
#define EMUPAG 2              /*   monitor a specific channel page         */
#define ACTPAG 3              /*   detail account display page             */
 
extern
int usrnum,                   /* global user number in effect              */
    othusn;                   /* general purpose "other" user number       */
extern
struct user user[NTERMS];     /* main volatile user-info structure array   */
extern
char input[INPSIZ];           /* raw user input data buffer                */
extern
int inplen,                   /* overall raw input string length           */
    status;                   /* raw status from btusts, where appropriate */
extern
int mbdone;                   /* main-loop exit flag                       */
extern
int horiz;                    /* horizontal function keys flag             */
extern
int color;                    /* color display adapter in use flag         */
extern
struct module *module[NMODS]; /* module access block pointer table         */
extern
BTVFILE *accbb;               /* user accounts btrieve data file           */
 
extern int othusn;            /* general purpose other-user channel number */
extern struct user *othusp;   /* gen purp other-user user structure ptr    */
extern struct usracc *othuap; /* gen purp other-user accounting data ptr   */
 
int sndchn;                   /* destination for message to be sent        */
char txtstg[MTXSIZ],          /* message text string buffer                */
     *txtptr;                 /* message text string pointer               */
 
#define CMLSIZ 80             /* max input command-line length             */
#define SKBSIZ 30             /* max number of keys used to construct it   */
char cmlbuf[CMLSIZ];          /* command line input buffer                 */
int skbuf[SKBSIZ],            /* soft-key input buffer                     */
    *skbptr;                  /* soft-key buffer pointer                   */
char cskbuf[SKBSIZ];          /* collapsed (char-wise) soft-key input buf  */
 
                              /* soft-key legend table                     */
char *sklegs[][10]={
     "monitor"," send  ","display"," post  ","detail ","set msg","emulate","",""," kill ",
     "  ALL  ","channel","","","","","","","","",
     "","","","","","","","","","",
     "","","","","","","","","<ENTER>","",
     "END-MSG","","","","","","","","","",
     "mem avl"," hours ","#accnts","#paying","topmsg#","","","","","",
     "UsedYTD","LiveYTD","UsedMTD","LiveMTD","UsedDTD","LiveDTD","PaidYTD","PaidMTD","PaidDTD","",
     "   1   ","   2   ","   3   ","","","","","","","",
     "","","","","","","","","","",
     "(PAID) ","(FREE) ","","","","","","","","",
     "","","","","","","","","","",
     "","","","","","","","","<ENTER>","",
     "","","","","","","","","<ENTER>","",
     "User-ID","","","","channel","","","","system ",""
};
                              /* soft-key legend-set and input state no.'s */
#define QUIET  0              /*   all quiet on the western front          */
#define SNDAC  1              /*   send to ALL or channel no.              */
#define INPCHN 2              /*   inputting first channel digit           */
#define INPCH2 3              /*   inputting second channel digit          */
#define INPMSG 4              /*   inputting a message                     */
#define SELDSP 5              /*   select qty to be displayed              */
#define SELXTD 6              /*   select hours-type and Y, M, or DTD      */
#define SEL123 7              /*   select position to display in           */
#define ENTAMT 8              /*   enter amount of credits to credit       */
#define ENTAPF 9              /*   still entering amount, term w/PAID|FREE */
#define ENTUID 10             /*   enter user-id (proceeds to ENTORU)      */
#define ENTORU 11             /*   still entering user-id, ENTER available */
#define ENTONL 12             /*   accept ENTER only                       */
#define SELKIL 13             /*   select item to kill                     */
 
struct otrail {               /* trail structure for backspace key         */
     struct otrail *link;     /*   link                                    */
     int opistt;              /*   operator input state                    */
     char *cmlptr;            /*   command-line string pointer             */
} opinf;                      /* trail header                              */
 
int stthue=0x07,              /* system status display attribute code      */
    lblhue=0x07,              /* soft-key labels display attribute code    */
    dsphue=0x07,              /* data display box attribute code           */
    chshue=0x07,              /* channel "special" display attribute code  */
    dshhue=0x07,              /* channel dashes display attribute code     */
    onlhue=0x07,              /* channel online display attribute code     */
    manhue=0x07,              /* monitor-all normal display attribute code */
    mashue=0x70,              /* monitor-all "special" disp attribute code */
    emthue=0x07,              /* emulation text display attribute code     */
    emshue=0x70;              /* emulation status display attribute code   */
 
inisho()
{
     char *alcmem(),*frzseg();
 
     if (color) {
          stthue=0x1D;
          lblhue=0x1F;
          dsphue=0x1A;
          chshue=0x1E;
          dshhue=0x14;
          onlhue=0x1A;
          manhue=0x0B;
          mashue=0x5B;
          emthue=0x0A;
          emshue=0x5B;
     }
     allmem();
     malscn=malsav=alcmem(4000);
     iniscn("mjrmal.scn");
     movmem(frzseg(),malsav,4000);
     emuscn=emusav=alcmem(4000);
     iniscn("mjremu.scn");
     movmem(frzseg(),emusav,4000);
     actscn=alcmem(4000);
     iniscn("mjract.scn");
     movmem(frzseg(),actscn,4000);
     if (horiz) {
          iniscn("mjrbbsh.scn");
          syssyc=14;
          cmwxul=2;
          cmwylr=21;
     }
     else {
          iniscn("mjrbbsv.scn");
          syssyc=12;
          cmwxul=18;
          cmwylr=23;
     }
     maisav=alcmem(4000);
     maiscn=NULL;
     clremu();
     quiesk();
     audbb=opnbtv("auditrai.dat",AUDSIZ);
     svbb=opnbtv("sysvbl.dat",sizeof(struct sysvbl));
     geqbtv(&sv,"key",0);
     uptnd();
}
 
uptnd()
{
     redisp();
     rtkick(60,&uptnd);
}
 
redisp()
{
     char *curtim(),*curdat();
     char tmpstg[20];
     long sizmem();
     int i,x,y;
     static int wxcoor[2][3]={ 2, 2,20,  20,20,20};
     static int wycoor[2][3]={16,17,17,  15,16,17};
 
     x=curcurx();
     y=curcury();
     setatr(dsphue);
     setwin(maiscn,0,0,79,24,1);
     locate(20,syssyc+2);
     printf("%s %s",curtim(5),curdat(2));
     for (i=0 ; i < 3 ; i++) {
          locate(wxcoor[!horiz][i],wycoor[!horiz][i]);
          switch (sv.dspopt[i]) {
          case 0:
               break;
          case 1:
               sprintf(tmpstg,"%ldK",sizmem()>>10);
               printf("mem avl: %-5s",tmpstg);
               break;
          case 3:
               printf("#accnts: %-5u",sv.numact);
               break;
          case 4:
               printf("#paying: %-5u",sv.numpai);
               break;
          case 5:
               printf("topmsg#: %-5u",sv.numero);
               break;
          default:
               sprintf(tmpstg,"%ld hrs",
                       *((&sv.usedytd)+(sv.dspopt[i]-21))/3600L);
               printf("%s: %-5.5s",sklegs[SELXTD][sv.dspopt[i]-21],tmpstg);
               break;
          }
     }
     locate(x,y);
     rstwin();
     updsv();
}
 
updsv()
{
     setbtv(svbb);
     geqbtv(NULL,"key",0);
     updbtv(&sv);
}
 
quiesk()
{
     zappo();
     opinf.opistt=QUIET;
     plosk(0);
     locate(2,0);
     *(opinf.cmlptr=cmlbuf)='\0';
     *(skbptr=skbuf)='\0';
     acckey();
     quiefl=1;
}
 
zappo()
{
     struct otrail *otrptr;
 
     while ((otrptr=opinf.link) != NULL) {
          opinf.link=otrptr->link;
          deamem(otrptr,sizeof(struct otrail));
     }
}
 
plosk(sktnum)
int sktnum;
{
     char **plskpt;
     int i,savx,savy;
 
     savx=curcurx();
     savy=curcury();
     setwin(NULL,0,0,79,24,0);
     setatr(lblhue);
     zapskl();
     for (plskpt=sklegs[sktnum],i=1 ; i <= 10 ; plskpt++,i++) {
          lblsk(i,*plskpt);
     }
     locate(savx,savy);
     rstwin();
}
 
zapskl()
{
     int i;
 
     for (i=1 ; i <= 9 ; i++) {
          lblsk(i,"       ");
     }
     lblsk(10,"      ");
}
 
lblsk(sknum,lblstg)
int sknum;
char *lblstg;
{
     if (horiz) {
          locate((sknum-1)*8+1,23);
     }
     else {
          locate(((sknum&1) ? 1 : 9),((sknum-1)&0xfffe)+15);
     }
     printf(lblstg);
}
 
acckey()
{
     struct otrail *otrptr;
 
     otrptr=(struct otrail *)alcmem(sizeof(struct otrail));
     movmem(&opinf,otrptr,sizeof(struct otrail));
     opinf.link=otrptr;
}
 
bckspc()
{
     struct otrail *otrptr;
 
     if (opinf.link->link != NULL) {
          otrptr=opinf.link;
          opinf.link=otrptr->link;
          deamem(otrptr,sizeof(struct otrail));
          otrptr=opinf.link;
          movmem(otrptr,&opinf,sizeof(struct otrail));
          opinf.link=otrptr;
          plosk(opinf.opistt);
          *opinf.cmlptr='\0';
          *--skbptr='\0';
          locate(cmwxul+((int)(opinf.cmlptr-cmlbuf)),cmwylr);
          setatr(lblhue);
          clreol();
     }
}
 
dwopr()
{
     while (kbhit() && !mbdone) {
          c=getchc();
          switch (dsppag) {
          case CMDPAG:
               setwin(maiscn,cmwxul,CMWYUL,CMWXLR,cmwylr,1);
               if (c == 8 && opinf.opistt != INPMSG) {
                    bckspc();
               }
               else {
                    switch (opinf.opistt) {
                    case QUIET:
                         switch (c) {
                         case F1:
                              monall();
                              dsppag=MALPAG;
                              break;
                         case F2:
                              hdlsk("Send a message to ",SNDAC);
                              break;
                         case F3:
                              hdlsk("Display ",SELDSP);
                              break;
                         case F4:
                              hdlsk("Post ",ENTAMT);
                              break;
                         case F5:
                              hdlsk("Detail info on (User-ID): ",ENTUID);
                              break;
                         case F6:
                              hdlsk("Set log-on message",ENTONL);
                              break;
                         case F7:
                              hdlsk("Emulate channel no. ",INPCHN);
                              break;
                         case F10:
                              hdlsk("Kill ",SELKIL);
                              break;
                         case 27:
                              emuchn(chnemd);
                              dsppag=EMUPAG;
                              break;
                         }
                         break;
                    case SNDAC:
                         switch (c) {
                         case F1:
                              hdlsk("ALL online channels",ENTONL);
                              break;
                         case F2:
                              hdlsk("channel no. ",INPCHN);
                              break;
                         }
                         break;
                    case INPCHN:
                         if (isxdigit(127&c)) {
                              c=toupper(c);
                              if (c > '3') {
                                   hdlskc(ENTONL);
                              }
                              else {
                                   hdlskc(INPCH2);
                              }
                         }
                         break;
                    case INPCH2:
                         if (isxdigit(127&c)) {
                              c=toupper(c);
                              hdlskc(ENTONL);
                         }
                         else if (c == '\r' || c == F9) {
                              retkey();
                         }
                         break;
                    case INPMSG:
                         dwtxte();
                         break;
                    case SELDSP:
                         switch (c) {
                         case F1:
                              hdlsk("memory available, in position ",
                                   SEL123);
                              break;
                         case F2:
                              hdlsk("hours ",SELXTD);
                              break;
                         case F3:
                              hdlsk("total number of accounts, in position ",
                                   SEL123);
                              break;
                         case F4:
                              hdlsk("number of paying accounts, in position ",
                                   SEL123);
                              break;
                         case F5:
                              hdlsk("top (current) message no., in position ",
                                   SEL123);
                              break;
                         }
                         break;
                    case SELXTD:
                         switch (c) {
                         case F1:
                              hdlsk("used Year-To-Date, in position ",SEL123);
                              break;
                         case F2:
                              hdlsk("of live use Year-To-Date, in position ",SEL123);
                              break;
                         case F3:
                              hdlsk("used Month-To-Date, in position ",SEL123);
                              break;
                         case F4:
                              hdlsk("of live use Month-To-Date, in position ",SEL123);
                              break;
                         case F5:
                              hdlsk("used \"Day-To-Date\", in position ",SEL123);
                              break;
                         case F6:
                              hdlsk("of live use \"Day-To-Date\", in position ",SEL123);
                              break;
                         case F7:
                              hdlsk("paid-for Year-To-Date, in position ",SEL123);
                              break;
                         case F8:
                              hdlsk("paid-for Month-To-Date, in position ",SEL123);
                              break;
                         case F9:
                              hdlsk("paid-for \"Day-To-Date\", in position ",SEL123);
                              break;
                         }
                         break;
                    case SEL123:
                         switch (c) {
                         case F1:
                         case '1':
                              c=F1;
                              hdlsk("1",ENTONL);
                              break;
                         case F2:
                         case '2':
                              c=F2;
                              hdlsk("2",ENTONL);
                              break;
                         case F3:
                         case '3':
                              c=F3;
                              hdlsk("3",ENTONL);
                              break;
                         }
                         break;
                    case ENTAMT:
                         if (isdigit(127&c) || c == '-') {
                              hdlskc(ENTAPF);
                         }
                         break;
                    case ENTAPF:
                         switch (c) {
                         case F1:
                              hdlsk(" PAID credits to User-Id: ",ENTUID);
                              break;
                         case F2:
                              hdlsk(" FREE credits to User-Id: ",ENTUID);
                              break;
                         default:
                              if (isdigit(127&c)) {
                                   hdlskc(ENTAPF);
                              }
                         }
                         break;
                    case ENTUID:
                         if (isalpha(127&c)) {
                              if (!isalpha(127&*(skbptr-1))) {
                                   c=toupper(c);
                                   hdlskc(ENTUID);
                              }
                              else {
                                   c=tolower(c);
                                   if (!isalpha(127&*(skbptr-2))) {
                                        hdlskc(ENTUID);
                                   }
                                   else {
                                        hdlskc(ENTORU);
                                   }
                              }
                         }
                         break;
                    case ENTORU:
                         if (isalpha(127&c)) {
                              c=tolower(c);
                              hdlskc(ENTORU);
                              break;
                         }
                    case ENTONL:
                         if (c == '\r' || c == F9) {
                              retkey();
                         }
                         break;
                    case SELKIL:
                         switch (c) {
                         case F1:
                              hdlsk("User-ID: ",ENTUID);
                              break;
                         case F5:
                              hdlsk("channel no. ",INPCHN);
                              break;
                         case F9:
                              hdlsk("system",ENTONL);
                              break;
                         }
                         break;
                    }
               }
               break;
          case MALPAG:
          case ACTPAG:
               monoff();
               dsppag=CMDPAG;
               break;
          case EMUPAG:
               if (c == 27) {
                    monoff();
                    dsppag=CMDPAG;
               }
               else if (c < 128) {
                    btumks(c);
               }
               break;
          }
     }
     if ((c=btumds()) != 0) {
          setwin(emuscn,0,1,79,24,1);
          locate(emux,emuy);
          setatr(emthue);
          do {
               printf("%c",c);
          } while ((c=btumds()) != 0);
          emux=curcurx();
          emuy=curcury();
          if (emuscn != NULL) {
               rstloc();
          }
     }
}
 
dwtxte()
{
     int i;
 
     setatr(lblhue);
     if (c == F1) {
          if (*(txtptr-1) == '\r') {
               *--txtptr='\0';
          }
          if (sndchn == -2) {
               strcpy(sv.lonmsg,txtstg);
          }
          else if (txtptr == txtstg) {
               setatr(0x70);
               printf(" NO MESSAGE SPECIFIED, NO MESSAGE SENT ");
          }
          else if (sndchn >= 0) {
               othusn=sndchn;
               prf("!!!\7\r\rFROM SYSOP:\r%s\r\r",txtstg);
               injoth();
          }
          else if (sndchn == -1) {
               for (i=0 ; i < NTERMS ; i++) {
                    if (user[i].class > SUPIPG) {
                         othusn=i;
                         prf("!!!\7\r\rFROM SYSOP:\r%s\r\r",txtstg);
                         injoth();
                    }
               }
          }
          quiesk();
     }
     else if (c == 8 && txtptr != txtstg && *(txtptr-1) != '\r') {
          printf("\10");
          *--txtptr='\0';
     }
     else if (txtptr-txtstg < MTXSIZ-1) {
          if (c == '\r') {
               printf("\n");
               *txtptr++=c;
               *txtptr='\0';
          }
          else if (c >= ' ' && c < 128) {
               printf("%c",c);
               *txtptr++=c;
               *txtptr='\0';
          }
     }
}
 
hdlskc(opistt)
int opistt;
{
     static char stg[2]={0,0};
 
     stg[0]=c;
     hdlsk(stg,opistt);
}
 
hdlsk(echstg,opistt)
char *echstg;
int opistt;
{
     strcpy(opinf.cmlptr,echstg);
     opinf.cmlptr+=strlen(echstg);
     locate(cmwxul,cmwylr);
     setatr(lblhue);
     if (quiefl) {
          quiefl=0;
          printf("\n");
     }
     printf("%s",cmlbuf);
     *skbptr++=c;
     *skbptr='\0';
     plosk(opistt);
     opinf.opistt=opistt;
     acckey();
}
 
getchc()
{
     int c;
 
     if ((c=getch()) == 0) {
          c=getch()<<8;
     }
     return(c);
}
 
retkey()
{
     int tmplen;
 
     collap();
     switch (skbuf[0]) {
     case F2:
          printf(":\n");
          if (skbuf[1] == F1) {
               sndchn=-1;
          }
          else {
               sscanf(&cskbuf[2],"%x",&sndchn);
          }
          *(txtptr=txtstg)='\0';
          plosk(opinf.opistt=INPMSG);
          break;
     case F3:
          if (skbuf[1] == F2) {
               sv.dspopt[(skbuf[3]-F1)>>8]=((skbuf[2]-F1)>>8)+21;
          }
          else {
               sv.dspopt[(skbuf[2]-F1)>>8]=((skbuf[1]-F1)>>8)+1;
          }
          quiesk();
          redisp();
          break;
     case F4:
          tmplen=strlen(&cskbuf[1]);
          usrnum=-1;
          switch (addcrd(&cskbuf[tmplen+2],&cskbuf[1],skbuf[tmplen+1] == F1)) {
          case 1:
               saycrd(&cskbuf[1]);
          case 0:
               quiesk();
          case -1:
               break;
          }
          break;
     case F5:
          dspact(&cskbuf[1]);
          break;
     case F6:
          printf(":\n");
          sndchn=-2;
          *(txtptr=txtstg)='\0';
          plosk(opinf.opistt=INPMSG);
          break;
     case F7:
          sscanf(&cskbuf[1],"%x",&chn2em);
          quiesk();
          emuchn(chn2em);
          dsppag=EMUPAG;
          break;
     case F10:
          switch (skbuf[1]) {
          case F1:
               switch (delacct(&cskbuf[2])) {
               case 1:
                    usrnum=othusn;
                    rstchn();
               case 0:
                    quiesk();
               case -1:
                    break;
               }
               break;
          case F5:
               sscanf(&cskbuf[2],"%x",&chn2kl);
               btulok(chn2kl,1);
               btuclo(chn2kl);
               btucli(chn2kl);
               btucmd(chn2kl,"D");           /* chg this when btuinj() avail */
               quiesk();
               break;
          case F9:
               mbdone=1;
               break;
          }
     }
}
 
dspact(userid)
char *userid;
{
     char *frzseg(),*scrn;
     struct usracc acc;
     int ison;
     extern char *sysstg[];
     char *spr();
 
     if (ison=onsys(userid)) {
          movmem(othuap,&acc,sizeof(struct usracc));
     }
     else {
          setbtv(accbb);
          if (!acqbtv(&acc,userid,0)) {
               return;
          }
     }
     quiesk();
     scrn=frzseg();
     movmem(scrn,maiscn=maisav,4000);
     dsppag=ACTPAG;
     movmem(actscn,scrn,4000);
     setwin(NULL,0,0,79,24,0);
     setatr(0x70);
     locate(24,0);
     printf("%s ",userid);
     setatr(stthue);
     locate(26,2);
     printf("%s",acc.usrnam);
     locate(26,3);
     printf("%s",acc.usrad1);
     locate(26,4);
     printf("%s",acc.usrad2);
     locate(26,5);
     printf("%s",acc.usrad3);
     locate(26,6);
     printf("%s",acc.usrpho);
     locate(26,7);
     printf("%s",sysstg[acc.systyp]);
     locate(26,8);
     printf("%d",acc.scnwid);
     locate(26,9);
     printf("%d",acc.age);
     locate(26,10);
     printf("%c",acc.sex);
     locate(26,11);
     printf("%s",acc.psword);
     locate(26,14);
     printf(spr("%ld",acc.tcktot));
     locate(26,15);
     printf(spr("%ld",acc.tckpai));
     locate(26,16);
     printf(spr("%ld",acc.tcktot-acc.tckpai));
     locate(26,17);
     printf(spr("%ld",acc.tckavl));
     locate(26,18);
     printf(spr("%ld",acc.frescu/60L));
     locate(26,20);
     printf("%s",acc.credat);
     locate(26,21);
     printf("%s",acc.usedat);
     if (ison) {
          locate(59,3);
          printf("%19.19s",module[othusp->state]->descrp);
          locate(75,6);
          printf("%d",othusp->substt);
          locate(75,7);
          printf("%d",othusp->minut4/4);
          locate(75,8);
          printf("%d",othusp->pfnacc);
     }
     else {
          locate(60,4);
          printf("(USER NOT ONLINE)");
     }
     locate(5,0);
}
 
collap()
{
     int *skptr;
     char *cskptr;
 
     setmem(cskbuf,sizeof(cskbuf),0);
     for (skptr=skbuf,cskptr=cskbuf ; skptr != skbptr ; ) {
          *cskptr++=((*skptr++)&255);
     }
}
 
shochn(legend,attrib)
char *legend,attrib;
{
     setatr(attrib);
     setwin(maiscn,0,0,79,24,0);
     locate((usrnum>>4)+40,(usrnum&0x0F)+2);
     printf("%-9s",legend);
     rstloc();
}
 
shocar(cardno)
int cardno;
{
     setatr(chshue);
     locate(cardno*10+44,1);
     printf("%d",cardno);
     rstloc();
}
 
shocst(wrtaud,text,parm1,parm2,parm3)
int wrtaud;
char *text,*parm1,*parm2,*parm3;
{
     char *curtim(),*curdat();
     char audrec[AUDSIZ];
 
     setmem(audrec,AUDSIZ,0);
     sprintf(audrec,"%s %s ",curtim(5),curdat(2));
     switch (usrnum) {
     case -2:
          strcat(audrec,"3AM CLEANUP  ");
          break;
     case -1:
          strcat(audrec,"MAIN CONSOLE ");
          break;
     default:
          sprintf(audrec+15,"CHANNEL %02x   ",usrnum);
     }
     setatr(stthue);
     setwin(maiscn,2,1,34,syssyc,1);
     locate(2,syssyc);
     printf("\n%s\n",audrec);
     sprintf(audrec+28,text,parm1,parm2,parm3);
     printf(audrec+28);
     rstloc();
     rstwin();
     if (wrtaud) {
          redisp();
          setbtv(audbb);
          insbtv(audrec);
     }
}
 
shomal()
{
     setwin(malscn,1,2,78,23,1);
     locate(1,23);
     if (status == 3) {
          setatr(manhue);
          printf("\n %02x %c%3d %c %.66s",usrnum,179,status,179,input);
          if (inplen > 66) {
               printf("\n    %c    %c %s",179,179,input+66);
          }
     }
     else {
          setatr(manhue);
          printf("\n");
          setatr(mashue);
          printf(" %02x ",usrnum);
          setatr(manhue);
          printf("%c",179);
          setatr(mashue);
          printf("%3d ",status);
          setatr(manhue);
          printf("%c",179);
          if (usrnum == chnemd) {
               rstloc();
               setwin(emuscn,0,1,79,24,1);
               locate(emux,emuy);
               setatr(emthue);
               printf("\n");
               setatr(emshue);
               printf(" STATUS: %d ",status);
               setatr(emthue);
               printf("\n");
               emux=curcurx();
               emuy=curcury();
               if (emuscn == NULL) {
                    return;
               }
          }
     }
     rstloc();
}
 
monall()
{
     char *frzseg(),*scrn;
 
     scrn=frzseg();
     movmem(scrn,maiscn=maisav,4000);
     movmem(malsav,scrn,4000);
     malscn=NULL;
     locate(2,0);
}
 
emuchn(chn2em)
int chn2em;
{
     char *frzseg(),*scrn;
 
     scrn=frzseg();
     movmem(scrn,maiscn=maisav,4000);
     movmem(emusav,scrn,4000);
     emuscn=NULL;
     if (chn2em != chnemd) {
          setatr(0x70);
          locate(21,0);
          printf("%02x",chn2em);
          clremu();
          chnemd=chn2em;
          btumon(chnemd);
     }
     else {
          locate(emux,emuy);
     }
}
 
clremu()
{
     chnemd=-1;
     setwin(emuscn,0,1,79,24,1);
     setatr(emthue);
     printf("\14");
     emux=curcurx();
     emuy=curcury();
     rstwin();
}
 
monoff()
{
     char *frzseg(),*scrn;
 
     scrn=frzseg();
     if (malscn == NULL) {
          movmem(scrn,malscn=malsav,4000);
     }
     else if (emuscn == NULL) {
          movmem(scrn,emuscn=emusav,4000);
     }
     movmem(maisav,scrn,4000);
     maiscn=NULL;
     locate(2,0);
}
 
unsigned numero()
{
     sv.numero+=1;
     redisp();
     return(sv.numero);
}
 
iniscn(filnam)
char *filnam;
{
     FILE *fp;
     char *frzseg(),*ptr;
     int i;
 
     if ((fp=fopen(filnam,"rb")) == NULL) {
          catastro("INISCN UNABLE TO OPEN \"%s\" FOR INPUT",filnam);
     }
     if (fread(frzseg(),4000,1,fp) != 1) {
          catastro("INISCN ERROR READING \"%s\"",filnam);
     }
     if (!color) {
          for (i=0,ptr=frzseg()+1 ; i < 2000 ; i++,ptr+=2) {
               if ((*ptr&0x70) != 0x70) {
                    *ptr&=~0x70;
               }
               if ((*ptr&0x07) != 0) {
                    *ptr|=0x07;
                    *ptr&=~8;
               }
          }
     }
     fclose(fp);
}
 
finsho()
{
     updsv();
     clsbtv(svbb);
     clsbtv(audbb);
     locate(0,23);
}
