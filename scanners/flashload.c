/*---------------------------------------------------------------------------
  flashload.c

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

#define SYNC 0x33
#define HDSZ 4

/*---------------------------------------------------------------------------
*/
void flashload_search(void)
{
   int i,sof,sod,eod,eof;
   int s,e,x,z,tcnt,hd[HDSZ];

   if(!quiet)
      msgout("  Flashload");
         

   for(i=20; i<tap.len-8; i++)
   {
      if((z=find_pilot(i,FLASH))>0) 
      {
         sof=i;
         i=z;
         if(readttbit(i,ft[FLASH].lp, ft[FLASH].sp, ft[FLASH].tp)==ft[FLASH].sv) /* got sync bit? */
         {
            i++; /* jump sync bit. */
            if(readttbyte(i, ft[FLASH].lp, ft[FLASH].sp, ft[FLASH].tp, MSbF)==SYNC)  /* got sync byte?. */
            {
               sod=i+8;

               /* decode the header, so we can validate the addresses */
               for(tcnt=0; tcnt<HDSZ; tcnt++)
                  hd[tcnt] = readttbyte(sod+(tcnt*8), ft[FLASH].lp, ft[FLASH].sp, ft[FLASH].tp, MSbF);

               s= hd[0]+ (hd[1]<<8);   /* get start address */
               e= hd[2]+ (hd[3]<<8);   /* get end address */

               if(e>s)
               {
                  x= e-s;     /* calculate last pulse from these addresses */
                  x*=8;
                  eod= sod+x+(HDSZ*8)+8;
                  eof= eod+7;
                  addblockdef(FLASH, sof,sod,eod,eof, 0);
                  i= eof;  /* optimize search */
               }
            }
         }
      }
   }
}
/*---------------------------------------------------------------------------
*/
int flashload_describe(int row)
{
   int i,s,b,cb,hd[HDSZ],rd_err;

   s= blk[row]->p2;    /* decode the header... */
   for(i=0; i<HDSZ; i++)
      hd[i] = readttbyte(s+(i*8), ft[FLASH].lp, ft[FLASH].sp, ft[FLASH].tp, MSbF);

   blk[row]->cs= hd[0]+ (hd[1]<<8);
   blk[row]->ce= hd[2]+ (hd[3]<<8);
   blk[row]->cx= (blk[row]->ce - blk[row]->cs)+1;

   /* get pilot & trailer lengths */
   blk[row]->pilot_len= (blk[row]->p2 - blk[row]->p1 -8)-1;
   blk[row]->trail_len=0;

   /* extract data and test checksum... */
   rd_err=0;
   cb=0;
   s= (blk[row]->p2)+(HDSZ*8);

   if(blk[row]->dd!=NULL)
      free(blk[row]->dd);
   blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);
   
   for(i=0; i<blk[row]->cx; i++)
   {
      b= readttbyte(s+(i*8), ft[FLASH].lp, ft[FLASH].sp, ft[FLASH].tp, MSbF);
      cb^= b;
      if(b==-1)
         rd_err++;
      blk[row]->dd[i]= b;
   }
   b= readttbyte(s+(i*8), ft[FLASH].lp, ft[FLASH].sp, ft[FLASH].tp, MSbF); /* read checksum */

   blk[row]->cs_exp= cb &0xFF;
   blk[row]->cs_act= b;
   blk[row]->rd_err= rd_err;
   return 0;
}

 
