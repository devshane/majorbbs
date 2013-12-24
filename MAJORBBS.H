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
 *   MAJORBBS.H                                                            *
 *                                                                         *
 *   This is the main Major BBS header file.                               *
 *                                                                         *
 *                                            - T. Stryker 6/24/86         *
 *                                                                         *
 ***************************************************************************/
 
#define NMODS     20          /* number of run-time modules supported      */
#define NTERMS    16          /* number of users to allow for              */
#define INPSIZ   128          /* input buffer size for each channel        */
#define OUTSIZ  2048          /* output buffer size for each channel       */
#define BPORT  0x2F0          /* default base port of breakthrough card(s) */
#define MTXSIZ   400          /* maximum message-text string size          */
 
struct user {                 /* volatile per-user info maintained         */
     int class;               /*    class (offline, or flavor of online)   */
     int state;               /*    state (module number in effect)        */
     int substt;              /*    substate (for convenience of module)   */
     int usetmr;              /*    usage timer (for nonlive timeouts etc) */
     int minut4;              /*    total minutes of use, times 4          */
     int countr;              /*    general purpose counter                */
     int pfnacc;              /*    profanity accumulator                  */
};
                                   /* user class codes                     */
#define VACANT 0                   /*    vacant, offline                   */
#define ONLINE 1                   /*    online, not logged on             */
#define SUPIPG 2                   /*    signup in progress                */
#define FRELOA 3                   /*    logged on and freeloading         */
#define PAYING 4                   /*    logged on and paying              */
 
                                   /* standard profanity thresholds        */
#define WRNPFN 5                   /*    warning level                     */
#define MAXPFN 8                   /*    logoff level                      */
 
struct module {                    /* module interface block               */
     char select;                  /*    main menu select character        */
     char *descrp;                 /*    description for main menu         */
     int (*inirou)();              /*    system initialization routine     */
     int (*lonrou)();              /*    user logon supplemental routine   */
     int (*sttrou)();              /*    input routine if selected         */
     int (*stsrou)();              /*    status-input routine if selected  */
     int (*huprou)();              /*    hangup (lost carrier) routine     */
     int (*mcurou)();              /*    midnight cleanup routine          */
     int (*dlarou)();              /*    delete-account routine            */
     int (*finrou)();              /*    finish-up (sys shutdown) routine  */
};
 
struct sysvbl {                    /* system-variable btrieve record layout*/
     char key[4];                  /*    4-character dummy key of "key"    */
     int dspopt[3];                /*    display options by position number*/
     unsigned numero;              /*    message-number counter            */
     unsigned numact;              /*    number of user accounts total     */
     unsigned numpai;              /*    number of paying accounts         */
     long usedytd;                 /*    "tickies" used year-to-date       */
     long liveytd;                 /*    live "tickies" used year-to-date  */
     long usedmtd;                 /*    "tickies" used month-to-date      */
     long livemtd;                 /*    live "tickies" used month-to-date */
     long useddtd;                 /*    "tickies" used day-to-date        */
     long livedtd;                 /*    live "tickies" used day-to-date   */
     long paidytd;                 /*    "tickies" paid-for year-to-date   */
     long paidmtd;                 /*    "tickies" paid-for month-to-date  */
     long paiddtd;                 /*    "tickies" paid-for day-to-date    */
     char lonmsg[MTXSIZ];          /*    log-on message in effect          */
     char spare[504-452];          /*    spare space, decrease when adding */
};
