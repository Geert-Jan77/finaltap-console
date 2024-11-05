/*---------------------------------------------------------------------------
  freeload.c

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

#define HDSZ 4

/*---------------------------------------------------------------------------
*/
void freeload_search(void)
{
   int i,sof,sod,eod,eof;
   int z,h,hd[HDSZ];
   int en,tp,sp,lp,sv;
   unsigned int s,e,x;

   en= ft[FREE].en;
   tp= ft[FREE].tp;
   sp= ft[FREE].sp;
   lp= ft[FREE].lp;
   sv= ft[FREE].sv;

   if(!quiet)
      msgout("  Freeload (+clones)");
     
    
   for(i=20; i<tap.len-8; i++)
   {
      if((z=find_pilot(i,FREE))>0)
      {
         sof=i;
         i=z;
         if(readttbyte(i,lp,sp,tp,en)==sv)
         {
            sod= i+8;
            /* decode the header so we can validate the addresses... */
            for(h=0; h<HDSZ; h++)
               hd[h]= readttbyte(sod+(h*8),lp,sp,tp,en);
            s= hd[0]+(hd[1]<<8);   /* get start address */
            e= hd[2]+(hd[3]<<8);   /* get end address */
            if(e>s)
            {
               x= e-s;
               eod= sod+((x+HDSZ)*8);
               eof= eod+7;
               addblockdef(FREE, sof,sod,eod,eof, 0);
               i= eof;  /* optimize search */
            }
         }
      }
      else
      {
         if(z<0)    /* find_pilot failed (too few/many), set i to failure point. */
            i=(-z);
      }
   }
}
/*---------------------------------------------------------------------------
*/
int freeload_describe(int row)
{
   int i,s,b,cb,hd[HDSZ];
      
   /* decode header... */
   s= blk[row]->p2;
   for(i=0; i<HDSZ; i++)
      hd[i]= readttbyte(s+(i*8), ft[FREE].lp, ft[FREE].sp, ft[FREE].tp, ft[FREE].en);

   blk[row]->cs= hd[0]+(hd[1]<<8);
   blk[row]->ce= hd[2]+(hd[3]<<8)-1;
   blk[row]->cx= (blk[row]->ce - blk[row]->cs)+1;  

   /* get pilot & trailer lengths */
   blk[row]->pilot_len= (blk[row]->p2- blk[row]->p1 -8)>>3;
   blk[row]->trail_len=0;

   /* extract data and test checksum... */
   cb= 0;
   s= blk[row]->p2+(HDSZ*8);

   if(blk[row]->dd!=NULL)
      free(blk[row]->dd);
   blk[row]->dd= (unsigned char*)malloc(blk[row]->cx);

   for(i=0; i<blk[row]->cx; i++)
   {
      b= readttbyte(s+(i*8), ft[FREE].lp, ft[FREE].sp, ft[FREE].tp, ft[FREE].en);
      cb^= b;
      if(b==-1)
         blk[row]->rd_err++;
      blk[row]->dd[i]= b;
   }
   b= readttbyte(s+(i*8), ft[FREE].lp, ft[FREE].sp, ft[FREE].tp, ft[FREE].en); /* read actual checkbyte. */

   blk[row]->cs_exp= cb &0xFF;
   blk[row]->cs_act= b;
   return 0;
}



