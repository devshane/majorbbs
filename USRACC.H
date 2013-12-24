/*  GALACTICOMM Major BBS user account header file  */
 
#define UIDSIZ 10             /* user-id size (including trailing zero)    */
#define PSWSIZ 10             /* password size (ditto)                     */
#define NADSIZ 30             /* name/address line size (ditto)            */
#define PHOSIZ 16             /* phone number field size (ditto)           */
#define DATSIZ 10             /* date size (ditto, plus 1 for alignment)   */
#define USRACCSPARE (252-202) /* spare space, decrease when adding entries */
 
struct usracc {
     char userid[UIDSIZ];     /* user-id                                   */
     char psword[PSWSIZ];     /* password                                  */
     char usrnam[NADSIZ];     /* user name                                 */
     char usrad1[NADSIZ];     /* address line 1 (company)                  */
     char usrad2[NADSIZ];     /* address line 2                            */
     char usrad3[NADSIZ];     /* address line 3                            */
     char usrpho[PHOSIZ];     /* phone number                              */
     int systyp;              /* system type code                          */
     int scnwid;              /* screen width                              */
     int age;                 /* user's age                                */
     int sex;                 /* user's sex ('M' or 'F')                   */
     char credat[DATSIZ];     /* account creation date                     */
     char usedat[DATSIZ];     /* date of last use of account               */
     long frescu;             /* free seconds used to date                 */
     long tcktot;             /* total "live" seconds credited             */
     long tckpai;             /* portion of "live" seconds paid for        */
     long tckavl;             /* "live" seconds available at the moment    */
     int csicnt;              /* classified-ad counts used so far          */
     char spare[USRACCSPARE]; /* spare space, for graceful upgrades        */
};
