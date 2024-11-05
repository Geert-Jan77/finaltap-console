/*---------------------------------------------------------------------------
  enigma.c (Enigma Variations)

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

/*---------------------------------------------------------------------------
*/
void enigma_search(void)
{
   int i,sof,sod,eod,eof;
   int z,b;

   if(!quiet)
      msgout("  Enigma Variations Tape");
      
   for(i=20; i<tap.len-8; i++)
   {
      if((z=find_pilot(i,ENIGMA))>0)
      {
         sof=i;
         i=z;
         if(readttbyte(i, ft[ENIGMA].lp, ft[ENIGMA].sp, ft[ENIGMA].tp, ft[ENIGMA].en)==ft[ENIGMA].sv)
         {
            sod=i+8;
            /* scan through all readable bytes... */
            do
            {
               b=readttbyte(i, ft[ENIGMA].lp, ft[ENIGMA].sp, ft[ENIGMA].tp, ft[ENIGMA].en);
               if(b!=-1)
                  i+=8;
            }
            while(b!=-1);
            i-=8;
            eod=i;
            eof=i+7;

            addblockdef(ENIGMA, sof,sod,eod,eof, 0);
            i=eof;  /* optimize search */
         }
      }
   }
}
/*---------------------------------------------------------------------------
*/
int enigma_describe(int row)
{
   int i,s,b;

   /* get pilot & trailer length */
   blk[row]->pilot_len= (blk[row]->p2- blk[row]->p1 -8)>>3;
   blk[row]->trail_len=0;

   /* compute data size... */
   blk[row]->cx = ((blk[row]->p3 - blk[row]->p2)>>3)+1;
   blk[row]->ce = blk[row]->cx-1;  /* fake end address. */

   /* extract data... */
   s= blk[row]->p2;

   if(blk[row]->dd!=NULL)
      free(blk[row]->dd);
   blk[row]->dd= (unsigned char*)malloc(blk[row]->cx);

   for(i=0; i<blk[row]->cx; i++)
   {
      b= readttbyte(s+(i*8), ft[ENIGMA].lp, ft[ENIGMA].sp, ft[ENIGMA].tp, ft[ENIGMA].en);
      if(b==-1)
         blk[row]->rd_err++;
      blk[row]->dd[i]= b;
   }
   return 0;
}



