/*---------------------------------------------------------------------------
  mydefs.h

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

---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <conio.h>
#include <ctype.h>
#include <io.h>
#include <malloc.h>
#include <time.h>

#include <dir.h>    /* borland needs this */

#include "scanners\_scanners.h"

#define FTVERSION 2.77

#define FTDAY 17          /* day, month and year of release... */
#define FTMONTH 5
#define FTYEAR 2006

#define TRUE 1
#define FALSE 0

#define FAIL 0xFFFFFFFF
#define DEFTOL 11       /* default bit reading tolerance. (1 = zero tolerance)   */

#define LAME 0x0F       /* cutoff value for 'noise' pulses when rebuilding pauses.*/

#define BLKMAX  2000    /* Maximum number of blocks allowed in database */
#define DBERR   -1      /* return value from "addblockdef" when database entry failed. */
#define DBFULL  -2      /* return value from "addblockdef" when database is full. */
#define CPS     985248  /* 6510 cycles per second (PAL) */

#define NA -1           /* indicator : Not Applicable. */
#define VV -1           /* indicator : A variable value is used. */
#define XX 0xFF         /* indicator: Dont care. */

#define LSbF 0          /* indicator : Least Significant bit First. */
#define MSbF 1          /* indicator : Most Significant bit First. */
#define CSYES 1         /* indicator : A checksum is used. */
#define CSNO 0          /* indicator : A checksum is not used. */



/* each of these constants indexes an entry in the "ft[]" fmt_t array... */
enum{GAP=1,PAUSE,CBM_HEAD,CBM_DATA,TT_HEAD,TT_DATA,FREE,USGOLD,ACES,WILD,WILD_STOP,NOVA,
     NOVA_SPC,OCEAN_F1,OCEAN_F2,OCEAN_F3,CHR_T1,CHR_T2,CHR_T3,RASTER,CYBER_F1,CYBER_F2,
     CYBER_F3,CYBER_F4_1,CYBER_F4_2,CYBER_F4_3,
     BLEEP,BLEEP_TRIG,BLEEP_SPC,HITLOAD,MICROLOAD,BURNER,RACKIT,SPAV1_HD,SPAV1,
     SPAV2_HD,SPAV2,VIRGIN,HITEC,ANIROG,VISI_T1,VISI_T2,VISI_T3,VISI_T4,
     SUPERTAPE_HEAD,SUPERTAPE_DATA,PAV,IK,FBIRD1,FBIRD2,TURR_HEAD,TURR_DATA,
     SEUCK_L2,SEUCK_HEAD,SEUCK_DATA,SEUCK_TRIG,SEUCK_GAME,JET,FLASH,
     TDI_F1,OCNEW1T1,OCNEW1T2,OCNEW2,ATLAN,SNAKE51,SNAKE50T1,SNAKE50T2,
     PAL_F1,PAL_F2,ENIGMA,AUDIOGENIC};

/*
 These constants are the loader IDs, used for quick scanning via CRC lookup...
 see file "loader_id.c"
 see also string array 'knam' in the main file.
*/

enum{LID_FREE=1,LID_BLEEP,LID_CHR,LID_BURN,LID_WILD,LID_USG,LID_MIC,LID_ACE,LID_T250,
     LID_RACK,LID_OCEAN,LID_RAST,LID_SPAV,LID_HIT,LID_ANI,LID_VIS1,LID_VIS2,
     LID_VIS3,LID_VIS4,LID_FIRE,LID_NOVA,LID_IK,LID_PAV,LID_CYBER,LID_VIRG,LID_HTEC,
     LID_FLASH,LID_SUPER,LID_OCNEW1T1,LID_OCNEW1T2,LID_ATLAN,LID_SNAKE,
     LID_OCNEW2,LID_AUDIOGENIC};


/* struct 'tap_t' contains general info about the loaded tap file... */
struct tap_t
{
   char path[512];           /* file path + name. */
   char name[512];           /* file name. */
   unsigned char *tmem;      /* storage for the loaded tap. */
   int len;                  /* length of the loaded tap. */
   int pst[256];             /* pulse stats table. */
   int fst[256];             /* file stats table. */
   int fsigcheck;            /* header check results... (1=ok, 0=failed) */
   int fvercheck;
   int fsizcheck;
   int detected;              /* number of bytes accounted for. */
   int detected_percent;      /* ..and as a percentage.*/
   int purity;                /* number of pulse types in the tap. */
   int total_gaps;            /* number of gaps. */
   int total_data_files;      /* number of files found. (not inc pauses and gaps) */
   int total_checksums;       /* number of files found with checksums. */
   int total_checksums_good;  /* number of checksums verified. */
   int optimized_files;       /* number of fully optimized files. */
   int total_read_errors;     /* number of read errors. */
   int fdate;                 /* age of file. (date and time stamp). */
   float taptime;             /* playing time (seconds). */
   int version;               /* TAP version (currently 0 or 1). */
   int bootable;              /* holds the number of bootable ROM file sequences */
   int changed;               /* flags that the tap has been altered (+ needs rescan) */
   unsigned long crc;         /* overall (data extraction) crc32 for this tap */
   unsigned long cbmcrc;      /* crc32 of the 1st CBM program found */
   int cbmid;                 /* loader id, see enums in mydefs.h (-1 = N/A) */
   char cbmname[20];          /* filename for first CBM file (if exists). */
   int tst_hd;
   int tst_rc;
   int tst_op;                /* quality test results. 0=Pass, 1= Fail */
   int tst_cs;
   int tst_rd;
};
extern struct tap_t tap;


/* struct 'fmt_t' contains info about a particular tape format... */
struct fmt_t
{
   char name[32];          /* format name  */
   int en;                 /* byte endianess, 0=LSbF, 1=MSbF */
   int tp;                 /* threshold pulsewidth (if applicable) */
   int sp;                 /* ideal short pulsewidth  */
   int mp;                 /* ideal medium pulsewidth (if applicable)  */
   int lp;                 /* ideal long pulsewidth  */
   int pv;                 /* pilot value   */
   int sv;                 /* sync value  */
   int pmin;               /* minimum pilots that should be present. */
   int pmax;               /* maximum pilots that should be present. */
   int has_cs;             /* flag, provides checksums, 1=yes, 0=no. */
};
extern struct fmt_t ft[100];



/* struct 'blk_t' is the basic unit of the tap database, each entity found in
  a tap file will have one of these, see array 'blk[]'...  */
struct blk_t
{
   int lt;   /* loader type (see loadername enum of constants above) */
   int p1;   /* position of first pulse */
   int p2;   /* position of first data pulse */
   int p3;   /* position of last data pulse */
   int p4;   /* position of last pulse */
   int xi;   /* extra info (usage varies) */

   int cs;               /* c64 ram start pos */
   int ce;               /* c64 ram end pos */
   int cx;               /* c64 ram len */
   unsigned char *dd;    /* pointer to decoded data block */
   int crc;              /* crc32 of the decoded data file */
   int rd_err;           /* number of read errors in block */
   int cs_exp;           /* expected checksum value (if applicable) */
   int cs_act;           /* actual checksum value (if applicable) */
   int pilot_len;        /* length of pilot tone (in bytes or pulses) */
   int trail_len;        /* length of trail tone (in bytes or pulses) */
   char *fn;             /* pointer to file name (if applicable) */
   int ok;               /* file ok indicator, 1=ok. */ 
};
extern struct blk_t *blk[BLKMAX];


/* struct 'prg_t' contains an extracted data file and infos for it,
  used as array 'prg[]' */
struct prg_t
{
   int lt;             /* loader type. (required for block unification) */
   int cs;             /* c64 ram start pos */
   int ce;             /* c64 ram end pos */
   int cx;             /* c64 ram len */
   unsigned char *dd;  /* pointer to decoded data block */
   int errors;         /* number of read errors in the file */
};
extern struct prg_t prg[BLKMAX];

extern int aborted;         /* general 'operation aborted' flag */
extern int tol;             /* turbotape bit reading tolerance. */
extern char lin[64000];     /* general purpose string building buffer */
extern char info[1000000];  /* string building buffer, gathers output from scanners */

extern const char knam[100][32];

extern int quiet;

extern int visi_type;    /* default visiload type.  */

extern int cyber_f2_eor1;
extern int cyber_f2_eor2;

extern int batchmode;

extern char exedir[512];
extern char ftverstr[256];
extern char ftreportname[256];
extern char tempftreportname[256];
extern char ftbatchreportname[256];
extern char tempftbatchreportname[256];
extern char ftinfoname[256];
extern char tapoutname[256];
extern char auoutname[256];
extern char wavoutname[256];


/* program options... */

extern char debug;
extern char noid;
extern char noc64eof;
extern char docyberfault;
extern char boostclean;
extern char noaddpause;
extern char sine;
extern char prgunite;
extern char extvisipatch;
extern char incsubdirs;
extern char sortbycrc;

extern char exportcyberloaders;
extern char preserveloadertable;


