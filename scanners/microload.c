/*---------------------------------------------------------------------------
  microload.c

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
void micro_search(void)
{
   int i,sof,sod,eod,eof;
   int x,z,hd[HDSZ];

   if(!quiet)
      msgout("  Microload");
         

   for(i=20; i<tap.len-8; i++)
   {
      if((z=find_pilot(i,MICROLOAD))>0)
      {
         sof=i;
         i=z;
         if(readttbyte(i, ft[MICROLOAD].lp, ft[MICROLOAD].sp, ft[MICROLOAD].tp, ft[MICROLOAD].en)==ft[MICROLOAD].sv)
         {
            if(readttbyte(i+8, ft[MICROLOAD].lp, ft[MICROLOAD].sp, ft[MICROLOAD].tp, ft[MICROLOAD].en)==0x09) /* i should really check whole sequence */
            {
               sod = i+(10*8);

               /* decode the header... */
               for(i=0; i<HDSZ; i++)
                  hd[i] = readttbyte(sod+(i*8), ft[MICROLOAD].lp, ft[MICROLOAD].sp, ft[MICROLOAD].tp, ft[MICROLOAD].en);

               x = ((hd[2]+ (hd[3]<<8))^0xFFFF)+1;  /* get length */
               
               eod = sod+(x*8)+(HDSZ*8);
               eof = eod+7;

               addblockdef(MICROLOAD, sof,sod,eod,eof, 0);
               i = eof;  /* optimize search */
            }
         }
      }
   }
}

/*---------------------------------------------------------------------------
*/
int micro_describe(int row)
{
   int i,s,b,rd_err,hd[HDSZ],cb;

   /* decode the header... */
   s= blk[row]->p2;
   for(i=0; i<HDSZ; i++)
      hd[i]= readttbyte(s+(i*8), ft[MICROLOAD].lp, ft[MICROLOAD].sp, ft[MICROLOAD].tp, ft[MICROLOAD].en);

   /* compute C64 start address, end address and size... */
   blk[row]->cs= hd[0]+ (hd[1]<<8);
   blk[row]->cx= ((hd[2]+ (hd[3]<<8))^0xFFFF)+1;
   blk[row]->ce= (blk[row]->cs+blk[row]->cx)-1;

   /* get pilot & trailer lengths */
   blk[row]->pilot_len= ((blk[row]->p2 - blk[row]->p1) -(10*8)) >>3;
   blk[row]->trail_len= (blk[row]->p4 - blk[row]->p3 -7) >>3;

   /* extract data and test checksum... */
   rd_err=0;
   cb=0;
   s= (blk[row]->p2)+(HDSZ*8);

   if(blk[row]->dd!=NULL)
      free(blk[row]->dd);
   blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);
      
   for(i=0; i<blk[row]->cx; i++)
   {
      b= readttbyte(s+(i*8), ft[MICROLOAD].lp, ft[MICROLOAD].sp, ft[MICROLOAD].tp, ft[MICROLOAD].en);
      cb^=b;
      if(b==-1)
         rd_err++;
      blk[row]->dd[i]=b;
   }
   b= readttbyte(s+(i*8), ft[MICROLOAD].lp, ft[MICROLOAD].sp, ft[MICROLOAD].tp, ft[MICROLOAD].en);
   blk[row]->cs_exp= cb &0xFF;
   blk[row]->cs_act= b;
   blk[row]->rd_err= rd_err;
   return 0;
}


