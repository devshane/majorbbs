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
 *   RTKICK.C                                                              *
 *                                                                         *
 *   This is the real-time kicktable handler, used to kick off a selected  *
 *   subroutine a specified number of seconds from the present time.       *
 *                                                                         *
 *                                            - T. Stryker 6/24/86         *
 *                                                                         *
 ***************************************************************************/
 
#define KTSIZE 20                  /* kicktable size                       */
 
struct kook {                      /* kicktable entry layout               */
     int countr;                   /*   number of seconds yet to go        */
     int (*dest)();                /*   subroutine address to invoke       */
} kcktbl[KTSIZE];
 
inirtk()
{
     int i;
     struct kook *kckptr;
 
     for (i=0,kckptr=kcktbl ; i < KTSIZE ; i++,kckptr++) {
          kckptr->countr=0;
     }
}
 
rtkick(delay,dstrou)
int delay;
int (*dstrou)();
{
     int i;
     struct kook *kckptr;
 
     for (i=0,kckptr=kcktbl ; i < KTSIZE ; i++,kckptr++) {
          if (kckptr->countr == 0) {
               kckptr->countr=delay;
               kckptr->dest=dstrou;
               return;
          }
     }
     catastro("RTKICK: KCKTBL OVERFLOW");
}
 
prcrtk()
{
     int i;
     struct kook *kckptr;
 
     for (i=0,kckptr=kcktbl ; i < KTSIZE ; i++,kckptr++) {
          if (kckptr->countr != 0 && --(kckptr->countr) == 0) {
               (*(kckptr->dest))();
          }
     }
}
 
