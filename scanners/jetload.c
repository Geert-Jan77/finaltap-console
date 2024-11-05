/*---------------------------------------------------------------------------
  jetload.c

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
void jetload_search(void)
{
   int i,sof,sod,eod,eof;
   int z,tcnt,hd[HDSZ];
   int s,e,x;

   if(!quiet)
      msgout("  Jetload");
            
   for(i=20; i<tap.len-8; i++)
   {
      if((z=find_pilot(i,JET))>0)
      {
         sof=i;
         i=z;
         if(readttbyte(i, ft[JET].lp, ft[JET].sp, ft[JET].tp, ft[JET].en)==ft[JET].sv)
         {
            sod= i+8;
            /* decode the header, so we can validate the addresses... */
            for(tcnt=0; tcnt<HDSZ; tcnt++)
               hd[tcnt]= readttbyte(sod+(tcnt*8), ft[JET].lp, ft[JET].sp, ft[JET].tp, ft[JET].en);
            s= hd[0]+ (hd[1]<<8);   /* get start address */
            e= hd[2]+ (hd[3]<<8);   /* get end address */
            if(e>s)
            {
               x= (e-s)-1;
               eod= sod+ ((x+HDSZ)*8);
               eof= eod+7;
               /* just step sof back through any S pulses... */
               while(tap.tmem[sof-1]>ft[JET].sp-tol && tap.tmem[sof-1]<ft[JET].sp+tol && !is_pause_param(sof-1))
                  sof--;
               addblockdef(JET, sof,sod,eod,eof, 0);
               i= eof;  /* optimize search */
            }
         }
      }
      else
      {
         if(z<0)    /* find_pilot failed (too few/many), set i to failure point.   */
            i=(-z);
      }
   }
}
/*---------------------------------------------------------------------------
*/
int jetload_describe(int row)
{
   int i,s,b,hd[HDSZ],rd_err;

   /* decode the header to get load address etc... */
   s= blk[row]->p2;
   for(i=0; i<HDSZ; i++)
      hd[i] = readttbyte(s+(i*8), ft[JET].lp, ft[JET].sp, ft[JET].tp, ft[JET].en);

   blk[row]->cs= hd[0]+ (hd[1]<<8);              /* record start address */
   blk[row]->ce= hd[2]+ (hd[3]<<8)-1;            /* record end address */
   blk[row]->cx= (blk[row]->ce - blk[row]->cs)+1;  /* record length in bytes */

   /* get pilot & trailer lengths */
   blk[row]->pilot_len = blk[row]->p2- blk[row]->p1-16;
   blk[row]->trail_len =0;

   /* extract data... */
   rd_err=0;
   s= blk[row]->p2+(HDSZ*8);

   if(blk[row]->dd!=NULL)
      free(blk[row]->dd);
   blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);
   
   for(i=0; i<blk[row]->cx; i++)
   {
      b= readttbyte(s+(i*8), ft[JET].lp, ft[JET].sp, ft[JET].tp, ft[JET].en);
      if(b==-1)
         rd_err++;
      blk[row]->dd[i]= b;
   }
   blk[row]->rd_err= rd_err;

   return 0;
}





