/*---------------------------------------------------------------------------
  hitec.c
 
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
void hitec_search(void)
{
   int i,sof,sod,eod,eof;
   int z,tcnt,hd[HDSZ];
   int s,e,x;

   if(!quiet)
      msgout("  Hi-Tec tape");

   for(i=20; i<tap.len-8; i++)
   {
      if((z=find_pilot(i,HITEC))>0)
      {
         sof=i;
         i=z;
         if(readttbyte(i, ft[HITEC].lp, ft[HITEC].sp, ft[HITEC].tp, ft[HITEC].en)==ft[HITEC].sv)
         {
            sod=i+8;

            /* decode the header, so we can validate the addresses */
            for(tcnt=0; tcnt<HDSZ; tcnt++)
               hd[tcnt] = readttbyte(sod+(tcnt*8), ft[HITEC].lp, ft[HITEC].sp, ft[HITEC].tp, ft[HITEC].en);

            s = hd[2]+ (hd[3]<<8);
            e = hd[4]+ (hd[5]<<8);
            x = e-s;

            if(e>s)
            {
               eod = sod+ ((x+HDSZ)*8);
               eof = eod+7;
               addblockdef(HITEC, sof,sod,eod,eof, 0);
               i = eof;  /* optimize search */
            }
         }
      }
   }
}
/*---------------------------------------------------------------------------
*/
int hitec_describe(int row)
{
   int i,s,b,hd[HDSZ],rd_err,cb;

   s= blk[row]->p2;
   for(i=0; i<HDSZ; i++)    /* decode the header... */
   {
      b= readttbyte(s+(i*8), ft[HITEC].lp, ft[HITEC].sp, ft[HITEC].tp, ft[HITEC].en);
      hd[i]=b;
   }

   blk[row]->cs= hd[2]+(hd[3]<<8);
   blk[row]->ce= hd[4]+(hd[5]<<8)-1;
   blk[row]->cx= (blk[row]->ce - blk[row]->cs)+1;

   sprintf(lin,"\n - Block Number : $%02X",hd[0]);
   strcat(info,lin);
   sprintf(lin,"\n - Execution Address : $%04X",hd[6]+(hd[7]<<8));
   strcat(info,lin);

   /* get pilot & trailer length... */
   blk[row]->pilot_len= ((blk[row]->p2 - blk[row]->p1)>>3)-1;
   blk[row]->trail_len=0;
    
   /* extract data and test checksum... */
   rd_err= 0;
   cb= 0;
   s= (blk[row]->p2)+(HDSZ*8);

   if(blk[row]->dd!=NULL)
      free(blk[row]->dd);
   blk[row]->dd= (unsigned char*)malloc(blk[row]->cx);
   
   for(i=0; i<blk[row]->cx; i++)
   {
      b= readttbyte(s+(i*8), ft[HITEC].lp, ft[HITEC].sp, ft[HITEC].tp, ft[HITEC].en);
      cb^=b;
      if(b==-1)
         rd_err++;
      blk[row]->dd[i]=b;
   }
   b= readttbyte(s+(i*8), ft[HITEC].lp, ft[HITEC].sp, ft[HITEC].tp, ft[HITEC].en);

   blk[row]->cs_exp= cb &0xFF;
   blk[row]->cs_act= b;
   blk[row]->rd_err= rd_err;
   return 0;
}



