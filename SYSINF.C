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
 *   SYSINF.C                                                              *
 *                                                                         *
 *   This is the demo board system-info request handler.                   *
 *                                                                         *
 *                                            - T. Stryker 7/1/86          *
 *                                                                         *
 ***************************************************************************/
 
#include "stdio.h"
#include "ctype.h"
#include "majorbbs.h"
#include "usracc.h"
#include "sysinf.h"
 
int inisin(),sysinf(),dfsthn(),clssin();
 
#define SINSTT      05        /* system info requesting state         */
struct module module05={      /* module interface block               */
     'S',                     /*    main menu select character        */
     "System information",    /*    description for main menu         */
     &inisin,                 /*    system initialization routine     */
     NULL,                    /*    user logon supplemental routine   */
     &sysinf,                 /*    input routine if selected         */
     &dfsthn,                 /*    status-input routine if selected  */
     NULL,                    /*    hangup (lost carrier) routine     */
     NULL,                    /*    midnight cleanup routine          */
     NULL,                    /*    delete-account routine            */
     &clssin                  /*    finish-up (sys shutdown) routine  */
};
 
static
FILE *sinmb,*opnmsg();        /* sys info named-message file block pointer */
 
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
    status;                   /* raw status from btusts, where appropriate */
extern
struct module *module[NMODS]; /* module access block pointer table         */
 
extern int othusn;            /* general purpose other-user channel number */
extern struct user *othusp;   /* gen purp other-user user structure ptr    */
extern struct usracc *othuap; /* gen purp other-user accounting data ptr   */
 
inisin()
{
     sinmb=opnmsg("sysinf.mcv");
}
 
sysinf()
{
     int c;
     static int infmsn[]={NOTUSH,NOTUSS,GALACO,FRESAM,HTBOH};
 
     setmbk(sinmb);
     switch (usrptr->substt) {
     case 0:
          prfmsg(INTRO);
          usrptr->substt=1;
          break;
     case 1:
          if (margc == 0) {
               prfmsg(REPRMT);
          }
          else if (margc == 1 && strlen(margv[0]) == 1) {
               if ((c=*margv[0]) == '1') {
                    othusp=user;
                    othuap=usracc;
                    prf("\rUSER-ID ...... OPTION SELECTED\r");
                    for (othusn=0 ; othusn < NTERMS ; othusn++) {
                         switch (othusp->class) {
                         case VACANT:
                              break;
                         case ONLINE:
                              prf("(line %02d)  ... (log-on)\r",othusn+1);
                              break;
                         case SUPIPG:
                              prf("(line %02d)  ... (sign-up)\r",othusn+1);
                              break;
                         default:
                              prf("%-11s... %s\r",othuap->userid,
                                  module[othusp->state]->descrp);
                              break;
                         }
                         othusp+=1;
                         othuap+=1;
                    }
                    prfmsg(REPRMT);
               }
               else if (c >= '2' && c <= '6') {
                    prfmsg(infmsn[c-'2']);
                    prfmsg(REPRMT);
               }
               else if (c == '?') {
                    prfmsg(INTRO);
               }
               else if (tolower(c) == 'x') {
                    prfmsg(EXISIN);
                    return(0);
               }
               else {
                    prfmsg(CNOTIL,c);
                    prfmsg(REPRMT);
               }
          }
          else {
               prfmsg(JSTONE);
               prfmsg(REPRMT);
          }
          break;
     }
     outprf(usrnum);
     return(1);
}
 
clssin()
{
     clsmsg(sinmb);
}
