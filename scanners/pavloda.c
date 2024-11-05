/*---------------------------------------------------------------------------
  pavloda.c

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
   

  Note : works perfectly, but many taps are missing pulses at very end of file
         ie. last pulse or 2 of the checkbyte, manual correction is required
         for these taps.

---------------------------------------------------------------------------*/

#include "../mydefs.h"
#include "../main.h"

#define HDSZ 4

/*---------------------------------------------------------------------------
*/
int pav_readbyte(int pos)
{
   int i,valid,val, extras;
   int bitpos,bit,byt;
   unsigned char byte[10];

   bitpos=0;
   i=0;
   extras=0;
   while(bitpos<8)   /* note: bit 0 is "L", bit 1 is "S,S" */
   {
      if(pos+i<20 || pos+i>tap.len-1) /* check next 8 bits are inside the TAP.. */
         return -1;

      if(is_pause_param(pos+i))
      {
         add_read_error(pos+i); /* read error, unexpected pause */
         return -1;
      }
      
      valid=0;
      byt = tap.tmem[pos+i];
      if(byt>(ft[PAV].sp-tol) && byt<(ft[PAV].sp+tol))     /* we found a valid SHORT pulse */
      {
         bit = 1;
         valid++;
      }
      if(byt>(ft[PAV].lp-tol) && byt<(ft[PAV].lp+tol))    /* we found a valid LONG pulse */
      {
         bit = 0;
         valid++;
      }

      if(valid==0)  /* pulse didnt qualify as either SHORT or LONG... */
      {
         add_read_error(pos+i);  /* read error, pulsewidth out of range */
         return -1;
      }

      if(valid==2)  /* pulse qualified as 0 AND 1!!!..... */
      {
         /* use closest match... */
         if((abs(ft[PAV].lp-byt)) > (abs(ft[PAV].sp-byt)))
            bit=0;
         else
            bit=1;
      }

      byte[bitpos++]=bit;   /* store the bit. */
      i++;                  /* bump tmem index to next pulse */

      if(bit==1)     /* bit 1's use 2 pulses, 2nd is ignored... */
      {
         i++;        /* bump tmem index to next pulse (AGAIN). */
         extras++;   /* count additional pulses used. */
      }
   }

   /* if we get this far, we have 8 valid bits...decode the byte (MSbF)... */
   val=0;
   for(i=0; i<8; i++)
   {
      if(byte[i]==1)
         val+=(128>>i);
   }

   return((extras<<8)+val);  /* return byte value + no. extra bits in upper byte. */
}
/*---------------------------------------------------------------------------
*/
void pav_search(void)
{
   int i,sof,sod,eod,eof, s,e,x,off,tcnt;
   int zcnt,hd[HDSZ],xtr;
   unsigned int byt;

   if(!quiet)
      msgout("  Pavloda");
   

   for(i=20; i<tap.len-8; i++)
   {
      if(!is_pause_param(i) && (int)tap.tmem[i]>ft[PAV].lp-tol && (int)tap.tmem[i]<ft[PAV].lp+tol) /* need sequence of 100+ bit 0's */
      {
         sof= i;
         zcnt= 0;
         while((int)tap.tmem[i]>ft[PAV].lp-tol && (int)tap.tmem[i]<ft[PAV].lp+tol)
         {
            i++;
            zcnt++;
         }
         if(zcnt>500 && (int)tap.tmem[i]>ft[PAV].sp-tol && (int)tap.tmem[i]<ft[PAV].sp+tol)
         {
            sod= i+2;  /* note +2 skip the bit1 sync (double SHORT pulse). */

            /* decode the header, so we can validate the addresses... */
            for(off=0,tcnt=0; tcnt<HDSZ; tcnt++)
            {
               xtr=0;
               byt= pav_readbyte(sod+off);

               if(byt==-1 && !quiet)
               {
                  sprintf(lin,"\n * Read error in PAVLODA header ($%04X), search abandoned.",sod+off);
                  msgout(lin);
                  return;
               }
               if(byt!=-1)
               {
                  xtr = (byt&0xFF00)>>8;
                  hd[tcnt] = byt&0xFF;
               }
               off+=(8+xtr);
            }

            s = hd[0]+ (hd[1]<<8);   /* get start address */
            e = hd[2]+ (hd[3]<<8);   /* get end address */

            if(e>s)
            {
               x = e-s;

               /* read all data bytes to locate end offset... */
               for(off=0,tcnt=0; tcnt<x+HDSZ+1; tcnt++)
               {
                  xtr=0;
                  byt = pav_readbyte(sod+off);
                  if(byt!=-1)
                     xtr = (byt&0xFF00)>>8;
                  off+=(8+xtr);  /* add size of previous byte to off. */
               }
               off-=(8+xtr);  /* step back to first pulse of checksum */

               eod = sod+off;
               eof = eod + 8+xtr-1;

               addblockdef(PAV, sof,sod,eod,eof, 0);

               i = eof;  /* optimize search */
            }
         }
      }
   }
}
/*---------------------------------------------------------------------------
*/
int pav_describe(int row)
{
   int i,s,off;
   int b,hd[HDSZ],xtr,rd_err,cb;
    
   /* decode the header, so we can validate the addresses... */
   s= blk[row]->p2;
   for(off=0,i=0; i<HDSZ; i++)
   {
      xtr=0;
      b= pav_readbyte(s+off);
      if(b!=-1)
      {
         xtr= (b&0xFF00)>>8;
         hd[i]= b&=0xFF;
      }
      off+=(8+xtr);
   }

   blk[row]->cs= hd[0]+ (hd[1]<<8);
   blk[row]->ce= hd[2]+ (hd[3]<<8);
   blk[row]->cx= blk[row]->ce - blk[row]->cs;

   /* get pilot length... */
   blk[row]->pilot_len= (blk[row]->p2 - blk[row]->p1 -8);
   blk[row]->trail_len=0;

   /* extract data and test checksum... */
   /* note : 'off' is in position from previous header read */
   rd_err=0;
   cb=0;
   s= blk[row]->p2;

   if(blk[row]->dd!=NULL)
      free(blk[row]->dd);
   blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);

   for(i=0; i<blk[row]->cx; i++)
   {
      b= pav_readbyte(s+off);
      if(b!=-1)
      {
         xtr = (b&0xFF00)>>8;
         blk[row]->dd[i]= b&0xFF;
         cb+=(b&0xFF)+1;
         cb&=0xFF;
      }
      else
         rd_err++;

      off+=(8+xtr);  /* add size of previous byte to off. */
   }
   b= pav_readbyte(s+off) & 0xFF;   /* read actual checkbyte */
   blk[row]->cs_exp= cb;
   blk[row]->cs_act= b;
   blk[row]->rd_err= rd_err;      
   return 0;
}

