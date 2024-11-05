/*---------------------------------------------------------------------------
  visiload.c

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
   
  This is a slightly tricky format, no file in the chain can be found in isolation
  apart from the 1st file as each files formatting is determined by info held in
  its predecessor.

  The scanner considers the job "finished" when read errors are found in a header
  or when the offset gets beyond 'len-300'. (this could be improved!).

30/8/2002...

  Problems with certain Visiload TAPs crashing FT are cured by the
  addition of error checking during header decodes, however I have found
  Visiload TAPs that contain "pilot-less" blocks that do NOT immediately
  begin with the header!!!. It was THIS that caused many crashes, now this
  is just causing abandonment of Visiload searches early on several TAPs
  and I need to study the visiload loader itself to see how IT is
  handling these cases.
  ie. Dizzy - Ultimate Cartoon Adventure.

  Anyway, its good to know its not a pulsewidth problem as I first
  suspected.


11/2/2002...

 Often the search is abandoned because unexpected pauses are encountered,
 I fixed today, "the captive", it contained many pauses where there should
 have been LONG pulses.

  
visiload thresholds...

t1. CRC $001CDFBE  :  threshold = $1B6 (TAP byte $35)
t2. CRC $001CE3DE  :  threshold = $1E6 (TAP byte $3B)
t3. CRC $001CE56A  :  threshold = $1F8 (TAP byte $3E)
t4. CRC $001CD5F7  :  threshold = $243 (TAP byte $47)


IMPORTANT : -

'visi_type' is a global defined in the main file, its default value (VISI_T2) is
over-ridden when the CBM program is ID'd as being whichever of the 4 types.

---------------------------------------------------------------------------*/

#include "../main.h" 
#include "../mydefs.h"

#define HDSZ 4

/*---------------------------------------------------------------------------
 Reads a Visiload format byte at 'pos' in tap.tmem,
 'abits' specifies a number of additional (1) bits that MUST precede the byte
*/
int visiload_readbyte(int pos, int endi,int abits)
{
   int b,valid;
   long i;
   unsigned char mb[8],p;
   
   /* check that all required bits are inside the tap and not inside any pauses... */
   for(i=0; i<(8+ abits); i++)
   {
      if(pos+i<20 || pos+i>tap.len-1)
         return -1;
      if(is_pause_param(pos+i))
         return -1;
   }

   /* read requires additional (1) bits, return -1 if they arent there. */
   for(i=0; i<abits; i++)
   {
      p = tap.tmem[pos+i];
      if(p<(ft[visi_type].lp-tol) || p>(ft[visi_type].lp+tol))  /* not valid 1 bit? */
      {
         add_read_error(pos+i);  /* read error, visiload 'addbits' failed. */
         return -1;
      }
   }

   /* read next 8 into mb[] */
   for(i=0; i<8; i++)
   {
      valid=0;

      p = tap.tmem[pos+abits+i];
      if(p>(ft[visi_type].lp-tol) && p<(ft[visi_type].lp+tol))  /* valid 1 bit? */
      {
         mb[i]=1;
         valid++;
      }
      if(p>(ft[visi_type].sp-tol) && p<(ft[visi_type].sp+tol))  /* valid 0 bit? */
      {
         mb[i]=0;
         valid++;
      }
      if(valid==0) /* not 1 or 0? */
      {
         add_read_error(pos+i);
         return -1;
      }
      if(valid==2) /* pulse qualified as 0 AND 1? */
      {
         add_read_error(pos+i);
         return -1;
      }
   }

   /* we may assume now that mb[8] contains 8 valid bits... */

   if(endi==MSbF)     /* decode them MSbF... */
   {
      b = 0;
      for(i=0; i<8; i++)
      {
         if(mb[i]==1)
            b+=(128>>i);
      }
   }
   else               /* decode them LSbF... */
   {
      b = 0;
      for(i=0; i<8; i++)
      {
         if(mb[i]==1)
            b+=(1<<i);
      }
   }
   return b;
}
/*---------------------------------------------------------------------------*/
void visiload_search(void)
{
   int i,sof,sod,eod,eof,zcnt;
   int j,start,end,len;
   int b,att,ah,ab,en,pt,db1;
   int hd[200],hcnt;
   
   if(!quiet)
      msgout("  Visiload");
         

   ah=0;    /* additional header bytes. (initial) */
   ab=1;    /* additional bits per byte. (initial) */
   en=MSbF; /* endianess. (intial) */

   for(i=20; i<tap.len-100; i++)
   {
      /* note: first block always has pilot, MSbF endianess, 1 additional bit per byte. */

      b= visiload_readbyte(i,en,ab);   /* look for a pilot byte... */
      if(b== ft[visi_type].pv)
      {
         sof= i;
         zcnt=0;
         while(visiload_readbyte(i+(zcnt*(8+ab)), en, ab)==ft[visi_type].pv) /* step thru all pilot bytes.. */
            zcnt++;

         b= visiload_readbyte(i+(zcnt*(8+ab)), en, ab); /* should be a sync */

         if(b==ft[visi_type].sv && zcnt>ft[visi_type].pmin)    /* got enough pilots + sync?... */
         {
            /* Application->MessageBox("at least 100 visi pilots +sync found","",MB_OK);   ok, appears just once. */

            sod= i+((zcnt+1)*(8+ab));    /* set start of data on byte after sync. */
            j= sod;
            pt=0;

            /* Found 1st block, now find all others... */
            do
            {
               if(pt==1)  /* pilot required?, read in pilot tone and sync if required... */
               {
                  do  /* find a first pilot byte... */
                  {
                     b= visiload_readbyte(j,en,ab);
                     j++;
                  }
                  while(b!=ft[visi_type].pv && j<tap.len-100);
                  j--;
                  sof= j;
                  do  /* read thru all pilot bytes... */
                  {
                     b= visiload_readbyte(j,en,ab);
                     j+=(8+ab);
                  }
                  while(b==ft[visi_type].pv && j<tap.len-100);
                  j-=(8+ab);

                  /* ! should check pilot count here?, theres usually only 2 or 3 pilots ! */

                  /* check sync byte... */
                  b= visiload_readbyte(j, en, ab);
                  if(b!=ft[visi_type].sv)
                  {
                     if(!quiet)
                     {
                        sprintf(lin," * Visiload sync byte failed @ %04X, search aborted.", j);
                        msgout(lin);
                     }
                     return;   /* abort search if sync fails. */
                  }
                  sod= j+(8+ab);
                  pt=0;
               }

               /* pilot sync are found or none were required... */

               /* decode header... (taking into account additional header bytes) */
               for(hcnt=0; hcnt<HDSZ+ah+1; hcnt++)  /* note: +1 reads 1 data byte too. */
               {
                  hd[hcnt]= visiload_readbyte(sod+(hcnt*(8+ab)), en, ab);

                  if(hd[hcnt]==-1)   /* 27/8/2002 : this may stop the crashes */
                  {
                     if(!quiet)
                     {
                        sprintf(lin,"\nFATAL : read error in Visiload header! ($%04X).",j+(hcnt*(8+ab)));
                        msgout(lin);
                        sprintf(lin,"header begins at $%04X and should hold %d bytes.",j,HDSZ+ah);
                        msgout(lin);

                        sprintf(lin,"\nVisiload search was aborted.");
                        msgout(lin);
                        sprintf(lin,"Experts note : Manual correction of this location should allow detection of more Visiload files. ");
                        msgout(lin);
                        sprintf(lin,"Typically the pulse found there will be too large to register as a Visiload LONG or the data-stream may ");
                        msgout(lin);
                        sprintf(lin,"contain an unexpected pause.");
                        msgout(lin);
                        msgout("");
                     }
                     return;
                  }
               }

               start= (hd[2+ah]<<8) + hd[3+ah];  /* start address is at offsets 2,3 */
               end= (hd[0+ah]<<8) + hd[1+ah];    /* end address is at offsets 0,1 */
               len= end-start;
               if(len==0)
                  len=1;  /* NOTE: if START==END then 1 byte will STILL be sent! */

               db1= hd[HDSZ+ah];    /* read first data byte. */
               
               eod= sod+(len+4+ah-1)*(8+ab);   /* same thing as above. */
               eof= eod+(8+ab)-1;

             /* create an attribute value for this block...
                att value...
                bit 7: endianess (1=msbf)
                bit 6: unused.
                bit 5: additional header bytes, bit 2
                bit 4: additional header bytes, bit 1
                bit 3: additional header bytes, bit 0
                bit 2: additional bits per byte, bit 2
                bit 1: additional bits per byte, bit 1
                bit 0: additional bits per byte, bit 0

                note : maximum ah and ab is 7.   */

               att= (en<<7)+(ah<<3)+ab;
               addblockdef(visi_type, sof,sod,eod,eof,att);

               j= eof+1;   /* very important to set this correctly  */
                           /* next header will be decoded immediately */
                           /* on next iteration. (unless block has pilot) */

               i= j;     /* optimize search (theres usually only 1 chain though). */
                
               sof= j;   /* set these ready for next block */
               sod= j;

               /* set formatting parameters for next block... */
               if(start==0x034B)
                  ab= db1-8;   /* first data byte holds no.of bits per byte for next block */
               if(start==0x03A4)
                  ah= db1-3;   /* first data byte holds no.of add header bytes +3 for next block */

               if(start==0x0347) /* if First DATA byte is $26 then next block will be MSbF else LSbF */
               {
                  if(db1==0x26)
                     en= MSbF;
                  else
                     en= LSbF;
               }
               if(start==0x03BB) /* Next block WILL have PILOT tone before it (ANY number , can be 1 !) */
                  pt=1;
            }
            while(j<tap.len-100);

         }
      }
   }
}
/*---------------------------------------------------------------------------
*/
int visiload_describe(int row)
{
   int i,s,xsb,ab,ah;
   int hd[HDSZ+1];
   int b,rd_err;
   char endian[5];

   xsb =(blk[row]->xi & 128)>>7;    /* get endianess. */
   ah = (blk[row]->xi & 56)>>3;     /* get no. additional header bytes. */
   ab = (blk[row]->xi & 7);         /* and no. additional bits per byte. */

   /* decode the header... */
   s = blk[row]->p2;
   for(i=ah; i<HDSZ+ah+1; i++)
      hd[i-ah] = visiload_readbyte(s+(i*(8+ab)), xsb, ab);

   /* compute C64 start address, end address and size... */
   blk[row]->cs = (hd[2]<<8)+ hd[3];
   blk[row]->ce = ((hd[0]<<8)+ hd[1])-1;
   blk[row]->cx = ((blk[row]->ce - blk[row]->cs)+1) & 0xFFFF;

   strcpy(lin,"");
   switch(blk[row]->cs)
   {
      case(0x034B) :
         strcat(lin,"\n - MODIFIER : first data byte holds no. of bits per byte in next block."); break;
      case(0x03A4) :
         strcat(lin,"\n - MODIFIER : first data byte is number of additional header bytes+3 in next."); break;
      case(0x0347) :
         strcat(lin,"\n - MODIFIER : if first data byte is $26 then next block will be MSbF else LSbF."); break;
      case(0x03BB) :
         strcat(lin,"\n - MODIFIER : next block (only) will have PILOT tone before it + possibly a pause."); break;
      default :
         strcat(lin,"\n - Next block will be formatted same as this one.");
   }
   strcat(info,lin);

   /* show the first data byte if its useful... */
   if(blk[row]->cs==0x034B || blk[row]->cs==0x03A4 || blk[row]->cs==0x0347)
   {
      sprintf(lin,"\n - First byte $%02X",hd[4]);
      strcat(info,lin);
   }
   /* print block format info...*/
   if(xsb==1)
      strcpy(endian,"MSbF");
   else
      strcpy(endian,"LSbF");
   sprintf(lin,"\n - Bits per byte: %d | Endianess: %s | Extra headers bytes: %d",8+ab, endian, ah);
   strcat(info,lin);

   /* get pilot & trailer lengths... */
   blk[row]->pilot_len = (blk[row]->p2 - blk[row]->p1) / (8+ab);
   if(blk[row]->pilot_len>0)
      blk[row]->pilot_len--;  /* discount sync if there is a pilot.*/
   blk[row]->trail_len= 0;


   /* extract data... */

   /* optionally dont decode "modifier" blocks...
     any block that loads to $03xx is ignored. */
   if(extvisipatch==FALSE && (blk[row]->cs & 0xFF00) == 0x0300)
      return(0);

   rd_err=0;
   s= (blk[row]->p2)+((ah+HDSZ)*(8+ab));

   if(blk[row]->dd!=NULL)
      free(blk[row]->dd);
   blk[row]->dd= (unsigned char*)malloc(blk[row]->cx);
   
   for(i=0; i< blk[row]->cx; i++)
   {
      b= visiload_readbyte(s+(i*(8+ab)), xsb,ab);
      if(b==-1)
         rd_err++;
      blk[row]->dd[i]= b;
   }
   blk[row]->rd_err= rd_err;
   return 0;
}


