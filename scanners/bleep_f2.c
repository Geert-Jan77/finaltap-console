/*---------------------------------------------------------------------------
  bleep_f2.c (Bleepload Special)
  
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

   Pilot byte values vary across tapes, therefore this code has to search for
   ALL possible values when locating blocks.

---------------------------------------------------------------------------*/

#include "../mydefs.h"
#include "../main.h"
    
/*---------------------------------------------------------------------------*/
void bleep_spc_search(void)
{
   int cnt,cnt2,i,sof,sod,eod,eof;
   int k,pi=0,start,zcnt,len;
   int plt,b,tmp, dr=0;
   unsigned char done;
   unsigned char p[100]; /* holds a list of "bleep special" pb's found in the tap */

   if(!quiet)
      msgout("  Bleepload Special");
   
   plt=0x0;  /* set inital pilot value. */
   /* (park patrol=$2A, thrust=$50, iball=$47)  */
   
   /* search for a valid pilot sequence using ANY pb value...  */
   for(cnt=0; cnt<tap.len-8; cnt++)
   {
      b=readttbyte(cnt, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, MSbF);

      if(b!=-1 && b!=0x00 && b!=0xFF)
      {
         i=0;
         cnt2= cnt; /* preserve original counter.  */
         do
         {
            tmp= readttbyte(cnt2, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, MSbF);
            cnt2+=8;
            i++;
         }
         while(tmp==b);
         cnt2-=8;

         if(i==10)  /* found pb*10...  */
         {
            if(readttbyte(cnt2, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, MSbF)==(b^0xFF))  /* if pb XOR $FF is correct...  */
            {
               if(readttbyte(cnt2+8, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, MSbF)==b)  /* if last pb is correct...  */
               {
                  if(readttbyte(cnt2+16, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, MSbF)==0)  /* if first byte is 0... */
                  {
                     plt= b;   /* FOUND correct bleep special pilot for this tap!. */
                  }
               }
            }
         }
      }
      if(plt!=0)
      {
         p[pi++]=plt;  /* record this "bleepload special" pb.  */
         plt=0;
         cnt=cnt2;
      }
   }

   if(pi==0)
      p[pi++]=0x47;  /* insert a default pb if we didnt find any ($47=Thrust) */


   /* now scan the tap for bleepload special chains using each of the pilot
   values we discovered above...   */


   k=0;
   do
   {
      plt= p[k];   /* guaranteed to be at least 1 pb value available.  */

      for(cnt=20; cnt<tap.len-8 && dr!=DBFULL; cnt++)
      {
         b= readttbyte(cnt, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, MSbF);
         if(b==plt)
         {
            start= cnt;
            /* trace "start" back to start of L pulse leader... */
            if(tap.tmem[start-1]>ft[BLEEP].lp-tol && tap.tmem[start-1]<ft[BLEEP].lp+tol)
            {
               while(tap.tmem[start-1]>ft[BLEEP].lp-tol && tap.tmem[start-1]<ft[BLEEP].lp+tol && !is_pause_param(start-1))
                  start--;
            }

            zcnt=0;
            while(readttbyte(cnt, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, MSbF)==plt)
            {
               cnt+=8;
               zcnt++;
            }
            if(zcnt>8 && readttbyte(cnt, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, MSbF)==(plt^0xFF))  /* ending with a sync.  */
            {
               if(readttbyte(cnt+8, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, MSbF)==plt) /* after sync is one more pilot */
               {
                  cnt+=16;

                  tmp=0; /* look for block number 0 initially  */
                  do
                  {
                     if(readttbyte(cnt, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, MSbF)==tmp) /* block num is correct. */
                     {
                        if(tmp==0)
                           sof = start;  /* use start of leader if 1st block.  */
                        else
                           sof= cnt-(12*8);
                        sod= cnt;
                        len= 256+8;  /* +8 accounts for checkbyte  */
                        eod= sod+(len*8)-8;
                        eof= eod+7;

                        /* override 'eof' if there is a trailer tone...  */
                        if(readttbyte(eof+1, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, MSbF)==0xFF)
                        {
                           while(tap.tmem[eof+1]>ft[BLEEP].lp-tol && tap.tmem[eof+1]<ft[BLEEP].lp+tol)
                              eof++;
                        }

                        dr= addblockdef(BLEEP_SPC, sof,sod,eod,eof, 0);
                        tmp++; /* bump to look for next block number..  */
                        cnt+=2208;
                        done=0;
                     }
                     else
                        done=1;  /* failed to find consecutive block number. */
                  }
                  while(!done && dr!=DBFULL);
               }
            }
         }
      }
      k++;
   }
   while(k<pi && dr!=DBFULL);
}
/*---------------------------------------------------------------------------*/
int bleep_spc_describe(int row)
{
   int s,i;
   int hd[8],b,rd_err,cb;

   s= blk[row]->p2;  /* get first data offset   */

   for(i=0; i<8; i++)
      hd[i]= readttbyte(s+(i*8), ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, MSbF);

   sprintf(lin,"\n - Block number : $%02X", hd[0]);
   strcat(info,lin);

   blk[row]->cs= hd[1]+(hd[2]<<8);
   blk[row]->cx= 256;
   blk[row]->ce= ((blk[row]->cs + blk[row]->cx)-1);

   /* get execution address...   */
   sprintf(lin,"\n - Execution address : $%04X", hd[5]+(hd[6]<<8));
   strcat(info,lin);

   /* get pilot length... */
   blk[row]->pilot_len = ((blk[row]->p2 - blk[row]->p1)/8) -2;
   blk[row]->trail_len =0;

   /* extract data and test checksum...  */
   rd_err=0;
   cb=0;
   s= (blk[row]->p2)+(7*8);

   if(blk[row]->dd!=NULL)
      free(blk[row]->dd);
   blk[row]->dd= (unsigned char*)malloc(blk[row]->cx);

   for(i=0; i<blk[row]->cx; i++)
   {
      b= readttbyte(s+(i*8), ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, MSbF);
      cb^=b;
      if(b==-1)
         rd_err++;
      blk[row]->dd[i]= b;
   }
   b= readttbyte(s+(i*8), ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, MSbF); /* read checkbyte  */

   blk[row]->cs_exp= cb &0xFF;
   blk[row]->cs_act= b;
   blk[row]->rd_err= rd_err;
   return 0;
}

