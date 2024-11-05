/*---------------------------------------------------------------------------
  chr.c (CHR Tape)

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

  Notes...

  Handles all 3 threshold types, CHR T1, CHR T2, CHR T3
  (aka. Cauldron, Hewson, Rainbird)

---------------------------------------------------------------------------*/

#include "../mydefs.h"
#include "../main.h"

#define HDSZ 10

/*---------------------------------------------------------------------------*/
void chr_search(void)
{
   int i,j,z,sof,sod,eod,eof;
   int x,hd[HDSZ];
   int en,tp,sp,lp,sv,ld,pass=1;
   char lname[3][3]= {"T1","T2","T3"};

   do
   {
      if(pass==1)  {ld= CHR_T1;}  /* Cauldron  */
      if(pass==2)  {ld= CHR_T2;}  /* Hewson    */
      if(pass==3)  {ld= CHR_T3;}  /* Rainbird  */

      en= ft[ld].en;
      tp= ft[ld].tp;
      sp= ft[ld].sp;
      lp= ft[ld].lp;
      sv= ft[ld].sv;

      if(!quiet)
      {
         sprintf(lin,"  CHR Loader %s",lname[pass-1]);
         msgout(lin);
      }

      for(i=20; i<tap.len-100; i++)
      {
         if((z=find_pilot(i,ld))>0)
         {
            sof= i;
            i= z;
            if(readttbyte(i,lp,sp,tp,en)==sv)
            {
               /* look for sequence $64...$FF */
               j=0;
               while(readttbyte(i+(j*8),lp,sp,tp,en)==sv+j)
                  j++;

               if(j==156)  /* whole sequence (156 bytes) exists?  */
               {
                  sod= i+(157*8);

                  /* now just just trace back from sof through any $20 bytes (pre-leader) */
                  if(readttbyte(sof-8, lp,sp,tp,en)==0x20)
                  {
                     while(readttbyte(sof-8, lp,sp,tp,en)==0x20)
                        sof-=8;
                  }

                  /* to find the last pulse, we look in the header for the start/end addresses... */
                  for(j=0; j<HDSZ; j++)   /* decode the header...  */
                     hd[j] = readttbyte(sod+(j*8), lp,sp,tp,en);

                  /* find block length */
                  x= (hd[2]+ (hd[3]<<8)) - (hd[0]+ (hd[1]<<8)); /* end-start */

                  if(x>0)
                  {
                     eod= sod+(x*8)+80;
                     eof= eod+7;
                     addblockdef(ld, sof,sod,eod,eof, 0);
                     i= eof;  /* optimize search  */
                  }
               }
            }
         }
         else
         {
            if(z<0)    /* find_pilot failed (too few/many), set i to failure point.  */
               i=(-z);
         }
      }
      pass++;
   }
   while(pass<4);  /* run 3 times */
}
/*---------------------------------------------------------------------------*/
int chr_describe(int row)
{
   int i,s,b,hd[HDSZ],rd_err,cb;
   int en,tp,sp,lp;

   en= ft[CHR_T1].en;
  
   if(blk[row]->lt==CHR_T1)
      {sp=ft[CHR_T1].sp;  lp=ft[CHR_T1].lp;  tp=ft[CHR_T1].tp;}
   if(blk[row]->lt==CHR_T2)
      {sp=ft[CHR_T2].sp;  lp=ft[CHR_T2].lp;  tp=ft[CHR_T2].tp;}
   if(blk[row]->lt==CHR_T3)
      {sp=ft[CHR_T3].sp;  lp=ft[CHR_T3].lp;  tp=ft[CHR_T3].tp;}

   /* decode the header... */
   s= blk[row]->p2;
   for(i=0; i<HDSZ; i++)
      hd[i]= readttbyte(s+(i*8),lp,sp,tp,en);

   blk[row]->cs= hd[0]+ (hd[1]<<8);
   blk[row]->ce= hd[2]+ (hd[3]<<8);
   blk[row]->cx= blk[row]->ce - blk[row]->cs;

   /* get pilot & trailer lengths */
   blk[row]->pilot_len= (blk[row]->p2- blk[row]->p1) >>3;
   blk[row]->trail_len= (blk[row]->p4- blk[row]->p3 -7) >>3;

   /* extract data and test checksum... */
   rd_err= 0;
   cb= 0;
   s= (blk[row]->p2)+(HDSZ*8);

   if(blk[row]->dd!=NULL)
      free(blk[row]->dd);
   blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);
   
   for(i=0; i<blk[row]->cx; i++)
   {
      b= readttbyte(s+(i*8), lp,sp,tp,en);
      cb^=b;
      if(b==-1)
         rd_err++;
      blk[row]->dd[i]=b;
   }
   b= readttbyte(s+(i*8), lp,sp,tp,en);

   blk[row]->cs_exp= cb &0xFF;
   blk[row]->cs_act= b;
   blk[row]->rd_err= rd_err;
   return 0;
}

