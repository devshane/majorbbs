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
 *   MJREXIT.C                                                             *
 *                                                                         *
 *   This is the Major BBS default user-exit handler.                      *
 *                                                                         *
 *                                            - T. Stryker 6/28/86         *
 *                                                                         *
 ***************************************************************************/
 
#include "stdio.h"
#include "ctype.h"
#include "majorbbs.h"
#include "usracc.h"
#include "majormsg.h"
 
int xitter(),dfsthn();
 
struct module module19={      /* module interface block               */
     'X',                     /*    main menu select character        */
     "Exit (terminate session)", /* description for main menu         */
     NULL,                    /*    system initialization routine     */
     NULL,                    /*    user logon supplemental routine   */
     &xitter,                 /*    input routine if selected         */
     &dfsthn,                 /*    status-input routine if selected  */
     NULL,                    /*    hangup (lost carrier) routine     */
     NULL,                    /*    midnight cleanup routine          */
     NULL,                    /*    delete-account routine            */
     NULL                     /*    finish-up (sys shutdown) routine  */
};
 
extern
FILE *mjrmb;                  /* executive named-message file block ptr    */
extern
int usrnum;                   /* global user-number (channel) in effect    */
extern
struct user *usrptr;          /* global pointer to user data in effect     */
extern
char *margv[INPSIZ/2];        /* array of ptrs to word starts, a la argv[] */
extern
int margc;                    /* number of words in margv[], a la argc     */
 
xitter()
{
     setmbk(mjrmb);
     switch (usrptr->substt) {
     case 0:
          prfmsg(RUSXIT);
          outprf(usrnum);
          usrptr->substt=1;
          break;
     case 1:
          if (margc > 0 && tolower(*margv[0]) == 'y') {
               byenow(SEEYA);
          }
          else {
               return(0);
          }
     }
     return(1);
}
 
