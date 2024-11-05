/*---------------------------------------------------------------------------
  anirog.c (Anirog/Skramble Turbotape)
  
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

/*---------------------------------------------------------------------------*/
void anirog_search(void)
{
   int i,j,z,cnt2,sof,sod,eod,eof;
   int pat[10],hd[HDSZ];
   int s,e,x;

   if(!quiet)
      msgout("  Anirog tape");
   
   for(i=20; i<tap.len-8; i++)
   {
      if((z=find_pilot(i,ANIROG))>0)
      {
         sof= i;
         i= z;
         if(readttbyte(i, ft[ANIROG].lp, ft[ANIROG].sp, ft[ANIROG].tp, ft[ANIROG].en)==ft[ANIROG].sv)
         {
            for(cnt2=0; cnt2<9; cnt2++)    /* decode a 10 byte sequence */
               pat[cnt2] = readttbyte(i+(cnt2*8), ft[ANIROG].lp, ft[ANIROG].sp, ft[ANIROG].tp, ft[ANIROG].en);

            if(pat[0]==0x09 && pat[1]==0x08 && pat[2]==0x07 && pat[3]==0x06 && pat[4]==0x05 &&
            pat[5]==0x04 && pat[6]==0x03 && pat[7]==0x02 && pat[8]==0x01 )
            {
               sod= i+72;

               /* decode the header, so we can validate the addresses... */
               for(j=0; j<HDSZ; j++)
                  hd[j] = readttbyte(sod+(j*8), ft[ANIROG].lp, ft[ANIROG].sp, ft[ANIROG].tp, ft[ANIROG].en);

               s= hd[1]+ (hd[2]<<8);   /* get start address */
               e= hd[3]+ (hd[4]<<8);   /* get end address   */

               if(e>s)
               {
                  x= e-s-1;
                  eod= sod+((x+HDSZ)*8);
                  eof= eod+7;
                  addblockdef(ANIROG, sof,sod,eod,eof, 0);
                  i= eof;   /* optimize search */
               }
            }
         }
      }
      else
      {
         if(z<0)    /* find_pilot() failed (too few/many), set i to failure point. */
            i=(-z);
      }
   }
}
/*---------------------------------------------------------------------------*/
int anirog_describe(int row)
{
   int i,s;
   int b,hd[HDSZ],rd_err;

   /* decode header... */
   s= blk[row]->p2;
   for(i=0; i<HDSZ; i++)
      hd[i]= readttbyte(s+(i*8), ft[ANIROG].lp, ft[ANIROG].sp, ft[ANIROG].tp, ft[ANIROG].en);
   
   /* compute C64 start address, end address and size...  */
   blk[row]->cs= hd[1]+ (hd[2]<<8);
   blk[row]->ce= hd[3]+ (hd[4]<<8)-1;
   blk[row]->cx= (blk[row]->ce - blk[row]->cs) +1;

   /* get pilot trailer lengths...  */
   blk[row]->pilot_len= (blk[row]->p2- blk[row]->p1 -8) >>3;
   blk[row]->trail_len= (blk[row]->p4- blk[row]->p3 -7) >>3;
         
   /* extract data... */
   rd_err=0;
   s= (blk[row]->p2)+(HDSZ*8);

   if(blk[row]->dd!=NULL)
      free(blk[row]->dd);
   blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);
   
   for(i=0; i<blk[row]->cx; i++)
   {
      b= readttbyte(s+(i*8), ft[ANIROG].lp, ft[ANIROG].sp, ft[ANIROG].tp, ft[ANIROG].en);
      if(b==-1)
         rd_err++;
      blk[row]->dd[i]= b;
   }
   blk[row]->rd_err= rd_err;
   return 0;
}

