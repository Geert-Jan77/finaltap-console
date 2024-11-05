/*---------------------------------------------------------------------------
  loader_id.c 

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
   

  An effort to speed up the scanning process for TAPs, if I can ID the loader type after
  the C64 ROM scan is done then I needn't search for ALL formats.

  Notes:- 

  This does not work out well on taps containing multiple games!, the program 
  switch 'noid' is provided for such cases and will force ft to scan all formats.

---------------------------------------------------------------------------*/

#include "main.h"
#include "mydefs.h"

/*----------------------------------------------------------------------------------------------------
 Look through the crc database for a crc matching the paramater, returns the defined value
 of the particular loader family if a a match is found, else return 0.
 The crc passed should obviously be that of the first CBM program in the TAP.
*/
int idloader(unsigned long crc)
{
   int i,t,id;

   unsigned long kcrc[1000][2]=
   {
   {0x261DBA0E,LID_FREE},
   {0x8AAE883E,LID_FREE},
   {0x2EF78649,LID_FREE},
   {0x03969E4B,LID_FREE},
   {0xFCE154D8,LID_FREE},
   {0xF9D0BE97,LID_FREE},
   {0x5A1D548D,LID_FREE},
   {0xCF2A7534,LID_FREE},
   {0xA19943D0,LID_FREE},
   {0x95094A3A,LID_FREE},
   {0xF6EA5D74,LID_FREE},
   {0x6C280859,LID_FREE},
   {0xBBDC123F,LID_FREE},
   {0xB3EDBE12,LID_FREE},
   {0x15C61C29,LID_FREE},
   {0xDE2C7D61,LID_FREE},
   {0x4D7D872D,LID_FREE},
   {0x2CC1E59C,LID_FREE},
   {0x89D45404,LID_FREE},

   {0xAD318CC4,LID_BLEEP},
   {0x230936D6,LID_BLEEP},
   {0x39B7A588,LID_BLEEP},
   {0x2277F4DD,LID_BLEEP},
   {0x4CF069E0,LID_BLEEP},
   {0x3B4A219E,LID_BLEEP},
   {0xAFD39E15,LID_BLEEP},
   {0x9B898EF1,LID_BLEEP},
   {0xAEAD5C1E,LID_BLEEP},
   {0x8B5FA78A,LID_BLEEP},
   {0x09A49BB4,LID_BLEEP},
   {0x58583B59,LID_BLEEP},
   {0xF33ED7A2,LID_BLEEP},
   {0x4825AB54,LID_BLEEP},
   {0x5DD93BE5,LID_BLEEP},
   {0xEB752E5F,LID_BLEEP},
   {0x1EB8DA0A,LID_BLEEP},
   {0x2BF72881,LID_BLEEP},
   {0x59142D65,LID_BLEEP},
   {0x723AD943,LID_BLEEP},
   {0xE24E270C,LID_BLEEP},
   {0xCD8D92EE,LID_BLEEP},

   {0x366784C8,LID_CHR},  

   {0xD09BF46F,LID_BURN},   
   {0x8D6E30E7,LID_BURN},
   {0x88613263,LID_BURN},
   {0x291E606A,LID_BURN},
   {0x6EA1C3AD,LID_BURN},

   {0xC618D67A,LID_WILD},   
   {0x657CC9CD,LID_WILD},
   {0x554FAB46,LID_WILD},
   {0x8D14D322,LID_WILD},
   {0x7EBF1CC9,LID_WILD},
   {0xCDEB4E81,LID_WILD},
   {0x6B76D7C7,LID_WILD},
   {0xDC5FABC2,LID_WILD},
   {0xC949C625,LID_WILD},
   {0x97AB5438,LID_WILD},
   {0xFC8E7F96,LID_WILD},
   {0x69D7FCC6,LID_WILD},

   {0x00A517B5,LID_USG},
   {0x518F7B24,LID_USG},
   {0x3F6DD277,LID_USG},
   {0xD73E85F7,LID_USG},
   {0x548CC3DA,LID_USG},
   {0x834D852F,LID_USG},
   {0x6C791C31,LID_USG},
   {0x1BEBD222,LID_USG},
   {0x9DCF6B97,LID_USG},
   {0x81C86C4A,LID_USG},
   {0x7C1A7B45,LID_USG},
   {0x917CCAF0,LID_USG},
   {0x5FBA5BA7,LID_USG},
   {0x4C2A7C85,LID_USG},
   {0x70900492,LID_USG},
   {0x7BF88049,LID_USG},
   {0xDE86E455,LID_USG},
   {0xC6A33E82,LID_USG},
   {0xB50AB01A,LID_USG},
   {0x223890AF,LID_USG},
   {0xEA89FD1F,LID_USG},
   {0xB3AE738A,LID_USG},
   {0xE5869C31,LID_USG},
   {0xCBCCDB4E,LID_USG},
   {0xD0528E47,LID_USG},
   {0x25035DF8,LID_USG},
   {0x4FA62BEC,LID_USG},
   {0x266B6BD6,LID_USG},
   {0x17060B09,LID_USG},

   {0x0645F350,LID_MIC},
   {0x78C43CB9,LID_MIC},
   {0xE0035766,LID_MIC},
   {0x0EA016C6,LID_MIC},
   {0x92571C3D,LID_MIC},
   {0x96677D19,LID_MIC},
   {0x9F20C2DB,LID_MIC},
   {0xBC7F0E8D,LID_MIC},
   {0xB1F027B4,LID_MIC},
   {0x178977C3,LID_MIC},
   {0x29DCB0D1,LID_MIC},

   {0x58EEE22A,LID_ACE},
   {0x915280DA,LID_ACE},
   {0x6A333F1D,LID_ACE},
   {0x292409A7,LID_ACE},
   {0xF4CF55C8,LID_ACE},

   {0x60BCE3A3,LID_T250},
   {0x2D7372C2,LID_T250},
   {0xE3033FA9,LID_T250},
   {0x50C1FAFE,LID_T250},
   {0x1FDF834A,LID_T250},
   {0x8E399D97,LID_T250},
   {0x7D11900C,LID_T250},
   {0xC5C8015F,LID_T250},
   {0xB3A98C46,LID_T250},

   {0x67D4643C,LID_RACK},
   {0xAB3A1BAC,LID_RACK},
   {0x95518E9E,LID_RACK},
   {0x8E4CCA04,LID_RACK},

   {0x96285AA9,LID_OCEAN},
   {0x7E818F78,LID_OCEAN},
   {0x20CDF565,LID_OCEAN},
   {0xD2908C53,LID_OCEAN},
   {0xD3958D73,LID_OCEAN},
   {0x06EC1039,LID_OCEAN},
   {0xD70F4CBA,LID_OCEAN},
   {0xA9F2CE53,LID_OCEAN},
   {0x1C237E84,LID_OCEAN},
   {0xC1C4A2A0,LID_OCEAN},
   {0x985B5C4D,LID_OCEAN},    
   {0xBD0ECB9E,LID_OCEAN},

   {0x04F2443B,LID_RAST},
   {0x880CA8A2,LID_RAST},

   {0xBDF9B4EF,LID_SPAV},
   {0x66CEE4E9,LID_SPAV},
   {0x603E8D53,LID_SPAV},
   {0x0412A711,LID_SPAV},
   {0x7CA20791,LID_SPAV},

   {0x5DC3AA69,LID_HIT},
   {0x3D9AD474,LID_HIT},

   {0x1482D2A7,LID_ANI},
   {0x0363E489,LID_ANI},
   {0x24BE37F6,LID_ANI},
   {0xA09E55E9,LID_ANI},

   {0xDD3DE175,LID_VIS1},
   {0x6B2A1236,LID_VIS1},    /* atom ant. */
   {0x9EF77DD5,LID_VIS2},
   {0x867C8969,LID_VIS3},
   {0xC7641A7E,LID_VIS4},

   {0xA7D33777,LID_FIRE},
   {0x041FDA59,LID_FIRE},

   {0x001B30E8,LID_NOVA},
   {0xA5950CFF,LID_NOVA},

   {0x25197B4E,LID_IK},

   {0x5C34F43D,LID_PAV},

   {0x0936B7DF,LID_VIRG},
   {0x075AE096,LID_VIRG},    /* Fist 2 (Mastertronic) */
   {0x342A2416,LID_VIRG},    /* same as "Hi-Tec" loader but thres=$016E   Future Bike */
   {0x895DCF44,LID_VIRG},    /* This is "Virgin Loader"  (thres=$015E)   Guardian II,Alcazar,Toy Bizarre, Hard Drivin */

   {0xFADDF41C,LID_HTEC},    /* This is "Hi-Tec" Loader.  (thres=$017E)  Chevy Chase, Top Cat, Yogi */

   {0x8E027BD2,LID_FLASH},
   {0x1754E006,LID_OCNEW1T1},
   {0x3A35F804,LID_OCNEW1T1},    /* Ocean New 1 T1 (adidas soccer) */
   {0xC039C251,LID_OCNEW1T2},
   {0x7FFB98B2,LID_OCNEW2},      /* Ocean New 2   (shadow warriors) */
   {0x9B132BD0,LID_OCNEW2},      /* Ocean New 2   (klax) */
   {0x5622E174,LID_ATLAN},       /* Atlantis loader. */
   {0x206A8B68,LID_AUDIOGENIC},  /* Audiogenic loader */
   {0,0}
   };

   unsigned char idstrings[5][10]=
   {
   {0x0E,0x0F,0x16,0x01,0},                  /* "NOVA" (screen, novaload)     */
   {0x4E,0x4F,0x56,0x41,0},                  /* "NOVA" (ascii, novaload)      */
   {0x03,0x19,0x02,0x05,0x12,0},             /* "CYBER" (screen, cyberload)   */
   {0x47,0x4F,0x20,0x41,0x57,0x41,0x59,0},   /* "GO AWAY" (ascii, ocean)      */
   {0x53,0x4E,0x41,0x4B,0x45,0}              /* "SNAKE" (ascii, snakeload)    */
   };
  

   /* search crc table for alias... */
   id= 0;
   for(i=0; kcrc[i][0]!=0; i++)
   {
      if(crc==kcrc[i][0])
         id= kcrc[i][1];
   }

   /* crc search failed?... search 1st CBM data file for identifying strings... */
   if(id==0)
   {
      t= find_decode_block(CBM_DATA,1);
      if(t!=-1)
      {
         if(find_seq(blk[t]->dd,blk[t]->cx, idstrings[0],strlen(idstrings[0]))!=-1)     
            id= LID_NOVA;
         if(find_seq(blk[t]->dd,blk[t]->cx, idstrings[1],strlen(idstrings[1]))!=-1)     
            id= LID_NOVA;
         if(find_seq(blk[t]->dd,blk[t]->cx, idstrings[2],strlen(idstrings[2]))!=-1)     
            id= LID_CYBER;
         if(find_seq(blk[t]->dd,blk[t]->cx, idstrings[3],strlen(idstrings[3]))!=-1)     
            id= LID_OCEAN;
         if(find_seq(blk[t]->dd,blk[t]->cx, idstrings[4],strlen(idstrings[4]))!=-1)     
            id= LID_SNAKE;
      }
   }
   return id;
}

