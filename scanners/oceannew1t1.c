/*---------------------------------------------------------------------------
  oceannew1t1.c (Ocean New Format 1, Threshold 1)

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

#define HDSZ 5

/*---------------------------------------------------------------------------
*/
void oceannew1t1_search(void)
{
   int i,sof,sod,eod,eof;
   int z,h,hd[HDSZ];
   unsigned int s,e,x;

   if(!quiet)
      msgout("  New Ocean Tape 1 T1");
         
   
   for(i=20; i<tap.len-8; i++)
   {
      if((z=find_pilot(i,OCNEW1T1))>0)
      {
         sof=i;
         i=z;
         if(readttbyte(i, ft[OCNEW1T1].lp, ft[OCNEW1T1].sp, ft[OCNEW1T1].tp, ft[OCNEW1T1].en)==ft[OCNEW1T1].sv)
         {
            sod=i+8;
            /* decode the header so we can validate the addresses... */
            for(h=0; h<HDSZ; h++)
               hd[h] = readttbyte(sod+(h*8), ft[OCNEW1T1].lp, ft[OCNEW1T1].sp, ft[OCNEW1T1].tp, ft[OCNEW1T1].en);
            s= hd[1]+(hd[2]<<8);   /* get start address */
            e= hd[3]+(hd[4]<<8);   /* get end address */
            if(e>s)
            {
               x=e-s;
               eod=sod+((x+HDSZ)*8);
               eof=eod+7;
               addblockdef(OCNEW1T1, sof,sod,eod,eof, 0);
               i=eof;  /* optimize search */
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
int oceannew1t1_describe(int row)
{
   int i,s,b,hd[HDSZ],cb;
   
   /* decode the header to get load address etc... */
   s=blk[row]->p2;
   for(i=0; i<HDSZ; i++)
      hd[i] = readttbyte(s+(i*8), ft[OCNEW1T1].lp, ft[OCNEW1T1].sp, ft[OCNEW1T1].tp, ft[OCNEW1T1].en);

   blk[row]->cs= hd[1]+(hd[2]<<8);
   blk[row]->ce= hd[3]+(hd[4]<<8)-1;
   blk[row]->cx= (blk[row]->ce - blk[row]->cs)+1;

   sprintf(lin,"\n - ID Byte: $%02X",hd[0]); 
   strcat(info,lin);

   /* get pilot & trailer lengths */
   blk[row]->pilot_len= (blk[row]->p2- blk[row]->p1 -8)>>3;
   blk[row]->trail_len= (blk[row]->p4- blk[row]->p3 -7)>>3;

   /* extract data and test checksum... */
   cb=0;
   s= blk[row]->p2+(HDSZ*8);

   if(blk[row]->dd!=NULL)
      free(blk[row]->dd);
   blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);

   for(i=0; i<blk[row]->cx; i++)
   {
      b= readttbyte(s+(i*8), ft[OCNEW1T1].lp, ft[OCNEW1T1].sp, ft[OCNEW1T1].tp, ft[OCNEW1T1].en);
      cb^=b;
      if(b==-1)
         blk[row]->rd_err++;
      blk[row]->dd[i]=b;
   }
   b= readttbyte(s+(i*8), ft[OCNEW1T1].lp, ft[OCNEW1T1].sp, ft[OCNEW1T1].tp, ft[OCNEW1T1].en); /* read actual checkbyte. */

   blk[row]->cs_exp= cb &0xFF;
   blk[row]->cs_act= b;
   return 0;
}




 
