/*---------------------------------------------------------------------------
  bleep_f1.c (Bleepload)
  
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


  Notes:-

  slightly special location format
  because of the dummy byte stuff :-


  sof, sod, eod, eof
                  |
            |     points at last pulse of execute address (2 after checkbyte.)
            |
      points at last
      data byte, NOT
      at checkbyte.

---------------------------------------------------------------------------*/

#include "../mydefs.h"
#include "../main.h"
    
/*---------------------------------------------------------------------------*/
void bleep_search(void)
{
   int i,sof,sod,eod,eof;
   int zcnt,len,plt,b;

   if(!quiet)
      msgout("  Bleepload");
         

   plt= 0x0F;  /* set inital pilot value. */

   for(i=20; i<tap.len; i++)
   {
      b= readttbyte(i, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, ft[BLEEP].en);  /* look for a pilot byte */
      if(b==plt)
      {
         sof=i;

         zcnt=0;
         while(readttbyte(i, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, ft[BLEEP].en)==plt)   /* look for a sequence of at least x... */
         {
            i+=8;
            zcnt++;
         }
         if(zcnt>4 && readttbyte(i, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, ft[BLEEP].en)==(plt^0xFF))  /* ending with a sync. */
         {
            if(readttbyte(i+8, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, ft[BLEEP].en)==plt) /* after sync is one more pilot */
            {
               sod= i+16;  /* points to 1st byte after 2nd sync (same as pilot byte) */

               /* decode and set new pilot value...  */
               plt = readttbyte(sod, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, ft[BLEEP].en);

               /* decode block number to discover data size... */
               /* block 00 is 64 bytes, others are 256.        */
               b= readttbyte(sod+8, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, ft[BLEEP].en);
               if(b==0)
               {
                  len= 64 +4;   /* +4 accounts for header  */
                  /* trace "sof" back to start of L pulse leader on block 0... */
                  if(tap.tmem[sof-1]>ft[BLEEP].lp-tol && tap.tmem[sof-1]<ft[BLEEP].lp+tol)
                  {
                     while(tap.tmem[sof-1]>ft[BLEEP].lp-tol && tap.tmem[sof-1]<ft[BLEEP].lp+tol)
                        sof--;
                  }
               }
               else
                  len= 256 +4;  /* +4 accounts for header */

               eod= sod+(len*8)-8;

               /* now get dummy byte count... */
               b= readttbyte(eod+8, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, ft[BLEEP].en);
               eof= eod+((b+1)*8)+24+7;  /* b+1 accounts for the count itself   */
                                         /* +24 accounts for exe address and spare byte */

               addblockdef(BLEEP, sof,sod,eod,eof, b); /* note: last param is dummy byte count. */


               /* check for presence of final trigger block (8 bytes)   */
               if(readttbyte(eof+1, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, ft[BLEEP].en)==0)
               {
                  sof= eof+1;
                  sod= sof;
                  eod= sof+56;
                  eof= sof+63;

                  /* override 'eof' if there is a trailer tone...  */
                  if(readttbyte(eof+1, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, ft[BLEEP].en)==0xFF)
                  {
                     while(tap.tmem[eof+1]>ft[BLEEP].lp-tol && tap.tmem[eof+1]<ft[BLEEP].lp+tol)
                        eof++;
                  }
                  addblockdef(BLEEP_TRIG,sof,sod,eod,eof,0);
               }
               i= eof;  /* optimize search */
            }
         }
      }
   }
}
/*---------------------------------------------------------------------------*/
int bleep_describe(int row)
{
   int s,t,i,b1,b2;
   int hd[4],b,rd_err,cb;
   
   if(blk[row]->lt==BLEEP_TRIG)
   {
      sprintf(lin,"\n - Data length : 8 bytes (always)");
      strcat(info,lin);

      /* get pilot/trailer length... */
      blk[row]->pilot_len= 0;
      blk[row]->trail_len= blk[row]->p4 - (blk[row]->p3+7);
   }

   /*---------------------------------------------------------------------------*/
   if(blk[row]->lt==BLEEP)
   {
      s= blk[row]->p2;
      t= blk[row]->xi;      /* get dummy byte count. */

      /* decode header... */
      for(i=0; i<4; i++)
         hd[i]= readttbyte(s+(i*8), ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, ft[BLEEP].en);

      sprintf(lin,"\n - Block number : $%02X", hd[1]);
      strcat(info,lin);

      blk[row]->cs= hd[2]+(hd[3]<<8);  /* record load addr. */
      if(hd[1]==0)   /* get data size from block number...  */
         blk[row]->cx= 64;
      else
         blk[row]->cx= 256;
      blk[row]->ce  = ((blk[row]->cs + blk[row]->cx) & 0xFFFF)-1;  /* compute end addr. */

      sprintf(lin,"\n - Pilot value for next block : $%02X", hd[0]);
      strcat(info,lin);
      sprintf(lin,"\n - Dummy bytes : %d", t);
      strcat(info,lin);

      /* get execution address...  */
      s= blk[row]->p3+(t*8)+(3*8);
      b1= readttbyte(s+0, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, ft[BLEEP].en);
      b2= readttbyte(s+8, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, ft[BLEEP].en);
      sprintf(lin,"\n - Execution address : $%04X", (b2<<8)+b1);
      strcat(info,lin);

      /* get pilot length... */
      blk[row]->pilot_len= ((blk[row]->p2- blk[row]->p1)/8) -2;
      blk[row]->trail_len= 0;

      /* extract data and test checksum... */
      rd_err= 0;
      cb= 0;
      s= (blk[row]->p2)+(4*8);

      if(blk[row]->dd!=NULL)
      free(blk[row]->dd);
      blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);
      
      for(i=0; i<blk[row]->cx; i++)
      {
         b= readttbyte(s+(i*8), ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, ft[BLEEP].en);
         cb^=b;
         if(b==-1)
            rd_err++;
         blk[row]->dd[i]=b;
      }
      i+=blk[row]->xi+1;  /* jump over dummmy bytes */
      
      b= readttbyte(s+(i*8), ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, ft[BLEEP].en); /* read checkbyte */
      blk[row]->cs_exp= cb &0xFF;
      blk[row]->cs_act= b;
      blk[row]->rd_err= rd_err;
      return 0;
   }
   return 0;
}



