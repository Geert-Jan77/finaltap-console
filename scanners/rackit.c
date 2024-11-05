/*---------------------------------------------------------------------------
  rackit.c

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

#include "../mydefs.h"
#include "../main.h"

#define HDSZ 8

/*---------------------------------------------------------------------------
*/
void rackit_search(void)
{
   int i, sof,sod,eod,eof;
   int pi,zcnt;
   int lstart,lend,tmp;
   int lead,sync,tcnt, hd[HDSZ],rd_err;
   unsigned char pilotsync[10][2] ={{0xDE,0x14},{0x25,0x3D},{0x97,0x1B},{0,0}};

   if(!quiet)
      msgout("  Rack-It tape");
         

   pi=0; /* step thru each pilot/sync pair in table. */

   while(pilotsync[pi][0]!=0 && pilotsync[pi][1]!=0)
   {
      lead= pilotsync[pi][0];  /* get next known pilot/sync pair. */
      sync= pilotsync[pi][1];

      for(i=20; i<tap.len-8; i++)
      {
         if(readttbyte(i, ft[RACKIT].lp, ft[RACKIT].sp, ft[RACKIT].tp, ft[RACKIT].en)==lead)
         {
            sof = i;
            zcnt=0;
            while(readttbyte(i, ft[RACKIT].lp, ft[RACKIT].sp, ft[RACKIT].tp, ft[RACKIT].en)==lead && i<tap.len)
            {
               i+=8;
               zcnt++;
            }
            if(zcnt>100 && readttbyte(i, ft[RACKIT].lp, ft[RACKIT].sp, ft[RACKIT].tp, ft[RACKIT].en)==sync)
            {
               sod= i+8;

               /* decode the header, so we can validate the addresses... */
               rd_err=0;
               for(tcnt=0; tcnt<HDSZ; tcnt++)
               {
                  hd[tcnt] = readttbyte(sod+(tcnt*8), ft[RACKIT].lp, ft[RACKIT].sp, ft[RACKIT].tp, ft[RACKIT].en);

                  if(hd[tcnt]==-1)
                     rd_err++;     /* count any read errors in header */
               }

               if(rd_err==0)
               {
                  lstart = ((hd[5]<<8)+ hd[6]);   /* get start address */
                  lend =   ((hd[3]<<8)+ hd[4]);   /* get end address */
                  if(lend>lstart)
                  {
                     tmp = lend-lstart;    /* calculate offsets of end of block... */
                     eod = sod+(tmp*8)+(HDSZ*8)-8;
                     eof=eod+7;
                     addblockdef(RACKIT, sof,sod,eod,eof, 0);
                     i = eof;  /* optimize search */
                  }
               }
            }
         }
      }
      pi++; /* bump to next pilot/sync pair. */
   }
}

/*---------------------------------------------------------------------------
*/
int rackit_describe(int row)
{
   int i,s,hd[HDSZ],rd_err,b,cb;
   unsigned char xor;

   /* decode the header... */
   s= blk[row]->p2;
   for(i=0; i<HDSZ; i++)
      hd[i]= readttbyte(s+(i*8), ft[RACKIT].lp, ft[RACKIT].sp, ft[RACKIT].tp, ft[RACKIT].en);

   /* compute C64 start address, end address and size... */
   blk[row]->cs = (hd[5]<<8)+ hd[6];
   blk[row]->ce = (hd[3]<<8)+ hd[4]-1;
   blk[row]->cx = (blk[row]->ce - blk[row]->cs)+1;

   xor= hd[0];   /* read xor decode value */

   sprintf(lin,"\n - XOR Decode value: $%02X",xor);
   strcat(info,lin);

   /* get pilot & trailer lengths... */
   blk[row]->pilot_len= (blk[row]->p2- blk[row]->p1 -8) >>3;
   blk[row]->trail_len= (blk[row]->p4- blk[row]->p3 -7) >>3;

   /* extract data and test checksum... */
   rd_err=0;
   cb=0;
   s= (blk[row]->p2)+(HDSZ*8);

   if(blk[row]->dd!=NULL)
      free(blk[row]->dd);
   blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);
   
   for(i=0; i<blk[row]->cx; i++)
   {
      b= readttbyte(s+(i*8), ft[RACKIT].lp, ft[RACKIT].sp, ft[RACKIT].tp, ft[RACKIT].en);
      cb^=b;
      if(b==-1)
         rd_err++;
      blk[row]->dd[i] = b^xor;  /* (note the decryption being done here) */
   }
   b= readttbyte(blk[row]->p2+(2*8), ft[RACKIT].lp, ft[RACKIT].sp, ft[RACKIT].tp, ft[RACKIT].en); /* read actual checbyte */

   blk[row]->cs_exp= cb &0xFF;
   blk[row]->cs_act= b;
   blk[row]->rd_err= rd_err;
   return 0;
}

