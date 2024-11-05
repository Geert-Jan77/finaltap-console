/*---------------------------------------------------------------------------
  turrican.c

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

#define HDSZ 193

/*---------------------------------------------------------------------------
*/
void turrican_search(void)
{
   int i,sof,sod,eod,eof;
   int z,byt,hd[HDSZ],tmp,j;
   static int _dfs=0,_dfe=0;

   if(!quiet)
      msgout("  Turrican loader");
         

   for(i=20; i<tap.len-8; i++)
   {
      if((z=find_pilot(i,TURR_HEAD))>0)
      {
         sof=i;
         i=z;
         if(readttbyte(i, ft[TURR_HEAD].lp, ft[TURR_HEAD].sp, ft[TURR_HEAD].tp, ft[TURR_HEAD].en)==ft[TURR_HEAD].sv)
         {
            /* check sync sequence.. */
            for(j=0, tmp=0x0C; tmp>0x01 && j<100; j++)
            {
               byt = readttbyte(i+(j*8), ft[TURR_HEAD].lp, ft[TURR_HEAD].sp, ft[TURR_HEAD].tp, ft[TURR_HEAD].en);
               if(byt==tmp)
                  tmp--;
            }

            if(tmp==1)  /* the sync sequence is correct... */
            {
               sod=(i+(j*8)+8);  /* sod points to ID byte */
               i=sod;

               /* is it header or data block?... */
               byt = readttbyte(sod, ft[TURR_HEAD].lp, ft[TURR_HEAD].sp, ft[TURR_HEAD].tp, ft[TURR_HEAD].en);
               if(byt==0x02)  /* its a header block... */
               {
                  eod = sod+(HDSZ*8)-8;
                  eof= eod+7;

                  /* extract and save the addresses... */
                  for(j=0; j<5; j++)
                     hd[j] = readttbyte(sod+(j*8), ft[TURR_HEAD].lp, ft[TURR_HEAD].sp, ft[TURR_HEAD].tp, ft[TURR_HEAD].en);

                  _dfs = hd[1]+(hd[2]<<8);
                  _dfe = hd[3]+(hd[4]<<8);
                  if(_dfe>_dfs)
                     addblockdef(TURR_HEAD, sof,sod,eod,eof, 0);
                  i=eof;  /* optimize search */
               }
               if(byt==0x00)  /* its a data block... */
               {
                  if(_dfs!=0 && _dfe!=0) /* header has already been found?... */
                  {
                     eod = sod+ ((_dfe-_dfs)*8)+8;
                     eof= eod+7;

                     /* bump eof through any BIT 0's (trailer)... */
                     while(tap.tmem[eof+1]>ft[TURR_HEAD].sp-tol && tap.tmem[eof+1]<ft[TURR_HEAD].sp+tol)
                        eof++;

                     addblockdef(TURR_DATA, sof,sod,eod,eof, 0);
                     i = eof;  /* optimize search */
                  }
               }
            }
         }
      }
   }
}
/*---------------------------------------------------------------------------
*/
int turrican_describe(int row)
{
   int i,j,s,b,hd[HDSZ],rd_err,cb;
   static int _dfs=0,_dfe=0;
   char fn[256],str[2000];
   
   s= blk[row]->p2;

   if(blk[row]->lt==TURR_HEAD)
   {
      /* compute C64 start address, end address and size... */
      blk[row]->cs= 0;
      blk[row]->cx= 193;
      blk[row]->ce= blk[row]->cs + blk[row]->cx;

      /* decode header... */
      for(i=0; i<33; i++)
         hd[i] = readttbyte(s+(i*8), ft[TURR_HEAD].lp, ft[TURR_HEAD].sp, ft[TURR_HEAD].tp, ft[TURR_HEAD].en);

      _dfs = hd[1]+(hd[2]<<8);  /* get load address of DATA block */
      _dfe = hd[3]+(hd[4]<<8);  /* get end address of DATA block */

      sprintf(lin,"\n - File ID : $%02X", hd[0]);
      strcat(info,lin);
      sprintf(lin,"\n - DATA FILE Load address : $%04X",_dfs);
      strcat(info,lin);
      sprintf(lin,"\n - DATA FILE End address : $%04X",_dfe);
      strcat(info,lin);

      /* get filename... */
      for(i=0,j=0; i<27; i++)
         fn[j++]= hd[i+6];
      fn[j]=0;
      trimstring(fn);
      pet2text(str,fn);

      if(blk[row]->fn!=NULL)
         free(blk[row]->fn);
      blk[row]->fn = (char*)malloc(strlen(str)+1);

      strcpy(blk[row]->fn, str);
      
      /* get pilot & trailer lengths.. */
      blk[row]->pilot_len= ((blk[row]->p2 - blk[row]->p1)>>3)-12;
      blk[row]->trail_len= 0;

      /* TODO.. extract as PRG... */
   }
   /*------------------------------------------------------------------------------*/
   if(blk[row]->lt==TURR_DATA)
   {
      /* compute C64 start address, end address and size... */
      blk[row]->cs = _dfs;           /* (recalled from previous header) */
      blk[row]->ce = _dfe -1;
      blk[row]->cx = (_dfe - _dfs)+1;

      hd[0] = readttbyte(s, ft[TURR_HEAD].lp, ft[TURR_HEAD].sp, ft[TURR_HEAD].tp, ft[TURR_HEAD].en);
      sprintf(lin,"\n - File ID : $%02X", hd[0]);
      strcat(info,lin);

      /* get pilot & trailer lengths.. */
      blk[row]->pilot_len= ((blk[row]->p2 - blk[row]->p1)>>3)-12;
      blk[row]->trail_len= (blk[row]->p4 - blk[row]->p3)-7;

      /* extract data and test checksum... */
      rd_err=0;
      cb=0;
      s= blk[row]->p2+8;  /* +8 skips ID byte */

      if(blk[row]->dd!=NULL)
         free(blk[row]->dd);
      blk[row]->dd = (unsigned char*)malloc(blk[row]->cx);
      
      for(i=0; i<blk[row]->cx; i++)
      {
         b= readttbyte(s+(i*8), ft[TURR_HEAD].lp, ft[TURR_HEAD].sp, ft[TURR_HEAD].tp, ft[TURR_HEAD].en);
         cb^=b;
         if(b==-1)
            rd_err++;
         blk[row]->dd[i]=b;
      }
      b= readttbyte(s+(i*8), ft[TURR_HEAD].lp, ft[TURR_HEAD].sp, ft[TURR_HEAD].tp, ft[TURR_HEAD].en); /* read actual cb. */
      blk[row]->cs_exp= cb &0xFF;
      blk[row]->cs_act= b;
      blk[row]->rd_err= rd_err;
   }
   return 0;
}

