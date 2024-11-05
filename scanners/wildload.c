/*---------------------------------------------------------------------------
  wildload.c

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
void wild_search(void)
{
   int i,sof,sod,eod,eof;
   int pos,cnt2,z,len;
   int hd[HDSZ],pat[10];

   if(!quiet)
      msgout("  Wildload");
         

   for(i=20; i<tap.len-8; i++)
   {
      if((z=find_pilot(i,WILD))>0)
      {
         sof=i;
         i=z;
         if(readttbyte(i, ft[WILD].lp, ft[WILD].sp, ft[WILD].tp, ft[WILD].en)==ft[WILD].sv)  /* ending with a sync.  */
         {
            for(cnt2=0; cnt2<10; cnt2++)
               pat[cnt2] = readttbyte(i+(cnt2*8), ft[WILD].lp, ft[WILD].sp, ft[WILD].tp, ft[WILD].en);   /* decode a 10 byte TT sequence  */

             if(pat[0]==0x0A && pat[1]==9 && pat[2]==8 && pat[3]==7 && pat[4]==6
               && pat[5]==5 && pat[6]==4 && pat[7]==3 && pat[8]==2 && pat[9]==1)
            {
               sod= i+80;

               /* decode header...  */
               for(cnt2=0; cnt2<HDSZ; cnt2++)
               {
                  hd[cnt2] = readttbyte(sod+(cnt2*8), ft[WILD].lp, ft[WILD].sp, ft[WILD].tp, ft[WILD].en);
                  if(hd[cnt2]==-1)
                     return;        /* read error in header, abort.  */
               }

               len= (hd[2])+(hd[3]<<8);

               eod= sod+(len*8)+(HDSZ*8);
               eof= eod+7;

               addblockdef(WILD, sof,sod,eod,eof, 0);  /* add 1st block   */


               /* locate any sub-blocks...   */
               pos= eof+1;    /* set pointer to next possible block  */
               do
               {
                  /* find data length for this block...  */
                  for(cnt2=0; cnt2<HDSZ; cnt2++)
                  {
                     hd[cnt2] = readttbyte(pos+(cnt2*8), ft[WILD].lp, ft[WILD].sp, ft[WILD].tp, ft[WILD].en);
                     if(hd[cnt2]==-1)
                        return;        /* read error in header, abort. */
                  }
                  len = (hd[2])+(hd[3]<<8);   /* read length of this block  */
                  if(len!=0) /* note if length read is $0000 then we're finished.  */
                  {
                     sof= pos;
                     sod= sof;
                     eod= pos+ (len*8)+(HDSZ*8);
                     eof= eod+7;
                     addblockdef(WILD, sof,sod,eod,eof, 0);
                     pos= eof+1;  /* bump pointer  */
                  }
                  else /* add stop block...   */
                  {
                     sof= pos;
                     sod= sof;
                     eod= sod+40;
                     eof= eod+6;   /* size is always 47 pulses.  */
                     addblockdef(WILD_STOP, sof,sod,eod,eof, 0);
                     pos= eof+1;  /* bump pointer  */
                  }
               }
               while(len!=0 && pos<tap.len);

               i=pos;  /* make sure following headblock search starts at correct position */
            }
         }
      }
   }
}
/*---------------------------------------------------------------------------*/
int wild_describe(int row)
{
   int i,s,len,scnt;
   int b,rd_err,cb,hd[HDSZ];

   if(blk[row]->lt==WILD)
   {
      /* decode the header... */
      s = blk[row]->p2;
      for(i=0; i<HDSZ; i++)
         hd[i] = readttbyte(s+(i*8), ft[WILD].lp, ft[WILD].sp, ft[WILD].tp, ft[WILD].en);

      /* compute C64 start address, end address and size...  */
      blk[row]->ce = hd[0]+ (hd[1]<<8);  /* (note: data loads backwards)   */
      blk[row]->cx = hd[2]+ (hd[3]<<8);
      blk[row]->cs = ((blk[row]->ce - blk[row]->cx)+1)&0xFFFF;

      if(blk[row]->xi!=0)
      {
         sprintf(lin,"\n - Last word: $%04X",blk[row]->xi);
         strcat(info,lin);
      }

      /* get pilot/trailer lengths... */
      blk[row]->pilot_len= (blk[row]->p2 - blk[row]->p1) >>3;
      blk[row]->trail_len= (blk[row]->p4 - blk[row]->p3 -7) >>3;

      /* extract data... (loaded backwards using 'len')    */
      rd_err=0;
      cb=0;
      s= (blk[row]->p2)+(HDSZ*8);

      if(blk[row]->dd!=NULL)
         free(blk[row]->dd);
      blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);

      scnt= blk[row]->ce;  /* ...end (start) address is required for correct decoding. */
      len = blk[row]->cx;

      for(i=0; i<blk[row]->cx; i++)
      {
         b= readttbyte(s+(i*8), ft[WILD].lp, ft[WILD].sp, ft[WILD].tp, ft[WILD].en);
         cb^= (b^(scnt&0xFF));
         if(b==-1)
            rd_err++;
         else
            blk[row]->dd[len-1-i] = b ^ (scnt & 0xFF);
         scnt--;
      }
      b= readttbyte(s+(i*8), ft[WILD].lp, ft[WILD].sp, ft[WILD].tp, ft[WILD].en); /* read actual cb.  */
      blk[row]->cs_exp = cb &0xFF;
      blk[row]->cs_act = b;
      blk[row]->rd_err = rd_err;
   }

   /*-----------------------------------------------------------*/
   if(blk[row]->lt==WILD_STOP)
   {
      sprintf(lin,"\n - Size : 47 pulses (always)");
      strcat(info,lin);

      /* decode 5 bytes... (they are not stored, just read for error purposes) */
      s= blk[row]->p2;
      for(i=0; i<HDSZ; i++)
         readttbyte(s+(i*8), ft[WILD].lp, ft[WILD].sp, ft[WILD].tp, ft[WILD].en);
   }
   return 0;
}

