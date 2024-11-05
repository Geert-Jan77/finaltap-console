/*---------------------------------------------------------------------------
  main.c  (Final TAP 2.7.x console version).

  Part of project "Final TAP". 
  
  A Commodore 64 tape remastering and data extraction utility.

  (C) 2001-2006 Stewart Wilson, Subchrist Software.
   
  
   
   This program is free software; you can redistribute it and/or modify it under 
   the terms of the GNU General Public License as published by the Free Software 
   Foundation; either version 2 of the License, or (at your option) any later 
   version.
   
   This program is distributed in the hope that it will be useful, but WITHOUT ANY 
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
   PARTICULAR PURPOSE. See the GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License along with 
   this program; if not, write to the Free Software Foundation, Inc., 51 Franklin 
   St, Fifth Floor, Boston, MA 02110-1301 USA
   





   Changes since last release (2.76)...
   
  -Added function "is_pulse()" to allow tidying up of some scanners code where pulses
   are tested to see if they qualify as a particular type. 
   
  -Removed 'tap_tr' from mydefs.h, batchscan database now uses full 'tap_t' structure. 

  -Done away with the global variables cbm_header,cbm_program,cbm_decoded and implemented a better method
   noticeable in loader_id.c, scanners that require access to other files must now do so by
   using find_decode_block().

  -Sped up batch scanner by altering report() to have a fast mode for batch scanner 
   (skips extraction etc), +fixed minor design fault made apparent by this change. 

  -Fixed problem with batch scanner being unable to scan an entire drive 
   (ie. ftcon -b C:\) due to a fault in filename processing.
   
  -Fixed problem with batchscan not allocating enough ram for filepaths and
   producing corrupted reports.
   
  -Added additional info to reports, batch report now records time taken. 
  -Added CSV file output from batch scanner. (all fields are currently quoted).
   
   
   Problems/Todo...
      
  -still a problem batch scanning drives when quotes are used. (-b "C:\")
   the quote character ends up IN the argument!. (more a problem with the frontend)    
      
  -Batch scanner has a limit of MAXTAPS taps (currently 8000)...
   ..implement a dynamic allocation.
   
  
---------------------------------------------------------------------*/

#include "main.h"
#include "mydefs.h"
#include "crc32.h"
#include "tap2audio.h"


/* Program Options... */

char debug                  =FALSE;
char noid                   =FALSE;
char noc64eof               =FALSE;
char docyberfault           =FALSE;
char boostclean             =FALSE;
char noaddpause             =FALSE;
char sine                   =FALSE;
char prgunite               =FALSE;
char extvisipatch           =FALSE;
char incsubdirs             =FALSE;
char sortbycrc              =FALSE;
char exportcyberloaders     =FALSE;
char preserveloadertable    =TRUE;
char noc64                  =FALSE;
char noaces                 =FALSE;
char noanirog               =FALSE;
char noatlantis             =FALSE;
char noaudiogenic           =FALSE;
char nobleep                =FALSE;
char noburner               =FALSE;
char nochr                  =FALSE;
char nocyber                =FALSE;
char noenigma               =FALSE;
char nofire                 =FALSE;
char noflash                =FALSE;
char nofree                 =FALSE;
char nohit                  =FALSE;
char nohitec                =FALSE;
char noik                   =FALSE;
char nojet                  =FALSE;
char nomicro                =FALSE;
char nonova                 =FALSE;
char noocean                =FALSE;
char nooceannew1t1          =FALSE;
char nooceannew1t2          =FALSE;
char nooceannew2            =FALSE;
char nopalacef1             =FALSE;
char nopalacef2             =FALSE;
char nopav                  =FALSE;
char norackit               =FALSE;
char noraster               =FALSE;
char noseuck                =FALSE;
char nosnake50              =FALSE;
char nosnake51              =FALSE;
char nospav                 =FALSE;
char nosuper                =FALSE;
char notdif1                =FALSE;
char noturbo                =FALSE;
char noturr                 =FALSE;
char nousgold               =FALSE;
char novirgin               =FALSE;
char novisi                 =FALSE;
char nowild                 =FALSE;

int tol= DEFTOL;      /* bit reading tolerance. (1 = zero tolerance). */
/*-----------------------------------*/

struct tap_t tap;            /* container for the loaded tap (note: only ONE tap). */
struct blk_t *blk[BLKMAX];   /* database of all found entities. */
struct prg_t prg[BLKMAX];    /* database of all extracted files (prg's). */

char info[1000000];          /* buffer area for storing each blocks description text.
                                also used to store the database report, hence the size! (1MB). */

char lin[64000];             /* general purpose string building buffer. */
char tmp[64000];             /* general purpose string building buffer. */

int read_errors[100];        /* storage for 1st 100 read error addresses. */
char note_errors;            /* set true only when decoding identified files,
                                it just tells 'add_read_error()' to ignore. */

int aborted= 0;              /* general 'operation aborted by user' flag. */

int visi_type= VISI_T2;      /* default visiload type, overidden when loader ID'd. */

int cyber_f2_eor1= 0xAE;     /* default XOR codes for cyberload f2.. */
int cyber_f2_eor2= 0xD2;

int batchmode= 0;            /* set to 1 when "batch analysis" is performed.     
                                set to 0 when the user Opens an individual tap. */

int quiet= 0;                /* set 1 to stop the loader search routines from producing output,   
                                ie. "Scanning: Freeload".      
                                i set it (1) when (re)searching during optimization. */

int dbase_is_full= 0;        /* used by 'addblockdef' to indicate database capacity reached.   */

/* create the tape-format table...

 usage: ie.  ft[FREE].sp, ft[FREE].lp  etc.   */

struct fmt_t ft_org[100];    /* a backup copy of the following... */
struct fmt_t ft[100]=
{
   /* name,                 en,    tp,   sp,   mp,  lp,   pv,   sv,  pmin, pmax, has_cs.    */

   {""                      ,NA,    NA,   NA,   NA,  NA,   NA,   NA,   NA,  NA,    NA},
   {"UNRECOGNIZED"          ,NA,    NA,   NA,   NA,  NA,   NA,   NA,   NA,  NA,    NA},
   {"PAUSE"                 ,NA,    NA,   NA,   NA,  NA,   NA,   NA,   NA,  NA,    NA},
   {"C64 ROM-TAPE HEADER"   ,LSbF,  NA,   0x30, 0x42,0x56, NA,   NA,   50,  NA,    CSYES},
   {"C64 ROM-TAPE DATA"     ,LSbF,  NA,   0x30, 0x42,0x56, NA,   NA,   50,  NA,    CSYES},
   {"TURBOTAPE-250 HEADER"  ,MSbF,  0x20, 0x1A, NA,  0x28, 0x02, 0x09, 50,  NA,    CSNO},
   {"TURBOTAPE-250 DATA"    ,MSbF,  0x20, 0x1A, NA,  0x28, 0x02, 0x09, 50,  NA,    CSYES},
   {"FREELOAD"              ,MSbF,  0x2C, 0x24, NA,  0x42, 0x40, 0x5A, 45,  400,   CSYES},
   {"US-GOLD TAPE"          ,MSbF,  0x2C, 0x24, NA,  0x42, 0x20, 0xFF, 50,  NA,    CSYES},
   {"ACE OF ACES TAPE"      ,MSbF,  0x2C, 0x22, NA,  0x47, 0x80, 0xFF, 50,  NA,    CSYES},
   {"WILDLOAD"              ,LSbF,  0x3B, 0x30, NA,  0x47, 0xA0, 0x0A, 50,  NA,    CSYES},
   {"WILDLOAD STOP"         ,LSbF,  0x3B, 0x30, NA,  0x47, 0xA0, 0x0A, 50,  NA,    CSNO},
   {"NOVALOAD"              ,LSbF,  0x3D, 0x24, NA,  0x56, 0,    1,    1800,NA,    CSYES},
   {"NOVALOAD SPECIAL"      ,LSbF,  0x3D, 0x24, NA,  0x56, 0,    1,    1800,NA,    CSYES},
   {"OCEAN/IMAGINE F1"      ,LSbF,  0x3B, 0x24, NA,  0x56, 0,    1,    3000,13000, CSNO},
   {"OCEAN/IMAGINE F2"      ,LSbF,  0x3B, 0x24, NA,  0x56, 0,    1,    3000,13000, CSNO},
   {"OCEAN/IMAGINE F3"      ,LSbF,  0x3B, 0x24, NA,  0x56, 0,    1,    3000,13000, CSNO},
   {"CHR TAPE T1"           ,MSbF,  0x20, 0x1A, NA,  0x28, 0x63, 0x64, 50,  NA,    CSYES},
   {"CHR TAPE T2"           ,MSbF,  0x2D, 0x26, NA,  0x36, 0x63, 0x64, 50,  NA,    CSYES},
   {"CHR TAPE T3"           ,MSbF,  0x3E, 0x36, NA,  0x47, 0x63, 0x64, 50,  NA,    CSYES},
   {"RASTERLOAD"            ,MSbF,  0x3F, 0x26, NA,  0x58, 0x80, 0xFF, 20,  NA,    CSYES},
   {"CYBERLOAD F1"          ,MSbF,  VV,   VV,   VV,  VV,   VV,   VV,   50,  NA,    CSNO},
   {"CYBERLOAD F2"          ,MSbF,  VV,   VV,   VV,  VV,   VV,   VV,   20,  NA,    CSNO},
   {"CYBERLOAD F3"          ,MSbF,  VV,   VV,   VV,  VV,   VV,   VV,    7,   9,    CSYES},
   {"CYBERLOAD F4_1"        ,MSbF,  VV,   VV,   VV,  VV,   VV,   VV,    6,  11,    CSYES},
   {"CYBERLOAD F4_2"        ,MSbF,  VV,   VV,   VV,  VV,   VV,   VV,    6,  11,    CSYES},
   {"CYBERLOAD F4_3"        ,MSbF,  VV,   VV,   VV,  VV,   VV,   VV,    6,  11,    CSYES},
   {"BLEEPLOAD"             ,MSbF,  0x45, 0x30, NA,  0x5A, VV,   VV,   50,  NA,    CSYES},
   {"BLEEPLOAD TRIGGER"     ,MSbF,  0x45, 0x30, NA,  0x5A, VV,   VV,   50,  NA,    CSNO},
   {"BLEEPLOAD SPECIAL"     ,MSbF,  0x45, 0x30, NA,  0x5A, VV,   VV,   50,  NA,    CSYES},
   {"HITLOAD"               ,MSbF,  0x4E, 0x34, NA,  0x66, 0x40, 0x5A, 30,  NA,    CSYES},
   {"MICROLOAD"             ,LSbF,  0x29, 0x1C, NA,  0x36, 0xA0, 0x0A, 50,  NA,    CSYES},
   {"BURNER TAPE"           ,VV,    0x2F, 0x22, NA,  0x42, VV,   VV,   30,  NA,    CSNO},
   {"RACK-IT TAPE"          ,MSbF,  0x2B, 0x1D, NA,  0x3D, VV,   VV,   50,  NA,    CSYES},
   {"SUPERPAV T1 HEADER"    ,MSbF,  NA,   0x2E, 0x45,0x5C, NA,   0x66, 50,  NA,    CSYES},
   {"SUPERPAV T1"           ,MSbF,  NA,   0x2E, 0x45,0x5C, NA,   0x66, 50,  NA,    CSYES},
   {"SUPERPAV T2 HEADER"    ,MSbF,  NA,   0x3E, 0x5D,0x7C, NA,   0x66, 50,  NA,    CSYES},
   {"SUPERPAV T2"           ,MSbF,  NA,   0x3E, 0x5D,0x7C, NA,   0x66, 50,  NA,    CSYES},
   {"VIRGIN TAPE"           ,MSbF,  0x2B, 0x21, NA,  0x3B, 0xAA, 0xA0, 30,  NA,    CSYES},
   {"HI-TEC TAPE"           ,MSbF,  0x2F, 0x25, NA,  0x45, 0xAA, 0xA0, 30,  NA,    CSYES},
   {"ANIROG TAPE"           ,MSbF,  0x20, 0x1A, NA,  0x28, 0x02, 0x09, 50,  NA,    CSNO},
   {"VISILOAD T1"           ,VV,    0x35, 0x25, NA,  0x48, 0x00, 0x16, 100, NA,    CSNO},
   {"VISILOAD T2"           ,VV,    0x3B, 0x25, NA,  0x4B, 0x00, 0x16, 100, NA,    CSNO},
   {"VISILOAD T3"           ,VV,    0x3E, 0x25, NA,  0x54, 0x00, 0x16, 100, NA,    CSNO},
   {"VISILOAD T4"           ,VV,    0x47, 0x30, NA,  0x5D, 0x00, 0x16, 100, NA,    CSNO},
   {"SUPERTAPE HEADER"      ,LSbF,  NA,   0x21, 0x32,0x43, 0x16, 0x2A, 50,  NA,    CSYES},
   {"SUPERTAPE DATA"        ,LSbF,  NA,   0x21, 0x32,0x43, 0x16, 0xC5, 50,  NA,    CSYES},
   {"PAVLODA"               ,MSbF,  0x28, 0x1F, NA,  0x3F, 0,    1,    50,  NA,    CSYES},
   {"IK TAPE"               ,MSbF,  0x2C, 0x27, NA,  0x3F, 1,    0,    1000,NA,    CSYES},
   {"FIREBIRD T1"           ,LSbF,  0x62, 0x44, NA,  0x7E, 0x02, 0x52, 30,  NA,    CSYES},
   {"FIREBIRD T2"           ,LSbF,  0x52, 0x45, NA,  0x65, 0x02, 0x52, 30,  NA,    CSYES},
   {"TURRICAN TAPE HEADER"  ,MSbF,  NA,   0x1B, NA,  0x27, 0x02, 0x0C, 30,  NA,    CSNO},
   {"TURRICAN TAPE DATA"    ,MSbF,  NA,   0x1B, NA,  0x27, 0x02, 0x0C, 30,  NA,    CSYES},
   {"SEUCK LOADER 2"        ,LSbF,  0x2D, 0x20, NA,  0x41, 0xE3, 0xD5, 5,   NA,    CSYES},
   {"SEUCK HEADER"          ,LSbF,  0x2D, 0x20, NA,  0x41, 0xE3, 0xD5, 5,   NA,    CSNO},
   {"SEUCK SUB-BLOCK"       ,LSbF,  0x2D, 0x20, NA,  0x41, 0xE3, 0xD5, 5,   NA,    CSYES},
   {"SEUCK TRIGGER"         ,LSbF,  0x2D, 0x20, NA,  0x41, 0xE3, 0xD5, 5,   NA,    CSNO},
   {"SEUCK GAME"            ,LSbF,  0x2D, 0x20, NA,  0x41, 0xE3, 0xAC, 50,  NA,    CSNO},
   {"JETLOAD"               ,LSbF,  0x3B, 0x33, NA,  0x58, 0xD1, 0x2E, 1,   NA,    CSNO},
   {"FLASHLOAD"             ,MSbF,  NA,   0x1F, NA,  0x31, 1,    0,    50,  NA,    CSYES},
   {"TDI TAPE F1"           ,LSbF,  NA,   0x44, NA,  0x65, 0xA0, 0x0A, 50,  NA,    CSYES},
   {"OCEAN NEW TAPE F1 T1"  ,MSbF,  NA,   0x22, NA,  0x42, 0x40, 0x5A, 50,  200,   CSYES},
   {"OCEAN NEW TAPE F1 T2"  ,MSbF,  NA,   0x35, NA,  0x65, 0x40, 0x5A, 50,  200,   CSYES},
   {"OCEAN NEW TAPE F2"     ,MSbF,  0x2C, 0x22, NA,  0x42, 0x40, 0x5A, 50,  200,   CSYES},
   {"ATLANTIS TAPE"         ,LSbF,  0x2F, 0x1D, NA,  0x42, 0x02, 0x52, 50,  NA,    CSYES},
   {"SNAKELOAD 5.1"         ,MSbF,  NA,   0x28, NA,  0x48, 0,    1,    1800,NA,    CSYES},
   {"SNAKELOAD 5.0 T1"      ,MSbF,  NA,   0x3F, NA,  0x5F, 0,    1,    1800,NA,    CSYES},
   {"SNAKELOAD 5.0 T2"      ,MSbF,  NA,   0x60, NA,  0xA0, 0,    1,    1800,NA,    CSYES},
   {"PALACE TAPE F1"        ,MSbF,  NA,   0x30, NA,  0x57, 0x01, 0x4A, 50,  NA,    CSYES},
   {"PALACE TAPE F2"        ,MSbF,  NA,   0x30, NA,  0x57, 0x01, 0x4A, 50,  NA,    CSYES},
   {"ENIGMA TAPE"           ,MSbF,  0x2C, 0x24, NA,  0x42, 0x40, 0x5A, 700, NA,    CSNO},
   {"AUDIOGENIC"            ,MSbF,  0x28, 0x1A, NA,  0x36, 0xF0, 0xAA, 4,   NA,    CSYES},
   {"",666,666,666,666,666,666,666,666,666,666}

/*    Name,                 en,    tp,   sp,   mp,  lp,   pv,   sv,  pmin,  has_cs.

   where a field is marked 'VV', loader/file interrogation is required to
   discover the missing value.
*/

};


/* The following strings are used by the loader ID system...
   see enum for this table in mydefs.h...
*/
const char knam[100][32]=
{
{"n/a"},
{"Freeload (or clone)"},
{"Bleepload"},
{"CHR loader"},
{"Burner"},
{"Wildload"},
{"US Gold loader"},
{"Microload"},
{"Ace of Aces loader"},
{"Turbotape 250"},
{"Rack-It loader"},
{"Ocean/Imagine (ns)"},
{"Rasterload"},
{"Super Pavloda"},
{"Hit-Load"},
{"Anirog loader"},
{"Visiload T1"},
{"Visiload T2"},
{"Visiload T3"},
{"Visiload T4"},
{"Firebird loader"},
{"Novaload (ns)"},
{"IK loader"},
{"Pavloda"},
{"Cyberload"},
{"Virgin"},
{"Hi-Tec"},
{"Flashload"},
{"Supertape"},
{"Ocean New 1 T1"},
{"Ocean New 1 T2"},
{"Atlantis Loader"},
{"Snakeload"},
{"Ocean New 2"},
{"Audiogenic"}
};

char exedir[512];                /* assigned in main, includes trailing slash. */
char ftverstr[256];              /* assigned in main. */
char ftreportname[256]=          "ftreport.txt";
char tempftreportname[256]=      "ftreport.tmp";
char ftbatchreportname[256]=     "ftbatch.txt";
char tempftbatchreportname[256]= "ftbatch.tmp";
char ftinfoname[256]=            "ftinfo.txt";
char tapoutname[256]=            "out.tap";
char auoutname[256]=             "out.au";
char wavoutname[256]=            "out.wav";

/* note: all generated files are saved to the exedir */

/*------------------------------------------------------------------------------------------
*/
int main(int argc, char* argv[])
{
   int i,j,actnum;
   time_t t1,t2;
   FILE *fp;
  
   char actname[11][30]= 
   {"No operation",
    "Testing", 
    "Optimizing",
    "Converting to v0",
    "Converting to v1", 
    "Fixing header size", 
    "Optimizing pauses",
    "Converting to au file",
    "Converting to wav file",
    "Batch scanning",
    "Creating info file"
   };
         
   deleteworkfiles();
         
   /* get exe path from argv[0]... */   
   strcpy(exedir, argv[0]);    
   for(i=strlen(exedir); i>0 && exedir[i]!='\\'; i--)  /* clip to leave path only */
   ;
   if(exedir[i]=='\\')
      exedir[i+1]=0;   
   
   /* note: i do this instead of using getcwd() because getcwd does not give
      the exe's directory when dragging and dropping a tap file to the program
      icon using windows explorer. */
      
   /* when run at the console argv[0] is simply "ftcon" or "ftcon.exe" 
      ie. not a full path */
   if(strcmp(exedir,"ftcon")==0 || strcmp(exedir,"ftcon.exe")==0)
   {
      getcwd(exedir,512);
      strcat(exedir,"\\");
   }
          
       
   /*-----------------------------------------------------------------------*/

   /* Allocate ram to file database and initialize array pointers... */
   for(i=0; i<BLKMAX; i++)
   {
      blk[i]= (struct blk_t*)malloc(sizeof(struct blk_t));
      if(blk[i]==NULL)
      {
         printf("\nError: malloc failure whilst creating file database.");
         return 1;
      }
      blk[i]->dd= NULL;
      blk[i]->fn= NULL;
   }

   copy_loader_table();
   build_crc_table();

   tol= DEFTOL;      /* set default read tolerance */
   debug= FALSE;     /* clear "debugging" mode. (allow block overlap). */
   
   printf("\n___________________________________________________________");
   printf("\n");
   printf("\nFinal TAP %0.2f Console - (C) 2001-%d Subchrist Software.", FTVERSION,FTYEAR);
   printf("\n___________________________________________________________");

   sprintf(ftverstr,"ftcon.exe v%0.2f (%d/%d/%d)", FTVERSION,FTDAY,FTMONTH,FTYEAR);
   
   if(argc==1)      /* show usage information if no arguments... */
   {
      printf("\n\nUsage:-\n");
      printf("\nft [option][parameter]");
      printf("\n\noptions...\n");

      printf("\n -t   [tap]     Test TAP.");
      printf("\n -o   [tap]     Optimize TAP.");
      printf("\n -b   [dir]     Batch test.");
      printf("\n -au  [tap]     Convert TAP to Sun AU audio file (44kHz).");
      printf("\n -wav [tap]     Convert TAP to Microsoft WAV audio file (44kHz).");
      printf("\n -rs  [tap]     Corrects the 'size' field of a TAPs header.");
      printf("\n -ct0 [tap]     Convert TAP to version 0 format.");
      printf("\n -ct1 [tap]     Convert TAP to version 1 format.\n");
      
      printf("\n -tol [0-15]    Set pulsewidth read tolerance, default=10.");
      printf("\n -debug         Allows detected files to overlap.");
      printf("\n -noid          Disable scanning for only the 1st ID'd loader.");
      printf("\n -noc64eof      C64 ROM scanner will not expect EOF markers.");
      printf("\n -docyberfault  Report Cyberload F3 bad checksums of $04.");
      printf("\n -boostclean    Raise cleaning threshold.");
      printf("\n -noaddpause    Dont add a pause to the file end after clean.");
      printf("\n -sine          Make audio converter use sine waves.");
      printf("\n -prgunite      Connect neighbouring PRG's into a single file.");
      printf("\n -extvisipatch  Extract Visiload loader patch files.");
      printf("\n -incsubdirs    Make batch scan include subdirectories.");
      printf("\n -sortbycrc     Batch scan sorts report by cbmcrc values.");
   }

   /* Note: Options should be processed before actions! */

   else
   {
      /* PROCESS OPTIONS... */

      for(i=0; i<argc; i++)
      {
         if(strcmp(argv[i],"-tol")==0)   /* flag = set tolerance */
         {
            if(argv[i+1]!=NULL)
            {
               tol= atoi(argv[i+1])+1;  /* 1 = zero tolerance (-tol 0) */
               if(tol<0 || tol>15)
               {
                  tol=DEFTOL;
                  printf("\n\nTolerance parameter out of range, using default.");
               }
            }
            else
               printf("\n\nTolerance parameter missing, using default.");
         }

         if(strcmp(argv[i],"-debug")==0)          debug= TRUE;
         if(strcmp(argv[i],"-noid")==0)           noid= TRUE;
         if(strcmp(argv[i],"-noc64eof")==0)       noc64eof= TRUE;
         if(strcmp(argv[i],"-docyberfault")==0)   docyberfault= TRUE;
         if(strcmp(argv[i],"-boostclean")==0)     boostclean= TRUE;
         if(strcmp(argv[i],"-noaddpause")==0)     noaddpause= TRUE;
         if(strcmp(argv[i],"-sine")==0)           sine= TRUE;
         if(strcmp(argv[i],"-prgunite")==0)       prgunite= TRUE;
         if(strcmp(argv[i],"-extvisipatch")==0)   extvisipatch= TRUE;
         if(strcmp(argv[i],"-incsubdirs")==0)     incsubdirs= TRUE;
         if(strcmp(argv[i],"-sortbycrc")==0)      sortbycrc= TRUE;
         if(strcmp(argv[i],"-ec")==0)             exportcyberloaders= TRUE;
         
         if(strcmp(argv[i],"-noc64")==0)          noc64= TRUE;
         if(strcmp(argv[i],"-noaces")==0)         noaces= TRUE;
         if(strcmp(argv[i],"-noanirog")==0)       noanirog= TRUE;
         if(strcmp(argv[i],"-noatlantis")==0)     noatlantis= TRUE;
         if(strcmp(argv[i],"-noaudiogenic")==0)   noaudiogenic= TRUE;
         if(strcmp(argv[i],"-nobleep")==0)        nobleep= TRUE;
         if(strcmp(argv[i],"-noburner")==0)       noburner= TRUE;
         if(strcmp(argv[i],"-nochr")==0)          nochr= TRUE;
         if(strcmp(argv[i],"-nocyber")==0)        nocyber= TRUE;
         if(strcmp(argv[i],"-noenigma")==0)       noenigma= TRUE;
         if(strcmp(argv[i],"-nofire")==0)         nofire= TRUE;
         if(strcmp(argv[i],"-noflash")==0)        noflash= TRUE;
         if(strcmp(argv[i],"-nofree")==0)         nofree= TRUE;
         if(strcmp(argv[i],"-nohit")==0)          nohit= TRUE;
         if(strcmp(argv[i],"-nohitec")==0)        nohitec= TRUE;
         if(strcmp(argv[i],"-nojet")==0)          nojet= TRUE;
         if(strcmp(argv[i],"-noik")==0)           noik= TRUE;
         if(strcmp(argv[i],"-nomicro")==0)        nomicro= TRUE;
         if(strcmp(argv[i],"-nonova")==0)         nonova= TRUE;
         if(strcmp(argv[i],"-noocean")==0)        noocean= TRUE;
         if(strcmp(argv[i],"-nooceannew1t1")==0)  nooceannew1t1= TRUE;
         if(strcmp(argv[i],"-nooceannew1t2")==0)  nooceannew1t2= TRUE;
         if(strcmp(argv[i],"-nooceannew2")==0)    nooceannew2= TRUE;
         if(strcmp(argv[i],"-nopalacef1")==0)     nopalacef1= TRUE;
         if(strcmp(argv[i],"-nopalacef2")==0)     nopalacef2= TRUE;
         if(strcmp(argv[i],"-nopav")==0)          nopav= TRUE;
         if(strcmp(argv[i],"-norackit")==0)       norackit= TRUE;
         if(strcmp(argv[i],"-noraster")==0)       noraster= TRUE;
         if(strcmp(argv[i],"-noseuck")==0)        noseuck= TRUE;
         if(strcmp(argv[i],"-nosnake50")==0)      nosnake50= TRUE;
         if(strcmp(argv[i],"-nosnake51")==0)      nosnake51= TRUE;
         if(strcmp(argv[i],"-nospav")==0)         nospav= TRUE;
         if(strcmp(argv[i],"-nosuper")==0)        nosuper= TRUE;
         if(strcmp(argv[i],"-notdif1")==0)        notdif1= TRUE;
         if(strcmp(argv[i],"-noturbo")==0)        noturbo= TRUE;
         if(strcmp(argv[i],"-noturr")==0)         noturr= TRUE;
         if(strcmp(argv[i],"-nousgold")==0)       nousgold= TRUE;
         if(strcmp(argv[i],"-novirgin")==0)       novirgin= TRUE;
         if(strcmp(argv[i],"-novisi")==0)         novisi= TRUE;
         if(strcmp(argv[i],"-nowild")==0)         nowild= TRUE;  


         if(strcmp(argv[i],"-noall")==0)  /* disable all scanners except 'c64 rom tape' */
         {
            noaces= TRUE;   noanirog= TRUE;   noatlantis= TRUE;
            noaudiogenic= TRUE;
            nobleep= TRUE;  noburner= TRUE;   nochr= TRUE;
            nocyber= TRUE;  noenigma= TRUE;   nofire= TRUE;
            noflash= TRUE;  nofree= TRUE;     nohit= TRUE;
            nohitec= TRUE;  nojet= TRUE;      noik= TRUE;
            nomicro= TRUE;  nonova= TRUE;     noocean= TRUE;
            nooceannew1t1= TRUE;   nooceannew1t2= TRUE;   nooceannew2= TRUE;
            nopalacef1= TRUE;   nopalacef2= TRUE;   nopav= TRUE;
            norackit= TRUE;   noraster= TRUE;   noseuck= TRUE;
            nosnake50= TRUE;  nosnake51= TRUE;  nospav= TRUE;
            nosuper= TRUE;   notdif1= TRUE;   noturbo= TRUE;
            noturr= TRUE;   nousgold= TRUE;   novirgin= TRUE;
            novisi= TRUE;   nowild= TRUE;
         }
      }
      printf("\n\nRead tolerance= %d",tol-1);



      /* PROCESS ACTIONS... */
      
      /* just test a tap if no option is present, just a filename. 
         this allows for drag and drop in explorer... */
               
      /* just 1 parameter (a filename), test for '-' ensures the 1 arg is not a switch !*/   
      if(argc==2 && argv[1][0]!='-')   
      {
         if(load_tap(argv[1]))
         {  
            printf("\n\nTesting: %s\n",argv[1]);
            time(&t1);
            if(analyze())
            {
               report();
               printf("\n\nSaved: %s",ftreportname);
               time(&t2);
               time2str(t2-t1,lin);
               printf("\nOperation completed in %s.",lin);
            }
         }
      }   
       
      else
      {
         for(i=0; i<argc; i++)
         {
            actnum= 0;
            if(strcmp(argv[i],"-t")==0)     actnum= 1;    /* test */ 
            if(strcmp(argv[i],"-o")==0)     actnum= 2;    /* optimize */
            if(strcmp(argv[i],"-ct0")==0)   actnum= 3;    /* convert to v0 */
            if(strcmp(argv[i],"-ct1")==0)   actnum= 4;    /* convert to v1 */         
            if(strcmp(argv[i],"-rs")==0)    actnum= 5;    /* fix header size */
            if(strcmp(argv[i],"-po")==0)    actnum= 6;    /* pause optimize */
            
            if(strcmp(argv[i],"-au")==0)    actnum= 7;    /* convert to au */
            if(strcmp(argv[i],"-wav")==0)   actnum= 8;    /* convert to wav */
            if(strcmp(argv[i],"-b")==0)     actnum= 9;    /* batch scan */  
            if(strcmp(argv[i],"-info")==0)  actnum= 10;   /* create info file */
                  
            
            /* This handles testing + any op that takes a tap, affects it and saves it... */
            if(actnum>0 && actnum<7)   
            {
               if(argv[i+1]!=NULL)
               {
                  if(load_tap(argv[i+1]))
                  {
                     time(&t1);
                     printf("\n\nLoaded: %s", tap.name);
                     printf("\n%s...\n",actname[actnum]);
                    
                     if(analyze())
                     {
                        switch(actnum)
                        {
                           case 2: clean();           break;
                           case 3: convert_to_v0();   break;
                           case 4: convert_to_v1();   break; 
                           case 5: fix_header_size(); 
                                   analyze();         break; 
                           case 6: convert_to_v0(); 
                                   clip_ends(); 
                                   unify_pauses();
                                   convert_to_v1();   
                                   add_trailpause();  break; 
                        }  
                        report();
                        printf("\nSaved: %s",ftreportname);
                        if(actnum>1 && actnum<6)
                        {
                           save_tap(tapoutname);
                           printf("\n\nSaved: %s",tapoutname);
                        }
                        time(&t2);
                        time2str(t2-t1,lin);
                        printf("\nOperation completed in %s.",lin);
                     }
                  }
               }
               else
                  printf("\n\nMissing file name.");
            }
                   
               
            if(actnum==7)   /* flag = convert to au */
            {
               if(argv[i+1]!=NULL)
               {
                  if(load_tap(argv[i+1]))
                  {
                     if(analyze())
                     {
                        printf("\n\nLoaded: %s", tap.name);
                        printf("\n%s...\n",actname[actnum]);
                        au_write(tap.tmem, tap.len, auoutname,sine);
                        printf("\nSaved: %s",auoutname);
                        msgout("\n");
                     }
                  }
               }
               else
                  printf("\n\nMissing file name.");
            }
            
            
            if(actnum==8)   /* flag = convert to wav */
            {
               if(argv[i+1]!=NULL)
               {
                  if(load_tap(argv[i+1]))
                  {
                     if(analyze())
                     {
                        printf("\n\nLoaded: %s", tap.name);
                        printf("\n%s...\n",actname[actnum]);
                        wav_write(tap.tmem, tap.len, wavoutname,sine);
                        printf("\nSaved: %s",wavoutname);
                        msgout("\n");
                     }
                  }
               }
               else
                  printf("\n\nMissing file name.");
            }
                        
            
            if(actnum==9)   /* flag = batch scan... */
            {
               batchmode= TRUE;
               quiet= TRUE;
               if(argv[i+1]!=NULL)
               {
                 
                  printf("\n\nBatch Scanning: %s\n",argv[i+1]);
                  batchscan(argv[i+1],incsubdirs,1);
               }
               else
               {
                  printf("\n\nMissing directory name, using current.");
                  printf("\n\nBatch Scanning: %s\n",exedir);
                  batchscan(exedir,incsubdirs,1);
   
               }
               batchmode= FALSE;
               quiet= FALSE;
            }
                    
            
            if(actnum==10)   /* flag = generate exe info file */
            {
               fp= fopen(ftinfoname,"w+t");
               if(fp!=NULL)
               {
                  printf("\n%s...\n",actname[actnum]); 
                  fprintf(fp,"%s",ftverstr);
                  fclose(fp);
               }
            }
         }
      }
   }

   printf("\n\n");

   unload_tap();
   free_crc_table();

   /* free ram from file database */
   for(i=0; i<BLKMAX; i++)
      free(blk[i]);
   
   return 0;
}
/*---------------------------------------------------------------------------
 Search the tap for all known (and enabled) file formats.

 Results are stored in the 'blk' database (an array of structs).

 If global variable 'quiet' is set (1), the scanners will not print a
 "Scanning: xxxxxxx" string. This simply reduces text output and I prefer this
 when optimizing (Search is called quite frequently).

 Note: This function calls all 'loadername_search' routines and as a result it
       fills out only about half of the fields in the blk[] database, they are...

       lt, p1, p2, p3, p4, xi (ie, all fields supported by 'addblockdef()').

       the remaining fields (below) are filled out by 'describe_blocks()' which
       calls all 'loadername_decribe()' routines for the particular format (lt).

       cs, ce, cx, rd_err, crc, cs_exp, cs_act, pilot_len, trail_len, ok.
*/
void search_tap(void)
{
   long i;
   int t;

   dbase_is_full= 0;       /* enable the "database full" warning. */
                           /* note: addblockdef sets it 1 when full. */
 
   msgout("\nScanning...");

   if(tap.changed)
   {
      for(i=0 ; i<BLKMAX; i++)   /* clear database... */
      {
         blk[i]->lt= 0;
         blk[i]->p1= 0;
         blk[i]->p2= 0;
         blk[i]->p3= 0;
         blk[i]->p4= 0;
         blk[i]->xi= 0;
         blk[i]->cs= 0;
         blk[i]->ce= 0;
         blk[i]->cx= 0;
         blk[i]->crc= 0;
         blk[i]->rd_err= 0;
         blk[i]->cs_exp= -2;
         blk[i]->cs_act= -2;
         blk[i]->pilot_len= 0;
         blk[i]->trail_len= 0;
         blk[i]->ok= 0;

         if(blk[i]->dd!= NULL)
         {
            free(blk[i]->dd);
            blk[i]->dd= NULL;
         }
         if(blk[i]->fn!= NULL)
         {
            free(blk[i]->fn);
            blk[i]->fn= NULL;
         }
      }

      /* initialize the read error table */
      for(i=0; i<100; i++)
         read_errors[i]= 0;


      /* Call the scanners... */
     
      pause_search();   /* pauses and CBM files always get searched for...  */
      cbm_search();

      /* Try to id the first CBM data file to see if its a known loader program... */
   
      t= find_decode_block(CBM_DATA,1);   /* decode first cbm data file (could hold a turbo loader) if exists... */
      if(t!=-1)
         tap.cbmcrc= compute_crc32(blk[t]->dd, blk[t]->cx);

      tap.cbmid= idloader(tap.cbmcrc);

      if(!quiet)
      {
         sprintf(lin,"\n  Loader ID: %s.\n", knam[tap.cbmid]);
         msgout(lin);
      }


      if(noid==FALSE)   /* scanning shortcuts enabled?  */
      {
         if(tap.cbmid==LID_T250 && noturbo==FALSE && !dbase_is_full && !aborted)
            turbotape_search();
         if(tap.cbmid==LID_FREE && nofree==FALSE && !dbase_is_full && !aborted)
            freeload_search();
         if(tap.cbmid==LID_NOVA && nonova==FALSE && !dbase_is_full && !aborted)
         {
            nova_spc_search();
            nova_search();
         }
         if(tap.cbmid==LID_BLEEP && nobleep==FALSE && !dbase_is_full && !aborted)
         {
            bleep_search();
            bleep_spc_search();
         }
         if(tap.cbmid==LID_OCEAN && noocean==FALSE && !dbase_is_full && !aborted)
            ocean_search();
         if(tap.cbmid==LID_CYBER && nocyber==FALSE &&!dbase_is_full)
         {
            cyberload_f1_search();
            cyberload_f2_search();
            cyberload_f3_search();
            cyberload_f4_search();
         }

         if(tap.cbmid==LID_USG && nousgold==FALSE && !dbase_is_full && !aborted)
            usgold_search();
         if(tap.cbmid==LID_ACE && noaces==FALSE && !dbase_is_full && !aborted)
            aces_search();
         if(tap.cbmid==LID_MIC && nomicro==FALSE && !dbase_is_full && !aborted)
            micro_search();
         if(tap.cbmid==LID_RAST && noraster==FALSE && !dbase_is_full && !aborted)
            raster_search();
         if(tap.cbmid==LID_CHR && nochr==FALSE && !dbase_is_full)
            chr_search();
         if(tap.cbmid==LID_BURN && noburner==FALSE && !dbase_is_full && !aborted)
            burner_search();

         /* if it's a visiload then choose correct type now...  */
         if(tap.cbmid==LID_VIS1)   visi_type = VISI_T1;
         if(tap.cbmid==LID_VIS2)   visi_type = VISI_T2;
         if(tap.cbmid==LID_VIS3)   visi_type = VISI_T3;
         if(tap.cbmid==LID_VIS4)   visi_type = VISI_T4;

         if(tap.cbmid==LID_VIS1 || tap.cbmid==LID_VIS2 || tap.cbmid==LID_VIS3 || tap.cbmid==LID_VIS4)
         {
            if(novisi==FALSE && !dbase_is_full && !aborted)
               visiload_search();
         }

         if(tap.cbmid==LID_WILD && nowild==FALSE && !dbase_is_full && !aborted)
            wild_search();
         if(tap.cbmid==LID_HIT && nohit==FALSE && !dbase_is_full && !aborted)
            hitload_search();
         if(tap.cbmid==LID_RACK && norackit==FALSE && !dbase_is_full && !aborted)
            rackit_search();
         if(tap.cbmid==LID_SPAV && nospav==FALSE && !dbase_is_full && !aborted)
            superpav_search();
         if(tap.cbmid==LID_ANI && noanirog==FALSE && !dbase_is_full && !aborted)
         {
            anirog_search();
            freeload_search();
         }
         if(tap.cbmid==LID_SUPER && nosuper==FALSE && !dbase_is_full && !aborted)
            supertape_search();
         if(tap.cbmid==LID_FIRE && nofire==FALSE && !dbase_is_full && !aborted)
            firebird_search();
         if(tap.cbmid==LID_PAV && nopav==FALSE && !dbase_is_full && !aborted)
            pav_search();
         if(tap.cbmid==LID_IK && noik==FALSE && !dbase_is_full && !aborted)
            ik_search();
         if(tap.cbmid==LID_FLASH && noflash==FALSE && !dbase_is_full && !aborted)
            flashload_search();
         if(tap.cbmid==LID_VIRG && novirgin==FALSE && !dbase_is_full && !aborted)
            virgin_search();
         if(tap.cbmid==LID_HTEC && nohitec==FALSE && !dbase_is_full && !aborted)
            hitec_search();
         if(tap.cbmid==LID_OCNEW1T1 && nooceannew1t1==FALSE && !dbase_is_full && !aborted)
            oceannew1t1_search();
         if(tap.cbmid==LID_OCNEW1T2 && nooceannew1t2==FALSE && !dbase_is_full && !aborted)
            oceannew1t2_search();
         if(tap.cbmid==LID_OCNEW2 && nooceannew2==FALSE && !dbase_is_full && !aborted)
            oceannew2_search();
         if(tap.cbmid==LID_SNAKE && nosnake50==FALSE && !dbase_is_full && !aborted)
         {
            snakeload50t1_search();
            snakeload50t2_search();
         }
         if(tap.cbmid==LID_SNAKE && nosnake51==FALSE && !dbase_is_full && !aborted)
            snakeload51_search();
         if(tap.cbmid==LID_ATLAN && noatlantis==FALSE && !dbase_is_full && !aborted)
            atlantis_search();
         if(tap.cbmid==LID_AUDIOGENIC && noaudiogenic==FALSE && !dbase_is_full && !aborted)
            audiogenic_search();

         /* todo : TURRICAN
            todo : SEUCK
            todo : JETLOAD
            todo : TENGEN   */

      }      




      /* Scan the lot.. (if shortcuts are disabled or no loader ID was found) */
      if((noid==FALSE && tap.cbmid==0) || (noid==TRUE))
      {
         if(noturbo==FALSE && !dbase_is_full && !aborted)
            turbotape_search();
         if(nofree==FALSE && !dbase_is_full && !aborted)
            freeload_search();
         if(nosnake50==FALSE && !dbase_is_full && !aborted)   /* comes here to avoid ocean misdetections  */
         {
           snakeload50t1_search();      /* snakeload is a 'safer' scanner than ocean.   */
           snakeload50t2_search();
         }
         if(nosnake51==FALSE && !dbase_is_full && !aborted)
            snakeload51_search();
         if(nonova==FALSE && !dbase_is_full && !aborted)
         {
            nova_spc_search();
            nova_search();
         }
         if(nobleep==FALSE && !dbase_is_full && !aborted)
         {
            bleep_search();
            bleep_spc_search();
         }
         if(noocean==FALSE && !dbase_is_full && !aborted)
           ocean_search();
         if(nocyber==FALSE && !dbase_is_full && !aborted)
         {
            cyberload_f1_search();
            cyberload_f2_search();
            cyberload_f3_search();
            cyberload_f4_search();

         }
         if(nousgold==FALSE && !dbase_is_full && !aborted)
            usgold_search();
         if(noaces==FALSE && !dbase_is_full && !aborted)
            aces_search();
         if(nomicro==FALSE && !dbase_is_full && !aborted)
            micro_search();
         if(noraster==FALSE && !dbase_is_full && !aborted)
            raster_search();
         if(nochr==FALSE && !dbase_is_full && !aborted)
            chr_search();
         if(noburner==FALSE && !dbase_is_full && !aborted)
            burner_search();
         if(novisi==FALSE && !dbase_is_full && !aborted)
            visiload_search();
         if(nowild==FALSE && !dbase_is_full && !aborted)
            wild_search();
         if(nohit==FALSE && !dbase_is_full && !aborted)
            hitload_search();
         if(norackit==FALSE && !dbase_is_full && !aborted)
            rackit_search();
         if(nospav==FALSE && !dbase_is_full && !aborted)
            superpav_search();
         if(noanirog==FALSE && !dbase_is_full && !aborted)
            anirog_search();
         if(nosuper==FALSE && !dbase_is_full && !aborted)
            supertape_search();
         if(nofire==FALSE && !dbase_is_full && !aborted)
            firebird_search();
         if(nopav==FALSE && !dbase_is_full && !aborted)
            pav_search();
         if(noik==FALSE && !dbase_is_full && !aborted)
            ik_search();
         if(noturr==FALSE && !dbase_is_full && !aborted)
            turrican_search();
         if(noseuck==FALSE && !dbase_is_full && !aborted)
            seuck1_search();
         if(nojet==FALSE && !dbase_is_full && !aborted)
            jetload_search();
         if(noflash==FALSE && !dbase_is_full && !aborted)
           flashload_search();
         if(novirgin==FALSE && !dbase_is_full && !aborted)
            virgin_search();
         if(nohitec==FALSE && !dbase_is_full && !aborted)
            hitec_search();
         if(notdif1==FALSE && !dbase_is_full && !aborted)
            tdi_search();
         if(nooceannew1t1==FALSE && !dbase_is_full && !aborted)
            oceannew1t1_search();
         if(nooceannew1t2==FALSE && !dbase_is_full && !aborted)
            oceannew1t2_search();
         if(nooceannew2==FALSE && !dbase_is_full && !aborted)
            oceannew2_search();
         if(noatlantis==FALSE && !dbase_is_full && !aborted)
            atlantis_search();
         if(nopalacef1==FALSE && !dbase_is_full && !aborted)
            palacef1_search();
         if(nopalacef2==FALSE && !dbase_is_full && !aborted)
            palacef2_search();
         if(noenigma==FALSE && !dbase_is_full && !aborted)
            enigma_search();
         if(noaudiogenic==FALSE && !dbase_is_full && !aborted)
            audiogenic_search();
      }
      
      sort_blocks();   /* sort the blocks into order of appearance */
      gap_search();    /* add any gaps to the block list */
      sort_blocks();   /* sort the blocks into order of appearance */

      tap.changed=0;   /* important to clear this. */

      if(quiet)
         msgout("  Done.");
   }
   else
      msgout("  Done.");
}
/*---------------------------------------------------------------------------
 Pass this function a valid row number (i) from the file database (blk) and
 it calls the describe function for that file format which will decode
 any data in the block and fill in most of the  missing info for that file.

 Note: Any "additional" (unstored in the database) text info will be available
 in the global buffer 'info'. (this could be improved).
*/
void describe_file(int row)
{
   int t;
   t= blk[row]->lt;

   sprintf(info,"");   /* clear the extra information buffer so it's ready to receive */

   switch(t)
   {
      case GAP:            gap_describe(row);            break;
      case PAUSE:          pause_describe(row);          break;
      case CBM_HEAD:       cbm_describe(row);            break;
      case CBM_DATA:       cbm_describe(row);            break;
      case USGOLD:         usgold_describe(row);         break;
      case TT_HEAD:        turbotape_describe(row);      break;
      case TT_DATA:        turbotape_describe(row);      break;
      case FREE:           freeload_describe(row);       break;
      case CHR_T1:         chr_describe(row);            break;
      case CHR_T2:         chr_describe(row);            break;
      case CHR_T3:         chr_describe(row);            break;
      case NOVA:           nova_describe(row);           break;
      case NOVA_SPC:       nova_spc_describe(row);       break;
      case WILD:           wild_describe(row);           break;
      case WILD_STOP:      wild_describe(row);           break;
      case ACES:           aces_describe(row);           break;
      case OCEAN_F1:       ocean_describe(row);          break;
      case OCEAN_F2:       ocean_describe(row);          break;
      case OCEAN_F3:       ocean_describe(row);          break;
      case RASTER:         raster_describe(row);         break;
      case VISI_T1:        visiload_describe(row);       break;
      case VISI_T2:        visiload_describe(row);       break;
      case VISI_T3:        visiload_describe(row);       break;
      case VISI_T4:        visiload_describe(row);       break;
      case CYBER_F1:       cyberload_f1_describe(row);   break;
      case CYBER_F2:       cyberload_f2_describe(row);   break;
      case CYBER_F3:       cyberload_f3_describe(row);   break;
      case CYBER_F4_1:     cyberload_f4_describe(row);   break;
      case CYBER_F4_2:     cyberload_f4_describe(row);   break;
      case CYBER_F4_3:     cyberload_f4_describe(row);   break;
      case BLEEP:          bleep_describe(row);          break;
      case BLEEP_TRIG:     bleep_describe(row);          break;
      case BLEEP_SPC:      bleep_spc_describe(row);      break;
      case HITLOAD:        hitload_describe(row);        break;
      case MICROLOAD:      micro_describe(row);          break;
      case BURNER:         burner_describe(row);         break;
      case RACKIT:         rackit_describe(row);         break;
      case SPAV1_HD:       superpav_describe(row);       break;
      case SPAV2_HD:       superpav_describe(row);       break;
      case SPAV1:          superpav_describe(row);       break;
      case SPAV2:          superpav_describe(row);       break;
      case VIRGIN:         virgin_describe(row);         break;
      case HITEC:          hitec_describe(row);          break;
      case ANIROG:         anirog_describe(row);         break;
      case SUPERTAPE_HEAD: supertape_describe(row);      break;
      case SUPERTAPE_DATA: supertape_describe(row);      break;
      case PAV:            pav_describe(row);            break;
      case IK:             ik_describe(row);             break;
      case FBIRD1:         firebird_describe(row);       break;
      case FBIRD2:         firebird_describe(row);       break;
      case TURR_HEAD:      turrican_describe(row);       break;
      case TURR_DATA:      turrican_describe(row);       break;
      case SEUCK_L2:       seuck1_describe(row);         break;
      case SEUCK_HEAD:     seuck1_describe(row);         break;
      case SEUCK_DATA:     seuck1_describe(row);         break;
      case SEUCK_TRIG:     seuck1_describe(row);         break;
      case SEUCK_GAME:     seuck1_describe(row);         break;
      case JET:            jetload_describe(row);        break;
      case FLASH:          flashload_describe(row);      break;
      case TDI_F1:         tdi_describe(row);            break;
      case OCNEW1T1:       oceannew1t1_describe(row);    break;
      case OCNEW1T2:       oceannew1t2_describe(row);    break;
      case ATLAN:          atlantis_describe(row);       break;
      case SNAKE51:        snakeload51_describe(row);    break;
      case SNAKE50T1:      snakeload50t1_describe(row);  break;
      case SNAKE50T2:      snakeload50t2_describe(row);  break;
      case PAL_F1:         palacef1_describe(row);       break;
      case PAL_F2:         palacef2_describe(row);       break;
      case OCNEW2:         oceannew2_describe(row);      break;
      case ENIGMA:         enigma_describe(row);         break;
      case AUDIOGENIC:     audiogenic_describe(row);     break;
   }
}

/*---------------------------------------------------------------------------
 Describe every file in the database.
 Also records a crc32 for each data-extracted file and updates the tap read-error
 record.
*/
void describe_files(void)
{
   int i,t,re;

   tap.total_read_errors= 0;

   for(i=0; blk[i]->lt!=0; i++)
   {
      describe_file(i);

      t= blk[i]->lt;

      /* get generic info that all data blocks have...   */
      if(t>PAUSE)
      {
         /* make crc32 if the block has been data extracted. */
         if(blk[i]->dd!=NULL)
            blk[i]->crc= compute_crc32(blk[i]->dd, blk[i]->cx);

         re= blk[i]->rd_err;
         tap.total_read_errors+= re;
      }
   }
}
/*---------------------------------------------------------------------------
 Read 1 pulse from the tap file offset at 'pos', decide whether it is a Bit0 or Bit1
 according to the values in the parameters. (+/- global tolerance value 'tol')
 lp = ideal long pulse width.
 sp = ideal short pulse width.
 tp = threshold pulse width (can be NA if unknown)

 Return (bit value) 0 or 1 on success, else -1 on failure.

 Note: Most formats can use this (and readttbyte()) for reading data, but some
 (ie. cbmtape, pavloda, visiload,supertape etc) have custom readers held in their
 own source files.
*/
int readttbit(int pos, int lp, int sp, int tp)
{
   int valid,v,b;

   if(pos<20 || pos>tap.len-1) /* return error if out of bounds.. */
      return -1;
   if(is_pause_param(pos))     /* return error if offset is on a pause.. */
      return -1;

   valid= 0;
   b= tap.tmem[pos];
   if(tp!=NA)  /* exact threshold pulse IS available...  */
   {
      if(b<tp && b>sp-tol)   /* its a SHORT (Bit0) pulse...  */
      {
         v=0;
         valid+=1;
      }
      if(b>tp && b<lp+tol)   /* its a LONG (Bit1) pulse... */
      {
         v=1;
         valid+=2;
      }
      if(b==tp)              /* its ON the threshold!...  */
         valid+=4;
   }
   else  /* threshold unknown? ...use midpoint method...  */
   {
      if(b>(sp-tol) && b<(sp+tol))    /* its a SHORT (Bit0) pulse...*/
      {
         valid+=1;
         v=0;
      }
      if(b>(lp-tol) && b<(lp+tol))    /* its a LONG (Bit1) pulse...*/
      {
         valid+=2;
         v=1;
      }
      if(valid==3)  /* pulse qualified as 0 AND 1!, use closest match...*/
      {
         if((abs(lp-b)) > (abs(sp-b)))
            v=0;
         else
            v=1;
      }
   }
   if(valid==0)  /* Error, pulse didnt qualify as either Bit0 or Bit1...*/
   {
      add_read_error(pos);
      v=-1;
   }
   if(valid==4)  /* Error, pulse is ON the threshold...*/
   {
      add_read_error(pos);
      v=-1;
   }
   return v;
}
/*---------------------------------------------------------------------------
 Generic "READ_BYTE" function, (can be used by most turbotape formats)
 Reads and decodes 8 pulses from 'pos' in the TAP file.
 parameters...
 lp = ideal long pulse width.
 sp = ideal short pulse width.
 tp = threshold pulse width (can be NA if unknown)
 return 0 or 1 on success else -1.
 endi = endianess (use constants LSbF or MSbF).
 Returns byte value on success, or -1 on failure.

 Note: Most formats can use this (and readttbit) for reading data, but some
 (ie. cbmtape, pavloda, visiload,supertape etc) have custom readers held in their
 own source files.
*/
int readttbyte(int pos, int lp, int sp, int tp, int endi)
{
   int i,v,b;
   unsigned char byte[10];

   /* check next 8 pulses are not inside a pause and *are* inside the TAP...  */
   for(i=0; i<8; i++)
   {
      b= readttbit(pos+i,lp,sp,tp);
      if(b==-1)
         return -1;
      else
         byte[i]= b;
   }

   /* if we get this far, we have 8 valid bits... decode the byte...  */
   if(endi==MSbF)
   {
      v=0;
      for(i=0; i<8; i++)
      {
         if(byte[i]==1)
            v+=(128>>i);
      }
   }
   else
   {
      v=0;
      for(i=0; i<8; i++)
      {
         if(byte[i]==1)
            v+=(1<<i);
      }
   }
   return v;
}
/*---------------------------------------------------------------------------
 Search for pilot/sync sequence at offset 'pos' in tap file...
 'fmt' should be the numeric ID (or constant) of a format described in ft[].

 Returns 0 if no pilot found.
 Returns end position if pilot found (and a legal quantity of them).
 Returns end position (negatived) if pilot found but too few/many.

 Note: End position = file offset of 1st NON pilot value, ie. a sync byte.
*/
int find_pilot(int pos, int fmt)
{
   int z,sp,lp,tp,en,pv,sv,pmin,pmax;

   if(pos<20)
      return 0;

   sp= ft[fmt].sp;
   lp= ft[fmt].lp;
   tp= ft[fmt].tp;
   en= ft[fmt].en;
   pv= ft[fmt].pv;
   sv= ft[fmt].sv;
   pmin= ft[fmt].pmin;
   pmax= ft[fmt].pmax;

   if(pmax==NA)
      pmax=200000;    /* set some crazy limit if pmax is NA (NA=0) */

   if((pv==0 || pv==1) && (sv==0 || sv==1))  /* are the pilot/sync BIT values?... */
   {
      if(readttbit(pos,lp,sp,tp)==pv)  /* got pilot bit? */
      {
         z=0;
         while(readttbit(pos,lp,sp,tp)==pv && pos < tap.len)  /* skip all pilot bits.. */
         {
            z++;
            pos++;
         }
         if(z==0)
            return 0;
         if(z<pmin || z>pmax)     /* too few/many pilots?, return position as negative. */
            return -pos;
         if(z>=pmin && z<=pmax)   /* enough pilots?, return position. */
            return pos;
      }
   }

   else  /* Pilot/sync are BYTE values... */
   {
      if(readttbyte(pos,lp,sp,tp,en)==pv)  /* got pilot byte? */
      {
         z=0;
         while(readttbyte(pos,lp,sp,tp,en)==pv && pos<tap.len)  /* skip all pilot bytes.. */
         {
            z++;
            pos+=8;
         }
         if(z==0)
            return 0;
         if(z<pmin || z>pmax)     /* too few/many pilots?, return position as negative. */
            return -pos;
         if(z>=pmin && z<=pmax)   /* enough pilots?, return position. */
            return pos;
      }
   }
   return 0;  /* never reached. */
}
/*---------------------------------------------------------------------------
 Load the named tap file into buffer tap.tmem[]
 return 1 on success, 0 on error.
*/
int load_tap(char *name)
{
   FILE *fp;
   int flen;

   fp= fopen(name,"rb");
   if(fp!=NULL)
   {
      unload_tap();

      fseek(fp,0,SEEK_END);
      flen= ftell(fp);
      rewind(fp);
      tap.tmem= (unsigned char*)malloc(flen);
      if(tap.tmem==NULL)
      {
         msgout("\nError: malloc failed in load_tap().");
         fclose(fp);
         return 0;
      }

      fread(tap.tmem, flen, 1, fp);
      fclose(fp);

      tap.len= flen;

      /* the following were uncommented in GUI app..
         these now appear in main()...
         tol=DEFTOL;        reset tolerance..
         debugmode=FALSE;      reset "debugging" mode. */

      /* if desired, preserve loader table between TAPs...  */
      if(preserveloadertable==FALSE)
         reset_loader_table();
      
      tap.changed= 1;  /* makes sure the tap is fully scanned afterwards. */

      strcpy(tap.path, name);                 
      getfilename(tap.name, name);
       
      return 1;  /* 1= loaded ok */
   }
   else
   {
      msgout("\nError: Couldn't open file in load_tap().");
      return 0;
   }
}
/*---------------------------------------------------------------------------
 Save buffer tap.tmem[] to a named file.
 Return 1 on success, 0 on error.
*/
int save_tap(char *name)
{
   FILE *fp;

   fp= fopen(name,"w+b");
   if(fp==NULL || tap.tmem==NULL)
      return 0;
   fwrite(tap.tmem, tap.len, 1, fp);
   fclose(fp);
   return 1;
}
/*---------------------------------------------------------------------------
 Erase all stored data for the loaded tap, free buffers.
*/
void unload_tap(void)
{
   int i;

   strcpy(tap.path,"");
   strcpy(tap.name,"");
   strcpy(tap.cbmname,"");
   if(tap.tmem!=NULL)
   {
      free(tap.tmem);
      tap.tmem=NULL;
   }
   tap.len=0;
   for(i=0; i<256; i++)
   {
     tap.pst[i]=0;
     tap.fst[i]=0;
   }
   tap.fsigcheck=0;
   tap.fvercheck=0;
   tap.fsizcheck=0;
   tap.detected=0;
   tap.detected_percent=0;
   tap.purity=0;
   tap.total_gaps=0;
   tap.total_data_files=0;
   tap.total_checksums=0;
   tap.total_checksums_good=0;
   tap.optimized_files=0;
   tap.total_read_errors=0;
   tap.fdate=0;
   tap.taptime=0;
   tap.version=0;
   tap.bootable=0;
   tap.changed=0;
   tap.crc=0;
   tap.cbmcrc=0;
   tap.cbmid=0;
   tap.tst_hd=0;
   tap.tst_rc=0;
   tap.tst_op=0;
   tap.tst_cs=0;
   tap.tst_rd=0;

   for(i=0 ;i<BLKMAX; i++)   /* empty the file datbase... */
   {
      blk[i]->lt= 0;
      blk[i]->p1= 0;
      blk[i]->p2= 0;
      blk[i]->p3= 0;
      blk[i]->p4= 0;
      blk[i]->xi= 0;
      blk[i]->cs= 0;
      blk[i]->ce= 0;
      blk[i]->cx= 0;
      blk[i]->crc= 0;
      blk[i]->rd_err= 0;
      blk[i]->cs_exp= -2;
      blk[i]->cs_act= -2;
      blk[i]->pilot_len= 0;
      blk[i]->trail_len= 0;
      blk[i]->ok= 0;

      if(blk[i]->dd!=NULL)   /* (note: search_tap() also frees these before search.) */
      {
         free(blk[i]->dd);
         blk[i]->dd= NULL;
      }
      if(blk[i]->fn!=NULL)   /* (note: search_tap() also frees these before search.) */
      {
         free(blk[i]->fn);
         blk[i]->fn= NULL;
      }
   }
   for(i=0 ;i<BLKMAX; i++)   /* empty the prg datbase...  */
   {
      prg[i].lt= 0;
      prg[i].cs= 0;
      prg[i].ce= 0;
      prg[i].cx= 0;
      if(prg[i].dd!=NULL)
      {
         free(prg[i].dd);
         prg[i].dd= NULL;
      }
      prg[i].errors= 0;
   }
}
/*---------------------------------------------------------------------------
 Perform a full analysis of the tap file...
 Return 0 if file is not a valid TAP, else 1.
*/
int analyze(void)
{
   double per;

   if(tap.tmem==NULL)
      return 0;        /* no tap file loaded */

   tap.fsigcheck= check_signature();
   tap.fvercheck= check_version();
   tap.fsizcheck= check_size();

   if(tap.fsigcheck==1 && tap.fvercheck==1 && tap.fsizcheck==1)
   {
      msgout("\nError: File is not a valid C64 TAP.");   /* all header checks failed */
      return 0;
   }

   tap.taptime= get_duration(20,tap.len);

   /* now call search_tap() to fill the file database (blk)  */
   /* + call describe_blocks() to extract data and get checksum info. */

   note_errors= FALSE;
   search_tap();
   note_errors= TRUE;
   describe_files();
   note_errors= FALSE;
 

   tap.purity= get_pulse_stats();    
   get_file_stats();

   tap.optimized_files = count_opt_files();
   tap.total_checksums_good = count_good_checksums();
   tap.detected = count_rpulses();
   tap.bootable = count_bootparts();
  
   per= ((double)tap.detected/((double)tap.len-20)) *100;
   tap.detected_percent= floor(per);
 
   if(tap.fsigcheck==0 && tap.fvercheck==0 && tap.fsizcheck==0)
      tap.tst_hd=0; else tap.tst_hd=1;
   if(tap.detected==(tap.len-20))
      tap.tst_rc=0; else tap.tst_rc=1;
   if(tap.total_checksums_good == tap.total_checksums)
      tap.tst_cs=0; else tap.tst_cs=1;
   if(tap.total_read_errors ==0)
      tap.tst_rd=0; else tap.tst_rd=1;
   if(tap.total_data_files - tap.optimized_files ==0)
      tap.tst_op=0; else tap.tst_op=1;

   tap.crc= compute_overall_crc();

   if(!batchmode)
   {
      make_prgs();
      save_prgs();
   }   
   return 1;
}
/*---------------------------------------------------------------------------
 Save a report for this TAP file.
 Note: Call 'analyze()' before calling this!.
*/
void report(void)
{
   int i;
   FILE *fp;
   char *rbuf;

   /* if in batch mode just show the percentage detected... */
   if(batchmode)     
   {
      sprintf(lin," (%d%%)",tap.detected_percent);
      msgout(lin);
      return;
   }

   rbuf=(char*)malloc(2000000);
   if(rbuf==NULL)
   {
      msgout("\nError: malloc failed in report().");
      exit(1);
   }

   chdir(exedir);

   fp= fopen(tempftreportname,"r");  /* delete any existing temp file... */
   if(fp!=NULL)
   {
      fclose(fp);
      sprintf(lin,"del %s",tempftreportname);
      system(lin);
   }
   fp= fopen(ftreportname,"r");     /* delete any existing report file... */
   if(fp!=NULL)
   {
      fclose(fp);
      sprintf(lin,"del %s",ftreportname);
      system(lin);
   }     

   fp= fopen(tempftreportname,"w+t");    /* create new report file... */

   if(fp!=NULL)
   {
      /* include results and general info... */
      print_results(rbuf);
      fprintf(fp,rbuf);
      fprintf(fp,"\n\n\n\n\n");
      fprintf(stdout,rbuf);         /* show this first part onscreen. */


      /* include file stats... */
      print_file_stats(rbuf);
      fprintf(fp,rbuf);
      fprintf(fp,"\n\n\n\n\n");

      /* include database in the file (partially interpreted)... */
      print_database(rbuf);
      fprintf(fp,rbuf);
      fprintf(fp,"\n\n\n\n\n");

      /* include pulse stats in the file... */
      print_pulse_stats(rbuf);
      fprintf(fp,rbuf);
      fprintf(fp,"\n\n\n\n\n");

      /* include 'read errors' report in the file... */
      if(tap.total_read_errors!=0)
      {
         fprintf(fp,"\n * Read error locations (Max 100)");
         fprintf(fp,"\n");
         for(i=0; read_errors[i]!=0; i++)
            fprintf(fp,"\n0x%04X",read_errors[i]);
         fprintf(fp,"\n\n\n\n\n");
      }
      
      fprintf(fp,"\nGenerated by Final TAP %0.2f console - (c) Subchrist Software. 2001-%d", FTVERSION,FTYEAR);
      fprintf(fp,"\n\n");
      fclose(fp);

      sprintf(lin,"ren %s %s",tempftreportname,ftreportname);  
      system(lin);   
   }
   else
      msgout("\nError: failed to create report file.");     

   free(rbuf);
} 
/*---------------------------------------------------------------------------
 Return the number of pulses accounted for in total across all known files.
*/
int count_rpulses(void)
{
   int i,tot;

   /* add up number of pulses accounted for...     */
   for(i=0,tot=0; blk[i]->lt!=0; i++)  /* for each block entry in blk */
   {
      if(blk[i]->lt!=GAP)
      {
         if(blk[i]->p1!=0 && blk[i]->p4!=0)  /* start and end addresses both present?  */
            tot+=(blk[i]->p4 - blk[i]->p1) +1;
      }
   }
   return tot;
}
/*---------------------------------------------------------------------------
 Look at the TAP header and verify signature as C64 TAP.
 Return 0 if ok else 1.
*/
int check_signature(void)
{
   int i;

   for(i=0; i<12; i++)    /* copy 1st 12 chars, strncpy fucks it up somehow. */
      lin[i]=tap.tmem[i];
   lin[i]=0;

   if(strcmp(lin,"C64-TAPE-RAW")==0)
      return 0;
   else
      return 1;
}
/*---------------------------------------------------------------------------
 Look at the TAP header and verify version, currently 0 and 1 are valid versions.
 Sets 'version' variable and returns 0 on success, else returns 1.
*/
int check_version(void)
{
   int b;

   b= tap.tmem[12];
   if(b==0 || b==1)
   {
      tap.version= b;
      return 0;
   }
   else
      return 1;
}
/*---------------------------------------------------------------------------
 Verifies the TAP header 'size' field.
 Returns 0 if ok, else 1.
*/
int check_size(void)
{
   int sz;

   sz = tap.tmem[16]+ (tap.tmem[17]<<8)+ (tap.tmem[18]<<16)+ (tap.tmem[19]<<24);
   if(sz== tap.len-20)
      return 0;
   else
      return 1;
}
/*---------------------------------------------------------------------------
 Return the duration in seconds between p1 and p2.
 p1 and p2 should be valid offsets within the scope of the TAP file.
*/
float get_duration(int p1, int p2)
{
   long i;
   long zsum;
   double tot=0;
   double p= (double)20000/CPS;
   float apr;

   for(i=p1; i<p2; i++)
   {
      if(tap.tmem[i]!=0)   /* handle normal pulses (non-zeroes) */
         tot+= ((double)(tap.tmem[i]*8) / CPS);
      if(tap.tmem[i]==0 && tap.version==0)  /* handle v0 zeroes.. */
         tot+= p;
      if(tap.tmem[i]==0 && tap.version==1)  /* handle v1 zeroes.. */
      {
         zsum = tap.tmem[i+1] + (tap.tmem[i+2]<<8) + (tap.tmem[i+3]<<16);
         tot+= (double)zsum/CPS;
         i+=3;
      }
   }
   apr= (float)tot;   /* approximate and return number of seconds. */
   return apr;
}
/*---------------------------------------------------------------------------
 Return the number of unique pulse widths in the TAP.
 Note: Also fills tap.pst[256] array with distribution stats.
*/
int get_pulse_stats(void)
{
   int i,tot,b;

   for(i=0; i<256; i++)    /* clear pulse table...  */
      tap.pst[i]=0;

   for(i=20; i<tap.len; i++)   /* create pulse table... */
   {
      b= tap.tmem[i];
      if(b==0 && tap.version==1)
         i+=3;
      else
         tap.pst[b]++;
   }

   /* add up pulse types... */
   tot=0;
   for(i=1; i<256; i++)  /* Note the start at 1 (pauses dont count) */
   {
      if(tap.pst[i]!=0)
        tot++;
   }
   return tot;
}
/*---------------------------------------------------------------------------
 Count all file types found in the TAP and their quantities.
 Also records the number of data files, checksummed data files and gaps in the TAP.
*/
void get_file_stats(void)
{
   int i;
   for(i=0; i<256; i++)  /* init table */
      tap.fst[i]= 0;

   for(i=0; blk[i]->lt!=0; i++)  /* count all contained filetype occurences... */
      tap.fst[blk[i]->lt]++;

   tap.total_data_files= 0;
   tap.total_checksums= 0;
   tap.total_gaps= 0;

   for(i=0; ft[i].sp!=666; i++)  /* for each file format in ft[]...  */
   {
      if(tap.fst[i]!=0)
      {
         if(ft[i].has_cs==CSYES)
            tap.total_checksums+= tap.fst[i];  /* count all available checksums.  */
      }
      if(i>PAUSE)
         tap.total_data_files+= tap.fst[i];    /* count data files  */
      if(i==GAP)
         tap.total_gaps+= tap.fst[i];          /* count gaps   */
   }
}
/*---------------------------------------------------------------------------
 Adds a block definition (file details) to the database (blk)...

 Only sof & eof must be assigned (legal) values for the block, the others can be 0.
 On success, returns the slot number that the block went to,
 On failure, (invalid block definition) returns DBERR (-1).
 On failure, (database full) returns DBFULL (-2).
*/
int addblockdef(int lt,int sof,int sod,int eod,int eof,int xi)
{
   int i,slot,e1,e2;

   if(debug==FALSE)
   {
      /* check that the block does not conflict with any existing blocks... */
      for(i=0; blk[i]->lt!=0; i++)
      {
         e1= blk[i]->p1;   /* get existing block start pos  */
         e2= blk[i]->p4;   /* get existing block end pos   */

         if(!((sof<e1 && eof<e1) || (sof>e2 && eof>e2)))
            return DBERR;
      }
   }

   if((sof>19 && eof<tap.len) && (eof>=sof))
   {
      /* find the first free slot (containing 0 in 'lt' field)...  */

      /* note: slot blk[BLKMAX-1] is reserved for the list terminator.
         the last usable slot is therefore BLKMAX-2.    */

      for(i=0; blk[i]->lt!=0; i++)
      ;
      slot=i;

      if(slot==BLKMAX-1) /* only clear slot is the last one? (the terminator) */
      {
         if(dbase_is_full==FALSE)  /* we only need give the error once */
         {
            if(!batchmode)   /* dont bother with the warning in batch mode.. */
               msgout("\n\nWarning: FT's database is full...\nthe report will not be complete.\nTry optimizing.\n\n");
            dbase_is_full= TRUE;
         }
         return DBFULL;
      }
      else
      {
         /* put the block in the last available slot... */
         blk[slot]->lt= lt;
         blk[slot]->p1= sof;
         blk[slot]->p2= sod;
         blk[slot]->p3= eod;
         blk[slot]->p4= eof;
         blk[slot]->xi= xi;

         blk[slot]->cs= 0;    /* just clear out the remaining fields (xxx_describe fills these in)...  */
         blk[slot]->ce= 0;
         blk[slot]->cx= 0;
         blk[slot]->dd= NULL;
         blk[slot]->crc= 0;
         blk[slot]->rd_err= 0;
         blk[slot]->cs_exp= -2;
         blk[slot]->cs_act= -2;
         blk[slot]->pilot_len= 0;
         blk[slot]->trail_len= 0;
         blk[slot]->fn= NULL;
         blk[slot]->ok= 0;
      }
   }
   else
      return DBERR;

   return slot;  /* ok, entry added successfully.   */
}
/*---------------------------------------------------------------------------
 Sort the database by p1 (file start position, sof) values.
*/
void sort_blocks(void)
{
   int i,swaps,size;
   struct blk_t *tmp;

   for(i=0; blk[i]->lt!=0 && i<BLKMAX; i++)
   ;
   size= i;  /* store current size of database */
   
   do
   {
      swaps= 0;
      for(i=0; i<size-1; i++)
      {
         /* examine file sof's (p1's), swap if necessary...  */
         if((blk[i]->p1) > (blk[i+1]->p1))
         {
            tmp= blk[i];
            blk[i]= blk[i+1];
            blk[i+1]= tmp;
            swaps++;  
         }
      }
   }
   while(swaps!=0);  /* repeat til no swaps occur.  */
}
/*---------------------------------------------------------------------------
 Counts the number of standard CBM boot sequence(s) (HEAD,HEAD,DATA,DATA) in
 the current file database.
 Returns the number found.
*/
int count_bootparts(void)
{
   int tblk[BLKMAX];
   int i,j,bootparts;
  
   /* make a block list (types only) without gaps/pauses included.. */
   for(i=0,j=0; blk[i]->lt!=0; i++)
   {
      if(blk[i]->lt > 2)
         tblk[j++]= blk[i]->lt;
   }
   tblk[j]=0;

   /* count bootable sections... */
   bootparts=0;
   for(i=0; i+3<j; i++)
   {
      if(tblk[i+0]==CBM_HEAD && tblk[i+1]==CBM_HEAD && tblk[i+2]==CBM_DATA && tblk[i+3]==CBM_DATA)
         bootparts++;
   }
   return bootparts;
}
/*---------------------------------------------------------------------------
  Calls upon functions found in clean.c to optimize and correct the TAP.
*/
void clean(void)
{
   int x;

   if(tap.tmem==NULL)
   {
      msgout("\nError: No TAP file loaded!.");
      return;
   }
   if(debug)
   {
      msgout("\nError: Optimization is disabled whilst in debugging mode.");
      return;
   }

   quiet= 1;   /* no talking between search routines and worklog  */

   convert_to_v0();        /* unpack pauses to V0 format if not already */
   clip_ends();            /* clip leading and trailing pauses */
   unify_pauses();         /* connect/rebuild any consecutive pauses */
   clean_files();          /* force perfect pulsewidths on known blocks */
   convert_to_v1();        /* repack pauses (v1 format) */

   fill_cbm_tone();        /* presently fills any gap of around 80 pulses  
                             (following a CBM block) with ft[CBM_HEAD].sp's. */

   /* this loop repairs pilot bytes and small gaps surrounding pauses, if
      any pauses are inserted by 'insert_pauses()' then new gaps and pilots
      may be identified in which case we repeat the loop until they are all
      dealt with. */                          
   do
   {
      fix_pilots();           /* replace broken pilots with new ones. */
      fix_prepausegaps();     /* fix all pre pause "spike runs" of 1 2 or 3. */
      fix_postpausegaps();    /* fix all post pause "spike runs" of 1 2 or 3. */
      x=insert_pauses();      /* insert pauses between blocks that need one. */
   }
   while(x);

   standardize_pauses();   /* standardize CBM HEAD -> CBM PROG pauses. */
   fix_boot_pilot();       /* add new $6A00 pulse pilot on a CBM boot header. */
   cut_postdata_gaps();    /* cuts post-data gaps <20 pulses. */

   if(noaddpause==FALSE)
      add_trailpause();     /* add a 5 second trailing pause   */

   fix_bleep_pilots();      /* correct any corrupted bleepload pilots */

   msgout("\n");
   msgout("\nCleaning finished.");
   quiet=0;   /* allow talking again. */
}
/*---------------------------------------------------------------------------
 Searches the file database for gaps. adds a definition for any found.
 Note: Must be called ONLY after sorting the database!.
 Note : The database MUST be re-sorted after a GAP is added!.
*/
void gap_search(void)
{
   int i,p1,p2,sz;

   p1= 20;          /* choose start of TAP and 1st blocks first pulse  */
   p2= blk[0]->p1;
   if(p1<p2)
   {
      sz= p2-p1;
      addblockdef(GAP,p1,0,0,p2-1, sz);
      sort_blocks();
   }

   for(i=0; blk[i]->lt!=0 && blk[i+1]->lt!=0 ;i++)    /* double dragon sticks in this loop */
   {
      p1 = blk[i]->p4;        /* get end of this block */
      p2 = blk[i+1]->p1;      /* and start of next  */
      if(p1<(p2-1))
      {
         sz= (p2-1)-p1;
         if(sz>0)
         {
            addblockdef(GAP,p1+1,0,0,p2-1, sz);
            sort_blocks();
         }
      }
   }
   
   p1= blk[i]->p4;   /* choose last blocks last pulse and End of TAP */
   p2= tap.len-1;
   if(p1<p2)
   {
      sz= p2-p1;
      addblockdef(GAP,p1+1,0,0,p2, sz);
      sort_blocks();
   }
}
/*---------------------------------------------------------------------------
 Write a description of a GAP file to global buffer 'info' for inclusion in
 the report.
*/
void gap_describe(int row)
{
   strcpy(lin,"");
   if(blk[row]->xi >1)
      sprintf(lin,"\n - Length = %d pulses",blk[row]->xi);
   else
      sprintf(lin,"\n - Length = %d pulse",blk[row]->xi);
   strcpy(info,lin);
}
/*---------------------------------------------------------------------------
 Check whether the offset 'x' is accounted for in the database (by a data file
 or a pause, not a gap), return 1 if it is, else 0.
*/
int is_accounted(int x)
{
   int i;

   for(i=0; blk[i]->lt!=0; i++)
   {
      if(blk[i]->lt!=GAP)
      {
         if((x>=blk[i]->p1) && (x<=blk[i]->p4))
            return 1;
      }
   }
   return 0;
}
/*---------------------------------------------------------------------------
 Add together all data file CRC32's
*/
int compute_overall_crc(void)
{
   int i,tot=0;
   for(i=0; blk[i]->lt!=0; i++)
      tot+= blk[i]->crc;
   return tot;
}
/*---------------------------------------------------------------------------
 Returns the number of imperfect pulsewidths found in the file at database
 entry 'slot'.
*/
int count_unopt_pulses(int slot)
{
   int i,t,b,imperfect;

   t= blk[slot]->lt;

   for(imperfect=0, i= blk[slot]->p1; i < blk[slot]->p4+1; i++)
   {
      b= tap.tmem[i];
      if(b!=ft[t].sp && b!=ft[t].mp && b!=ft[t].lp)
         imperfect++;
   }
   return imperfect;
}
/*---------------------------------------------------------------------------
 Return a count of files in the database that are 100% optimized...
*/
int count_opt_files(void)
{
   int i,n;

   for(n=0,i=0; blk[i]->lt!=0; i++)
   {
      if(blk[i]->lt > PAUSE)
      {
         if(count_unopt_pulses(i)==0)
            n++;
      }
   }
   return n;
}

/*---------------------------------------------------------------------------
 Checks whether location 'ofst' holds a valid tape pulse of type 'width'
 Return 1 if it is, 0 if not.
*/
int is_pulse(int ofst, int width)
{
   if(ofst<20 || ofst>tap.len-1 || is_pause_param(ofst))    
      return 0;
   if(tap.tmem[ofst]>(width-tol) && tap.tmem[ofst]<(width+tol))   /* pulse at ofst qualifies? */
      return 1;
   return 0;       /* pulse failed to qualify */  
}
/*---------------------------------------------------------------------------
 Checks whether location 'ofst' is inside a pause. (harder than it sounds!)
 Return 1 if it is, 0 if not.
*/
int is_pause_param(int ofst)
{
   int i,z,pos;

   if(ofst<20 || ofst>tap.len-1)   /* ofst is out of bounds  */
      return 0;

   if(tap.tmem[ofst]==0)     /* ofst is pointing at a zero! */
      return 1;

   if(tap.version==0)   /* previous 'if' would have dealt with v0. the rest is v1 only */
      return 0;
   
   if(ofst<24)    /* test very beginning of TAP file, ensures no rewind into header! */
   {
      if(tap.tmem[20]==0)
         return 1;
      else
         return 0;
   }

   pos= ofst-4;    /* pos will be at least 20 */

   do   /* find first 4 pulses containing no zeroes (behind ofst)...  */
   {
      z=0;
      for(i=0; i<4; i++)
      {
         if(tap.tmem[pos+i]==0)
            z++;
      }
      if(z!=0)
         pos--;
   }
   while(z!=0 && pos>19);
 

   if(z==0)  /* if TRUE, we found the first 4 containing no zeroes (behind p)  */
   {
      pos+=4;    /* pos now points to first v1 pause (a zero)  */

      /* ie.          xxxxxxxxxxx 0xx0 00xx 0x0x x 0x0x 00xx
                                  ^=pos          ^ = ofst        */
 
      for(i=pos; i<tap.len-4 ; i++)
      {
         if(tap.tmem[i]==0)   /* skip over v1 pauses */
            i+=3;
         else
         {
            if(i==ofst)
               return 0;  /* ofst is not in a pause */
            if(i>ofst)
               return 1;  /* ofst is in a pause */
         }
      }
   }
   return 0;
}
/*---------------------------------------------------------------------------
 Returns the quantity of 'has checksum and its OK' files in the database.
*/
int count_good_checksums(void)
{
   int i,c;

   for(i=0,c=0; blk[i]->lt!=0; i++)
   {
      if(blk[i]->cs_exp!=-2)
      {
         if(blk[i]->cs_exp == blk[i]->cs_act)
            c++;
      }
   }
   return c;
}
/*---------------------------------------------------------------------------
 Make a copy of the loader table ft[] so we can revert back to it after any changes.
 the copy is globally available in ft_org[]
*/
void copy_loader_table(void)
{
   int i;
   for(i=0; ft[i].sp!=666; i++)
   {
      strcpy(ft_org[i].name, ft[i].name);
      ft_org[i].en     = ft[i].en;
      ft_org[i].tp     = ft[i].tp;
      ft_org[i].sp     = ft[i].sp;
      ft_org[i].mp     = ft[i].mp;
      ft_org[i].lp     = ft[i].lp;
      ft_org[i].pv     = ft[i].pv;
      ft_org[i].sv     = ft[i].sv;
      ft_org[i].pmin   = ft[i].pmin;
      ft_org[i].pmax   = ft[i].pmax;
      ft_org[i].has_cs = ft[i].has_cs;
   }
}
/*---------------------------------------------------------------------------
 Reset loader table to all original values, ONLY call this if a call to
 copy_loader_table() has been made!.
*/
void reset_loader_table(void)
{
   int i;
   for(i=0; ft[i].sp!=666; i++)
   {
      strcpy(ft[i].name, ft_org[i].name);
      ft[i].en     = ft_org[i].en;
      ft[i].tp     = ft_org[i].tp;
      ft[i].sp     = ft_org[i].sp;
      ft[i].mp     = ft_org[i].mp;
      ft[i].lp     = ft_org[i].lp;
      ft[i].pv     = ft_org[i].pv;
      ft[i].sv     = ft_org[i].sv;
      ft[i].pmin   = ft_org[i].pmin;
      ft[i].pmax   = ft_org[i].pmax;
      ft[i].has_cs = ft_org[i].has_cs;
   }
}
/*-----------------------------------------------------------------------------
 Search blk[] array for instance number 'num' of block type 'lt' and calls
 'xxx_describe_block' for that file (which decodes it).
 this is for use by scanners which need to get access to data held in other files
 ahead of describe() time.
 returns the block number in blk[] of the matching (and now decoded) file.
 on failure returns -1;

 NOTE : currently only implemented for certain file types.
*/
int find_decode_block(int lt, int num)
{
   int i,j;

   for(i=0,j=0; i<BLKMAX; i++)
   {
      if(blk[i]->lt==lt)
      {
         j++;
         if(j==num)  /* right filetype and right instance number? */
         {
            if(lt==CBM_DATA || lt==CBM_HEAD)
            {
               cbm_describe(i);
               return i;
            }
            if(lt==CYBER_F1)
            {
               cyberload_f1_describe(i);
               return i;
            }
            if(lt==CYBER_F2)
            {
               cyberload_f2_describe(i);
               return i;
            }
         }
      }
   }
   return -1;
}
/*---------------------------------------------------------------------------
 Show loader table values... (unused in console version)
*/
void show_loader_table(void)
{
   int i;
   char inf[100];
   
   msgout("\nThe following table is generated from internal data that FT uses for");
   msgout("\nscanning and cleaning, some values may change with program use.");
   msgout("\n");
   msgout("\n");

   sprintf(lin,"\nFORMAT NAME            EN   TP  SP  MP  LP  PV  SV  CS");
   msgout(lin);
   msgout("\n");
   for(i=3; ft[i].sp!=666; i++)
   {
      strcpy(lin,"");
      strcpy(inf,"");
      sprintf(inf,"%-22s ", ft[i].name);
      strcat(lin,inf);
      
      if(ft[i].en==LSbF)    sprintf(inf,"LSbF ");
      if(ft[i].en==MSbF)    sprintf(inf,"MSbF ");
      if(ft[i].en==XX)      sprintf(inf,"XX   ");
      strcat(lin,inf);

      if(ft[i].tp!=XX)    sprintf(inf,"0x%02X ",ft[i].tp);
      else                sprintf(inf,"XX  ");
      strcat(lin,inf);
      if(ft[i].sp!=XX)    sprintf(inf,"0x%02X ",ft[i].sp);
      else                sprintf(inf,"XX  ");
      strcat(lin,inf);
      if(ft[i].mp!=XX)    sprintf(inf,"0x%02X ",ft[i].mp);
      else                sprintf(inf,"XX  ");
      strcat(lin,inf);
      if(ft[i].lp!=XX)    sprintf(inf,"0x%02X ",ft[i].lp);
      else                sprintf(inf,"XX  ");
      strcat(lin,inf);
      if(ft[i].pv!=XX)    sprintf(inf,"0x%02X ",ft[i].pv);
      else                sprintf(inf,"XX  ");
      strcat(lin,inf);
      if(ft[i].sv!=XX)    sprintf(inf,"0x%02X ",ft[i].sv);
      else                sprintf(inf,"XX  ");
      strcat(lin,inf);

      if(ft[i].has_cs==CSYES) sprintf(inf,"Y   ");
      else                    sprintf(inf,"N   ");
      strcat(lin,inf);
      msgout(lin);
   }
   msgout("\n");
   msgout("\nEN = Byte Endianess");
   msgout("\nTP = Threshold Pulse");
   msgout("\nSP = Short Pulse");
   msgout("\nMP = Medium Pulse");
   msgout("\nLP = Long Pulse");
   msgout("\nPV = File Pilot Value");
   msgout("\nSV = File Sync Value");
   msgout("\nCS = File Checksum Available (Y/N)");
   msgout("\n");
   msgout("\nNote: XX = Variable/Not Applicable/Don't Care.");
   msgout("\nNote: MSbF = Most Significant bit First, LSbF = Least Significant bit First.");
   msgout("\nNote: Y = Yes, N = No.");
   msgout("\nNote: If PV and SV are both 0 or 1 the pilot/sync is a bitstream.");
   msgout("\n");
   msgout("\n");
}
/*---------------------------------------------------------------------------
 Counts pauses within the tap file, note: v1 pauses are still held
 as separate entities in the database even when they run consecutively.
 This function counts consecutive v1's as a single pause.
*/
int count_pauses(void)
{
   int i,n;

   for(n=0,i=0; blk[i]->lt!=0; i++)
   {
      if(blk[i]->lt==PAUSE)
      {
          n++;
          if(tap.version==1)  /* consecutive v1 pauses count as 1 pause.. */
          {
             while(blk[i++]->lt==PAUSE && i<BLKMAX-1)
             ;
             i--;
          }
      }
   }
   return n;
}
/*---------------------------------------------------------------------------
 Add an entry to the 'read_errors[100]' array...
*/
int add_read_error(int addr)
{
   int i;

   if(!note_errors)
      return -1;

   for(i=0; i<100; i++)  /* reject duplicates.. */
   {
      if(read_errors[i]==addr)
         return -1;
   }

   for(i=0; read_errors[i]!=0 && i<100; i++)  /* find 1st free slot.. */
   ;

   if(i<100)
   {
      read_errors[i]= addr;
      return 0;
   }
   return -1;  /* -1 = error table is full */
}
/*---------------------------------------------------------------------------
 Print the human readable TAP report to a buffer.
 Note: this is done so I can send the info to both the screen and the report
 without repeating the code.
 Note: Call 'analyze()' before calling this!.
*/
void print_results(char *buf)
{
   char tstr[2][256] = {"PASS","FAIL"};
   char tstr2[2][256] = {"OK","FAIL"};

   
   sprintf(buf,"\nFINAL TAP Test Report\n",tol-1);   /* note: only first line prints to buf */
   sprintf(lin,"\nPath= %s",tap.path);
   strcat(buf,lin);
   sprintf(lin,"\nRead Tolerance= %d",tol-1);
   strcat(buf,lin);

   sprintf(lin,"\n\n\nGENERAL INFO AND TEST RESULTS\n");
   strcat(buf,lin);
   sprintf(lin,"\nTAP Name    : %s",tap.name);
   strcat(buf,lin);
   sprintf(lin,"\nTAP Size    : %d bytes (%d kB)",tap.len, tap.len>>10);
   strcat(buf,lin);
   sprintf(lin,"\nTAP Version : %d",tap.version);
   strcat(buf,lin);
   sprintf(lin,"\nRecognized  : %d%s",tap.detected_percent,"%%");
   strcat(buf,lin);
   sprintf(lin,"\nData Files  : %d",tap.total_data_files);
   strcat(buf,lin);
   sprintf(lin,"\nPauses      : %d",count_pauses());
   strcat(buf,lin);
   sprintf(lin,"\nGaps        : %d",tap.total_gaps);
   strcat(buf,lin);
   sprintf(lin,"\nMagic CRC32 : %08X", tap.crc);
   strcat(buf,lin);

   if(tap.bootable)
   {
      if(tap.bootable==1)
         sprintf(lin,"\nBootable    : YES (1 part, name: %s)", pet2text(tmp, tap.cbmname));
      else
         sprintf(lin,"\nBootable    : YES (%d parts, 1st name: %s)", tap.bootable, pet2text(tmp, tap.cbmname));
      strcat(buf,lin);
   }
   else
   {
      sprintf(lin,"\nBootable    : NO");
      strcat(buf,lin);
   }
   sprintf(lin,"\nLoader ID   : %s",knam[tap.cbmid]);
   strcat(buf,lin);

   /* TEST RESULTS... */

   sprintf(lin,"\n");
   strcat(buf,lin);
   if(tap.tst_hd==0 && tap.tst_rc==0 && tap.tst_cs==0 && tap.tst_rd==0 && tap.tst_op==0)
      sprintf(lin,"\nOverall Result    : PASS");
   else
      sprintf(lin,"\nOverall Result    : FAIL");
   strcat(buf,lin);

   sprintf(lin,"\n");
   strcat(buf,lin);
   sprintf(lin,"\nHeader test       : %s [Sig: %s] [Ver: %s] [Siz: %s]"        ,tstr[tap.tst_hd], tstr2[tap.fsigcheck], tstr2[tap.fvercheck], tstr2[tap.fsizcheck]);
   strcat(buf,lin);
   sprintf(lin,"\nRecognition test  : %s [%d of %d bytes accounted for] [%d%s]",tstr[tap.tst_rc], tap.detected, tap.len-20, tap.detected_percent,"%%");
   strcat(buf,lin); 
   sprintf(lin,"\nChecksum test     : %s [%d of %d checksummed files OK]"      ,tstr[tap.tst_cs], tap.total_checksums_good, tap.total_checksums);
   strcat(buf,lin);
   sprintf(lin,"\nRead test         : %s [%d Errors]"                          ,tstr[tap.tst_rd], tap.total_read_errors);
   strcat(buf,lin);
   sprintf(lin,"\nOptimization test : %s [%d of %d files OK]"                  ,tstr[tap.tst_op], tap.optimized_files, tap.total_data_files);
   strcat(buf,lin);
   sprintf(lin,"\n");
   strcat(buf,lin);
}
/*---------------------------------------------------------------------------
 Print out fully interpreted file info for every file in the database to buffer 'buf'.
 Note: re-calls xxxxx_describe functions for each format in order to
 capture the text output from those functions and include it in the report.
*/
void print_database(char *buf)
{
   int i,t;

   sprintf(buf,"\nFILE DATABASE\n");

   for(i=0; blk[i]->lt!=0; i++)
   {
      sprintf(lin,"\n---------------------------------");
      strcat(buf,lin);
      sprintf(lin,"\nFile Type: %s", ft[blk[i]->lt].name);
      strcat(buf,lin);
      sprintf(lin,"\nLocation: $%04X -> $%04X -> $%04X -> $%04X", blk[i]->p1, blk[i]->p2, blk[i]->p3, blk[i]->p4);
      strcat(buf,lin);

      if(blk[i]->lt > PAUSE)  /* info for data files only... */
      {
         sprintf(lin,"\nLA: $%04X  EA: $%04X  SZ: %d", blk[i]->cs, blk[i]->ce, blk[i]->cx);
         strcat(buf,lin);

         if(blk[i]->fn != NULL)  /* filename, if applicable */
         {
            sprintf(lin,"\nFile Name: %s", blk[i]->fn);
            strcat(buf,lin);
         }

         sprintf(lin,"\nPilot/Trailer Size: %d/%d", blk[i]->pilot_len, blk[i]->trail_len);
         strcat(buf,lin);

         if(ft[blk[i]->lt].has_cs == TRUE)  /* checkbytes, if applicable */
         {
            sprintf(lin,"\nCheckbyte Actual/Expected: $%02X/$%02X", blk[i]->cs_act, blk[i]->cs_exp);
            strcat(buf,lin);
         }

         sprintf(lin,"\nRead Errors: %d", blk[i]->rd_err);
         strcat(buf,lin);
         sprintf(lin,"\nUnoptimized Pulses: %d", count_unopt_pulses(i));
         strcat(buf,lin);
         sprintf(lin,"\nCRC32: %08X", blk[i]->crc);
         strcat(buf,lin);
      }
     
      describe_file(i);
      strcat(buf,info);   /* add additional text */
      strcat(buf,"\n");
   }
}
/*---------------------------------------------------------------------------
 Print pulse stats to buffer 'buf'.
 Note: Call 'analyze()' before calling this!.
*/
void print_pulse_stats(char *buf)
{
   int i;

   sprintf(buf,"\nPULSE FREQUENCY TABLE\n");

   for(i=1; i<256; i++)
   {
      if(tap.pst[i]!=0)
      {
         sprintf(lin,"\n0x%02X (%d)", i, tap.pst[i]);
         strcat(buf,lin);
      }
   }
}
/*---------------------------------------------------------------------------
 Print file stats to buffer 'buf'.
 Note: Call 'analyze()' before calling this!.
*/
void print_file_stats(char *buf)
{
   int i;

   sprintf(buf,"\nFILE FREQUENCY TABLE\n");
 
   for(i=0; ft[i].sp!=666; i++)  /* for each file format in ft[]...  */
   {
      if(tap.fst[i]!=0)  /* list all found files and their frequency...  */
      {
         sprintf(lin,"\n%s (%d)", ft[i].name, tap.fst[i]);
         strcat(buf,lin);
      }
   }
}
/*---------------------------------------------------------------------------
 Create a table of exportable PRGs in the prg[] array based on the current
 data extractions available in the blk[] array.
 Note: if 'prgunite' is not 0, then any neigbouring files will be
 connected as a single PRG. (neighbour = data addresses run consecutively).
*/
void make_prgs(void)
{
   int i,c,j,t,x,s,e,errors,ti,pt[BLKMAX];
   unsigned char *tmp, done;

   /* create table of all exported files by index (of blk)... */
   for(i=0,j=0; blk[i]->lt!=0; i++)
   {
      if(blk[i]->dd!=NULL)
         pt[j++]=i;
   }
   pt[j]=-1;  /* terminator */
      

   for(i=0; i<BLKMAX ;i++)    /* clear the prg table... */
   {
      prg[i].lt= 0;
      prg[i].cs= 0;
      prg[i].ce= 0;
      prg[i].cx= 0;
      if(prg[i].dd!=NULL)
      {
         free(prg[i].dd);
         prg[i].dd= NULL;
      }
      prg[i].errors= 0;
   }
      
   
   tmp= (unsigned char*)malloc(65536*2);  /* create buffer for unifications */
   j=0;   /* j steps through the finished prg's */

   /* scan through the 'data holding' blk[] indexes held in pt[]... */
   for(i=0; pt[i]!=-1 ;i++)
   {
      if(blk[pt[i]]->dd!=NULL)  /* should always be true. */
      {
         ti= 0;
         done= 0;

         s= blk[pt[i]]->cs;   /* keep 1st start address. */
         errors= 0;           /* this will count the errors found in each blk
                                entry used to create the final data prg in tmp. */
         do
         {
            t= blk[pt[i]]->lt;   /* get details of next exportable part... */
                                 /* note: where files are united, the type will be set to the type of only the last file */
            x= blk[pt[i]]->cx;
            e= blk[pt[i]]->ce;
            errors+= blk[pt[i]]->rd_err;
            
            if(ft[blk[pt[i]]->lt].has_cs == TRUE && blk[pt[i]]->cs_exp != blk[pt[i]]->cs_act) /* bad checksum also counts as an error */
            {
               errors++;
            } 
                        
            for(c=0; c<x; c++)
               tmp[ti++]= blk[pt[i]]->dd[c]; /* copy the data to tmp buffer */



            /* block unification wanted?... */
            if(prgunite)
            {
               /* scan following blocks and override default details. */
               if(pt[i+1]!=-1)   /* another file available? */
               {
                  if(blk[pt[i+1]]->cs == (e+1)) /* is next file a neighbour? */
                     i++;
                  else
                     done =1;
               }
               else
                  done =1;
            }
            else
               done =1;
         }
         while(!done);

         /* create the finished prg entry using data in tmp...  */
         
         x= ti;                                     /* set final data length */
         prg[j].dd= (unsigned char*)malloc(x);      /* allocate the ram */
         for(c=0; c<x; c++)                         /* copy the data.. */
            prg[j].dd[c]= tmp[c];
         prg[j].lt= t;                              /* set file type */ 
         prg[j].cs= s;                              /* set file start address */ 
         prg[j].ce= e;                              /* set file end address */ 
         prg[j].cx= x;                              /* set file length */ 
         prg[j].errors= errors;                     /* set file errors */ 
         j++;                                       /* onto the next prg... */
      }
   }
   prg[j].lt= 0;   /* terminator */

   free(tmp);
}
/*---------------------------------------------------------------------------
 Save all available prg's to a folder (console app only).
 Returns the number of files saved.
*/
int save_prgs(void)
{
   int i;
   FILE *fp;    

   chdir(exedir);

   if(chdir("prg")==0)        /* delete old prg's... */
      system("del *.prg");
   else                       /* create prg folder if needed... */
   {
      system("mkdir prg");
      chdir("prg");
   }

   for(i=0; prg[i].lt!=0; i++)
   {
      sprintf(lin,"%03d (%04X-%04X)",i+1, prg[i].cs, prg[i].ce);
      if(prg[i].errors!=0)   /* append error indicator if necessary) */
         strcat(lin," BAD");
      strcat(lin,".prg");

      fp= fopen(lin,"w+b");
      if(fp!=NULL)
      {
         fputc(prg[i].cs & 0xFF, fp);          /* write low byte of start address */
         fputc((prg[i].cs & 0xFF00)>>8, fp);   /* write high byte of start address */
         fwrite(prg[i].dd, prg[i].cx, 1, fp);  /* write data */
         fclose(fp);
      }
   }

   chdir(exedir);
   return 0;
}

/*---------------------------------------------------------------------------
 Displays a message.
 I made this to quickly convert the method of text output from the windows
 sources. (ie. popup message windows etc).
*/
void msgout(char *str)
{
   printf("%s",str);
}
/*---------------------------------------------------------------------------
 Search integer array 'buf' for occurrence of sequence 'seq'.
 On success return offset of matching sequence.
 On failure return -1.
 Note : value XX (0xFF) may be used in 'seq' as a wildcard.
*/
int find_seq(unsigned char *buf,int bufsz, unsigned char *seq,int seqsz)
{
   int i,j, match;

   if(seqsz> bufsz)  /* sequence size must be less than or equal to buffer size! */
      return -1;

   for(i=0; i< bufsz-seqsz; i++)
   {
      if(buf[i]==seq[0] || seq[0]==XX)   /* match first number. */
      {
         match=0;
         for(j=0; j< seqsz && (i+j)< bufsz; j++)
         {
            if(buf[i+j]==seq[j] || seq[j]==XX)
               match++;
         }
         if(match==seqsz)  /* whole sequence found?  */
            return i;
      }
   }
   return -1;
}
/*---------------------------------------------------------------------------
 Isolate the filename part of a full path+filename and store it in buffer *dest.
*/
void getfilename(char *dest, char *fullpath)
{
   int i,j,k;

   i= strlen(fullpath);
   for(j=i; j>0 && fullpath[j]!='\\'; j--)
   ;   /* rewind j to 0 or first slash.. */

   if(fullpath[j]=='\\')
      j++;                /* skip over the slash and copy the file name... */
   for(k=0 ;j<i; j++)
      dest[k++]= fullpath[j];
   dest[k]= 0;
   return;
}
/*---------------------------------------------------------------------------
 Convert PetASCII string to ASCII text string.
 user provides destination storage string 'dest'.
 Returns a pointer to dest so the function may be called inline.
*/
char* pet2text(char *dest, char *src)
{
   int i,lwr;
   char ts[500];
   unsigned char ch;

   lwr=0;  /* lowercase off. */

   /* process file name... */
   strcpy(dest,"");
   for(i=0; src[i]!=0; i++)
   {
      ch= (unsigned char)src[i];

      /* process CHR$ 'SAME AS' codes... */
      if(ch==255)
         ch=126;
      if(ch>223 && ch<255)   /* produces 160-190 */
         ch-=64;
      if(ch>191 && ch<224)    /* produces 96-127 */
         ch-=96;

      if(ch==14)   /* switch to lowercase.. */
         lwr=1;
      if(ch==142)  /* switch to uppercase.. */
         lwr=0;

      if(ch>31 && ch<128)   /* print printable character... */
      {
         if(lwr)  /* lowercase?, do some conversion... */
         {
            if(ch>64 && ch<91)
               ch+=32;
            else if(ch>96 && ch<123)     
               ch-=32;
         }
         sprintf(ts,"%c",ch);
         strcat(dest,ts);
      }
   }
   return dest;
}
/*---------------------------------------------------------------------------
 Trims trailing spaces from a string.
*/
void trimstring(char *str)
{
   int i,len;
   
   len= strlen(str);
   if(len>0)
   {
      for(i=len-1; str[i]==32 && i>0; i--)  /* nullify trailing spaces..  */
         str[i]=0;
   }
}
/*---------------------------------------------------------------------------
 Pads the string 'str' with trailing spaces so the resulting string is 'wid' chars long. 
*/
void padstring(char *str, int wid)
{
   int i,len;

   len= strlen(str);
   if(len<wid)
   {
      for(i=len; i<wid; i++)
         str[i]=32;
      str[i]=0;
   }
}
/*---------------------------------------------------------------------------
 Converts an integer number of seconds to a time string of format HH:MM:SS.
*/
void time2str(int secs, char *buf)
{
   int h,m,s;

   h= secs/3600;
   m= (secs-(h*3600)) /60;
   s= secs -(h*3600) -(m*60);
   sprintf(buf,"%02d:%02d:%02d",h,m,s);
}
/*---------------------------------------------------------------------------
 Remove all/any existing work files. 
*/
void deleteworkfiles(void)
{
   FILE *fp;
      
   /* delete existing work files...
      note: the fopen tests avoid getting any console error output. */   
 
   fp= fopen(tempftreportname,"r");    
   if(fp!=NULL)
   {
      fclose(fp);
      sprintf(lin,"del %s",tempftreportname);
      system(lin);
   }
   fp= fopen(ftreportname,"r");     
   if(fp!=NULL)
   {
      fclose(fp);
      sprintf(lin,"del %s",ftreportname);
      system(lin);
   }
   fp= fopen(tempftbatchreportname,"r");      
   if(fp!=NULL)
   {
      fclose(fp);
      sprintf(lin,"del %s",tempftbatchreportname);
      system(lin);
   }
   fp= fopen(ftbatchreportname,"r");    
   if(fp!=NULL)
   {
      fclose(fp);
      sprintf(lin,"del %s",ftbatchreportname);
      system(lin);
   }
   fp= fopen(ftinfoname,"r");   
   if(fp!=NULL)
   {
      fclose(fp);
      sprintf(lin,"del %s",ftinfoname);
      system(lin);
   }  
}  















