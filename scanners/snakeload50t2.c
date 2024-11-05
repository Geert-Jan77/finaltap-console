/*---------------------------------------------------------------------------
  snakeload50t2.c (Snakeload 5.0, Threshold 2)

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

#define HDSZ 10

/*---------------------------------------------------------------------------
*/
void snakeload50t2_search(void)
{
   int i,sof,sod,eod,eof;
   int z,h,hd[HDSZ];
   unsigned int s,e,x;

   if(!quiet)
      msgout("  Snakeload 5.0 T2");
         
   
   for(i=20; i<tap.len-8; i++)
   {
      if((z=find_pilot(i,SNAKE50T2))>0)
      {
         sof=i;
         i=z;
         if(readttbit(i, ft[SNAKE50T2].lp, ft[SNAKE50T2].sp, ft[SNAKE50T2].tp)==ft[SNAKE50T2].sv)
         {
            i++;   /* skip sync bit */
            sod=i;
            /* decode the header so we can validate the addresses... */
            for(h=0; h<HDSZ; h++)
               hd[h] = readttbyte(sod+(h*8), ft[SNAKE50T2].lp, ft[SNAKE50T2].sp, ft[SNAKE50T2].tp, ft[SNAKE50T2].en);

            /* check for identifier string "eilyk"... */
            if(hd[0]==0x65 && hd[1]==0x69 && hd[2]==0x6C && hd[3]==0x79 && hd[4]==0x4B)
            {
               s=hd[6]+(hd[7]<<8);   /* get start address */
               e=hd[8]+(hd[9]<<8);   /* get end address */

               if(e>s)
               {
                  x=e-s;
                  eod=sod+((x+HDSZ)*8);
                  eof=eod+7;

                  /* trace 'eof' to end of trailer (skip through S pulses)... */
                  if(eof>0 && eof<tap.len-1)
                  {
                     while(tap.tmem[eof+1]>ft[SNAKE50T2].sp-tol && tap.tmem[eof+1]<ft[SNAKE50T2].sp+tol && eof<tap.len-1)
                        eof++;
                  }


                  addblockdef(SNAKE50T2, sof,sod,eod,eof, 0);
                  i=eof;  /* optimize search */
               }
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
int snakeload50t2_describe(int row)
{
   int i,s,b,hd[HDSZ],rd_err,cb;
   
   /* decode the header to get load address etc... */
   s=blk[row]->p2;
   for(i=0; i<HDSZ; i++)
      hd[i]= readttbyte(s+(i*8), ft[SNAKE50T2].lp, ft[SNAKE50T2].sp, ft[SNAKE50T2].tp, ft[SNAKE50T2].en);

   /* get start/end addresses and size */
   blk[row]->cs = hd[6]+(hd[7]<<8);
   blk[row]->ce = hd[8]+(hd[9]<<8)-1;
   blk[row]->cx = (blk[row]->ce - blk[row]->cs)+1;

   /* get pilot & trailer lengths */
   blk[row]->pilot_len = (blk[row]->p2 - blk[row]->p1 -8);
   blk[row]->trail_len = (blk[row]->p4 - blk[row]->p3 -7);

   /* extract data and test checksum... */
   rd_err=0;
   cb=0;
   s= blk[row]->p2+(HDSZ*8);

   if(blk[row]->dd!=NULL)
      free(blk[row]->dd);
   blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);

   for(i=0; i<blk[row]->cx; i++)
   {
      b= readttbyte(s+(i*8), ft[SNAKE50T2].lp, ft[SNAKE50T2].sp, ft[SNAKE50T2].tp, ft[SNAKE50T2].en);
      cb+=b;

      if(b==-1)
         rd_err++;
      blk[row]->dd[i]=b;
   }
   b= readttbyte(s+(i*8), ft[SNAKE50T2].lp, ft[SNAKE50T2].sp, ft[SNAKE50T2].tp, ft[SNAKE50T2].en); /* read actual checkbyte. */

   blk[row]->cs_exp= cb &0xFF;
   blk[row]->cs_act= b;
   blk[row]->rd_err= rd_err;

   return 0;
}





 
