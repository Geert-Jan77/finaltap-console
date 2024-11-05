/*---------------------------------------------------------------------------
  burner.c
  
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

/*---------------------------------------------------------------------------*/
void burner_search(void)
{
   int i,j,z,sof,sod,eod,eof;
   int hd[HDSZ],x,ib;
 
   if(!quiet)
      msgout("  Burner");
     
   /* first we retrieve the burner variables from the CBM header... */

   ib= find_decode_block(CBM_HEAD, 1);
   if(ib==-1)
      return;    /* failed to locate cbm header. */

   ft[BURNER].pv= blk[ib]->dd[0x88] ^ 0x59;
   ft[BURNER].sv= blk[ib]->dd[0x93] ^ 0x59;
   ft[BURNER].en= blk[ib]->dd[0x83] ^ 0x59;  /* MSbF rol ($26) or.. LSbF ror($66)  */

   if(ft[BURNER].en!=0x26 && ft[BURNER].en!=0x66)  /* skip search if endianess check failed. */
      return;

   if(ft[BURNER].en==0x26)
      ft[BURNER].en= MSbF;
   if(ft[BURNER].en==0x66)
      ft[BURNER].en= LSbF;

   sprintf(lin,"Burner variables found and set: pv=$%02X, sv=$%02X, en=%d", ft[BURNER].pv,ft[BURNER].sv,ft[BURNER].en);
   msgout(lin);

   /*------------------------------------------------------------------------------- */

   for(i=20; i<tap.len-8; i++)
   {
      if((z=find_pilot(i,BURNER))>0)
      {
         sof=i;
         i=z;
         if(readttbyte(i, ft[BURNER].lp, ft[BURNER].sp, ft[BURNER].tp, ft[BURNER].en)==ft[BURNER].sv)
         {
            sod= i+8;

            /* decode the header, so we can validate the addresses */
            for(j=0; j<HDSZ; j++)
               hd[j] = readttbyte(sod+(j*8), ft[BURNER].lp, ft[BURNER].sp, ft[BURNER].tp, ft[BURNER].en);

            x= (hd[2]+(hd[3]<<8)) - (hd[0]+ (hd[1]<<8));  /* get data length */
            if(x>0)
            {
               eod= sod+ ((x+HDSZ)*8)-8;
               eof= eod+7;
               addblockdef(BURNER, sof,sod,eod,eof, ft[BURNER].pv+ (ft[BURNER].sv<<8)+ (ft[BURNER].en<<16));
               i= eof;  /* optimize search  */
            }
         }
      }
      else
      {
         if(z<0)    /* find_pilot failed (too few/many), set i to failure point.  */
            i=(-z);
      }
   }
}
/*---------------------------------------------------------------------------*/
int burner_describe(int row)
{
   int i,s,hd[HDSZ],b,rd_err,endi;
   char endiname[5];

   endi= (blk[row]->xi & 0xFF0000)>>16;

   if(endi!=LSbF && endi!=MSbF)  /* skip search if endianess check failed.  */
      return 0;

   if(endi==MSbF)
      strcpy(endiname,"MSbF");
   else
      strcpy(endiname,"LSbF");

   /* decode the header...  */
   s= blk[row]->p2;
   for(i=0; i<HDSZ; i++)
      hd[i] = readttbyte(s+(i*8), ft[BURNER].lp, ft[BURNER].sp, ft[BURNER].tp, endi);

   blk[row]->cs= hd[0]+ (hd[1]<<8);
   blk[row]->ce= hd[2]+ (hd[3]<<8)-1;
   blk[row]->cx= (blk[row]->ce - blk[row]->cs)+1;  

   sprintf(lin,"\n - Pilot: $%02X, Sync: $%02X, Endianess : %s",blk[row]->xi&0xFF, (blk[row]->xi&0xFF00)>>8, endiname);
   strcat(info,lin);
   
   /* get pilot and trailer lengths  */
   blk[row]->pilot_len= ((blk[row]->p2 - blk[row]->p1 -8)>>3)-1;
   blk[row]->trail_len= (blk[row]->p4 - blk[row]->p3 -7)>>3;

   /* extract data...  */
   rd_err=0;
   s= (blk[row]->p2)+(HDSZ*8);

   if(blk[row]->dd!=NULL)
      free(blk[row]->dd);
   blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);

   for(i=0; i<blk[row]->cx; i++)
   {
      b= readttbyte(s+(i*8), ft[BURNER].lp, ft[BURNER].sp, ft[BURNER].tp, endi);
      if(b==-1)
         rd_err++;
      blk[row]->dd[i]=b;
   }
   blk[row]->rd_err= rd_err;
   return 0;
}



