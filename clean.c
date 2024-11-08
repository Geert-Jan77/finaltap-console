/*---------------------------------------------------------------------------
  clean.c (Tape conversion, repairing and cleaning)

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
   
   Most cleaning functions automatically cause analyze().

---------------------------------------------------------------------------*/

#include "main.h"
#include "mydefs.h"


/*---------------------------------------------------------------------------
 Make the current TAP header size == tap.len-20
*/
void fix_header_size(void)
{
   unsigned char b;
   b= (tap.len-20) & 0xFF;
   tap.tmem[16]= b;
   b= ((tap.len-20) & 0xFF00) >>8;
   tap.tmem[17]= b;
   b= ((tap.len-20) & 0xFF0000) >>16;
   tap.tmem[18]= b;
   b= ((tap.len-20) & 0xFF000000) >>24;
   tap.tmem[19]= b;
}
/*---------------------------------------------------------------------------
 Clips leading/trailing pauses from the loaded (v0) tap.
 If the TAP contains ALL pauses then no action is taken.
*/
void clip_ends(void)
{
   int i,j,sp,ep,s1,s2, sz;
   unsigned char b,cnt,*tmp;

   msgout("\nClipping leading/trailing pauses...");

   if(tap.version>0)  /* TAP must be v0 format */
   {
      msgout("\n  Clipping aborted, TAP is not v0.");
      return;
   }

   /* need at least 100 pulses in the file for this to work... */
   if(tap.len-20<100)
   {
      msgout("\n  Clipping aborted, TAP is too short.");
      return;
   }

   /* convert $FF pulses to $00 in v0 TAP files first.
    such pulses were found on some old TAP files probably not made with MTAP. */
   for(i=20; i<tap.len; i++)
   {
      if(tap.tmem[i]==0xFF)
         tap.tmem[i]=0;
   }

   /* Find clip point from start of tap... */
   for(i=20; i<tap.len-100; i++)
   {
      cnt=0;
      for(j=0; j<100; j++)   /* scan a 100 byte block  */
      {
         b= tap.tmem[i+j];
         if(b<LAME)     /* note: pause is any byte <0x15  (CIA Delta's lead pause uses 0x10) */
            cnt++;
      }
      if(cnt==0)   /* cnt==0 means the current stretch has no pause. */
         break;
   }
   sp=i;
    

   /* Find clip point from end of tap... */
   for(i=tap.len-100; i>20; i--)
   {
      cnt=0;
      for(j=0; j<100; j++)   /* scan a 100 byte block   */
      {
         b= tap.tmem[i+j];
         if(b<LAME)     /* note: pause is any byte <0x15  */
            cnt++;
      }
      if(cnt==0)   /* cnt==0 means the stretch has no pause.  */
         break;
   }
   ep= i+100;

   if(i==20)   /* if this is true then the whole file must be pause! */
   {
      sp= 20;        /* note: this doesnt cause any cutting, it preserves everything */
      ep= tap.len;   /* but the v1 converter will create a 17 sec pause 00,FF,FF,FF */
   }                 /* in place of a great many 00's anyhow. */
      
   s1= sp-20;
   s2= tap.len-ep;
   if(s1==0 && s2==0)
   {
      msgout("\n  No clipping required.");
      return;
   }

   sz= (ep-sp)+20;
   tap.len= sz;
   tmp= (unsigned char*)malloc(sz+1024);

   for(i=0; i<20; i++)           /* copy the original header.. */
      tmp[i]= tap.tmem[i];
   for(i=sp,j=20; i<ep+1; i++)   /* copy the clipped data area...   */
      tmp[j++]= tap.tmem[i];
   free(tap.tmem);
   tap.tmem= tmp;
   fix_header_size();
   tap.changed= 1;

   msgout("  Done.");
   analyze();
}
/*---------------------------------------------------------------------------
 Pause unifier, unifies consecutive pauses.
 on v0 TAPs this process permits some small amounts of noise removal.
*/
void unify_pauses(void)
{
   int i,j,z,tot,c,p,s,m;
   unsigned char *tmp;

   /*
    For v1 pauses. finds any consecutive ones (or groups) and computes the total
    cycle count of any found.
    a new "unified" pause is then created in their place.

    Note: v1 unifier never actually gets used!.
   */
   if(tap.version==1)
   {
      msgout("\nUnifying pauses (v1)...");

      tmp = (unsigned char*)malloc(tap.len+1024);  /* create a buffer for the rewrite. */
      for(i=0; i<20; i++)     /* copy the original TAP header.. */
         tmp[i] = tap.tmem[i];
     
      for(i=20,c=20; i<tap.len; i++)    /* c steps thru the 2nd buffer */
      {
         if(tap.tmem[i]!=0)        /* directly copy non pause bytes to the buffer. */
            tmp[c++] = tap.tmem[i];

         if(tap.tmem[i]==0 && i<(tap.len-4))  /* found a (v1) pause */
         {
            tot= tap.tmem[i+1] + (tap.tmem[i+2]<<8) + (tap.tmem[i+3]<<16);  /* get its length */

            for(j=1; tap.tmem[i+(j*4)]==0; j++)  /* count the consecutive ones... */
            ;
            if(j>1)   /* do we have more than 1 pause here?... */
            {
               /* printf("Found %d consecutive pauses at $%04X", j,i);   */

               for(j=1; tap.tmem[i+(j*4)]==0; j++)  /* add up all the cycles... */
                  tot+=  tap.tmem[i+(j*4)+1] + (tap.tmem[i+(j*4)+2]<<8) + (tap.tmem[i+(j*4)+3]<<16);
               /* printf("total cycles : %lu", tot);  */
            }

            i+=(j*4)-1; /* point i to end of pause chain, ready for next iteration.
                           note: this was moved to outside the above if()
                           so it works on single pauses too. doh!  */

            if(tot<16777216)  /* now write out the SINGLE unified pause to buffer... */
            {                 /* note: this writes out any single pauses too. */

               /* round cycle count to multiple of 20,000 cycles */
               m= tot%20000;
               tot-=m;
               if(m>10000)     /* round UP if needed. */
                  tot+=20000;
               if(tot<20000)   /* minimum pause is 20,000 cycles */
                  tot=20000;

               tmp[c]=0;
               tmp[c+1]=tot&0xFF;
               tmp[c+2]=(tot&0xFF00)>>8;
               tmp[c+3]=(tot&0xFF0000)>>16;
               c+=4;
            }
            else /* total is too large for a single unification. ( >24 bits or 17 secs) */
            {
               tmp[c]=0;       /* just write out a maximum... */
               tmp[c+1]=0xFF;
               tmp[c+2]=0xFF;
               tmp[c+3]=0xFF;
               c+=4;
            }
         }
      }
   }

/*  version 0 pause rebuilder...
    recalculates fragmented or normal pauses in v0 taps.
    removes "noise" patches and restructures each pause, accurately
    computing a final zero count. */

   if(tap.version==0)
   {
      msgout("\nUnifying pauses (v0)...");

      tmp= (unsigned char*)malloc(tap.len+1024);  /* create a buffer for the rewrite. */

      for(i=0; i<20; i++)     /* copy the original header.. */
         tmp[i]= tap.tmem[i];

      for(i=20,c=20; i<tap.len && c<tap.len; i++)
      {
         if(tap.tmem[i]>LAME)        /* dump non LAME bytes to the output buffer. */
            tmp[c++]= tap.tmem[i];

            
         if(tap.tmem[i]<LAME+1) /* found a v0 LAME byte... */
         {
            s=i; /* save location of first LAME. */

            do /* find nearest following block (of x bytes) with NO LAME bytes... */
            {
               z=0;   /* this goes to 1 if a LAME byte exists in block. */
               for(j=0; j<30 && (s+j)<tap.len; j++)
               {
                  if(tap.tmem[s+j]<LAME+1)
                     z=1;
               }
               s++;
            }
            while(z==1);
            s--;
            /* at this point we have the nearest block with no LAMEs.
               and 's' will be pointing at its first byte.  */


            /* now compute the output sequence for this pause block... */
            tot= 0;                 /* add up cycles in the whole stretch... */
            for(j=i; j<s; j++)
            {
               if(tap.tmem[j]==0)
                  tot+=20000;
               else
                  tot+= tap.tmem[j]*8;
            }

            /* now compute 0's required in pause... */
            p = (int)tot/20000;
            if(p==0)
               p=1;   /* make sure there is at least 1. */

            /* write out the computed 0's... */
            for(j=0; j<p; j++)
               tmp[c++] = 0;

            i=s-1;
         }
      }
   }

   tap.len=c;
   free(tap.tmem);
   tap.tmem = tmp;
   fix_header_size();
   tap.changed= 1;

   msgout("  Done.");
   analyze();
}
/*---------------------------------------------------------------------------
 The file cleaning routine.
*/
void clean_files(void)
{
   int i,j,t,s,e;
   int limit,b,k,sp,mp,lp,tp, owned;

   analyze();
   
   /* look at each block type and clean it accordingly...  */
   for(i=0; blk[i]->lt!=0; i++)
   {
      t = blk[i]->lt;   /* get block type and ideal pulses... */
      sp=ft[t].sp;
      mp=ft[t].mp;
      lp=ft[t].lp;
      tp=ft[t].tp;

      s= blk[i]->p1;   /* get block start  */
      e= blk[i]->p4;   /* get block end  */

      /* clean... */
      limit= tol+2;
      if(boostclean)
         limit+=10;

      /* clean... threshold is unavailable...
       dont try and clean gaps or pauses!  */
      if(t>2 && tp==NA)
      {
         sprintf(lin,"\nCleaning %s tape block from $%04X to $%04X...",ft[t].name,s,e);
         msgout(lin);

         for(k=2; k<limit; k++)   /* use progressive tolerance from 2 to limit.. */
         {
            for(j=s; j<e+1; j++)
            {
               b = tap.tmem[j];
               owned=0;
               if(sp!=0 && (b>(sp-k)) && (b<(sp+k)))   /* rewrite short pulse */
                  owned+=1;
               if(mp!=0 && (b>(mp-k)) && (b<(mp+k)))   /* rewrite medium pulse */
                  owned+=2;
               if(lp!=0 && (b>(lp-k)) && (b<(lp+k)))   /* rewrite long pulse */
                  owned+=4;

               /* decide on which "ideal" pulse to use... */
               if(owned==1)
                  tap.tmem[j]= sp;
               if(owned==2)
                  tap.tmem[j]= mp;
               if(owned==4)
                  tap.tmem[j]= lp;

               if(owned==3)     /* qualified as sp AND mp...choose closer... */
               {
                  if(abs(sp-b) < abs(mp-b))
                     tap.tmem[j] = sp;
                  if(abs(sp-b) > abs(mp-b))
                     tap.tmem[j] = mp;
                  /* note: no change if there is total ambiguity. (abs(sp-b) == abs(mp-b))  */
               }
               if(owned==6)     /* qualified as mp AND lp... choose closer... */
               {
                  if(abs(mp-b) < abs(lp-b))
                     tap.tmem[j] = mp;
                  if(abs(mp-b) > abs(lp-b))
                     tap.tmem[j] = lp;
                  /* note: no change if there is total ambiguity. (abs(mp-b) == abs(lp-b)) */
               }
                if(owned==5)     /* qualified as sp AND lp...choose closer... */
               {
                  if(abs(sp-b) < abs(lp-b))
                     tap.tmem[j] = sp;
                  if(abs(sp-b) > abs(lp-b))
                     tap.tmem[j] = lp;
                  /* note: no change if there is total ambiguity. (abs(sp-b) == abs(lp-b))  */
               }
            }
         }
      }
      /* clean... (threshold IS available)... */
      if(t>2 && tp!=NA)
      {
         sprintf(lin,"\nCleaning %s tape block from $%04X to $%04X.",ft[t].name,s,e);
         msgout(lin);

         for(j=s; j<e+1; j++)
         {
            b = tap.tmem[j];
            if(b>tp && b<lp+limit)
               tap.tmem[j] = lp;
            if(b<tp && b>sp-limit)
               tap.tmem[j] = sp;
            /* note: pulses that match threshold are unaffected. */
         }
      }
   }
   tap.changed=1;


   /* Flatten novaload trailers.. ie. gaps following NOVA blocks...
    this works nicely...  but should move it elsewhere sometime. */

   for(i=0; blk[i]->lt!=0 && blk[i+1]->lt!=0; i++)
   {
      if(blk[i]->lt==NOVA && blk[i+1]->lt==GAP)
      {
         for(j=blk[i+1]->p1; j<(blk[i+1]->p4)+1; j++)
            tap.tmem[j]= ft[NOVA].sp;
      }
   }

   msgout("\n");
   analyze();
}
/*---------------------------------------------------------------------------
 Creates new CBM pilot ($6A00 pulses) on bootable tapes
*/ 
void fix_boot_pilot(void)
{
   int i,siz,p;
   unsigned char *tmp;

   if(tap.bootable)
   {
      msgout("\nCreating new CBM boot pilot...");

      for(i=0; blk[i]->lt!=CBM_HEAD; i++)
      ;
      p= blk[i]->p2-(9*20);  /* get CBM header 'sync sequence' offset.  */

      siz= 20+ 0x6A00 + (tap.len -p);   /* compute length of finished tap */

      tmp= (unsigned char*)malloc(siz+1024);  /* create a buffer for the rewrite. */

      for(i=0; i<20; i++)     /* copy the original header.. */
         tmp[i]= tap.tmem[i];

      for(i=0; i<0x6A00; i++)     /* write out perfect CBM pilot.. */
         tmp[20+i]= ft[CBM_HEAD].sp;

      for(i=0x6A00+20; p<tap.len; p++)  /* and copy remainder of tap.. */
         tmp[i++]= tap.tmem[p];

      tap.len= i;     
      free(tap.tmem);
      tap.tmem= tmp;
      fix_header_size();
      tap.changed=1;

      msgout("  Done.");
      analyze();
   }
}
/*---------------------------------------------------------------------------
 Convert TAP to v1 format.
*/
void convert_to_v1(void)
{
   int i,j,z,z1,z2,z3;
   unsigned char b,*buf;

   if(tap.version==1)
      return;

   msgout("\nConverting to TAP v1...");

   /* convert $FF pulses to $00 in v0 TAP files first... */
   if(tap.version==0)
   {
      for(i=20; i<tap.len; i++)
      {
         if(tap.tmem[i]==0xFF)
            tap.tmem[i]=0;
      }
   }
   
   buf= (unsigned char*)malloc(tap.len+8192);  /* create buffer for rewrite. (+8k security) */

   for(i=0; i<20; i++)     /* copy the original header.. */
      buf[i]= tap.tmem[i];

   for(i=20,j=20; i<tap.len; i++)
   {
      b= tap.tmem[i];
      if(b!=0)
         buf[j++]= b;  /* non zero bytes go straight into out buffer */

      else  /* we found a zero... */
      {
         z= 0;
         while(tap.tmem[i++]==0 && i<tap.len+1)  /* now count the zeroes... */
            z++;
         z*=20000;  /* each zero is worth 20000 cycles (1/50 sec) */

         if(z>16777215)   /* allowed maximum is about 17 secs  */
            z= 16777215;

         z1= (unsigned long)(z&(0xFF0000))>>16;  /* get 24 bits worth of the total cycles... */
         z2= (unsigned long)(z&(0xFF00))>>8;
         z3= (unsigned long)(z&(0xFF));

         buf[j]= 0;    /* write v1 pause...  */
         buf[j+1]= z3;
         buf[j+2]= z2;
         buf[j+3]= z1;
         j+=4;
         i-=2;   /* have to step this back. */
      }
   }
   tap.len= j;
   free(tap.tmem);
   tap.tmem= buf;
   fix_header_size();
   tap.tmem[12]= 1;
   tap.version= 1;
   tap.changed= 1;

   msgout("  Done.");
   analyze();
}
/*---------------------------------------------------------------------------
 Convert TAP to v0 format.

 Note: a single v1 pause (4 bytes) when converted to v0 can result in as many
 as 838 bytes being generated in the output buffer

 ie. 00,FF,FF,FF (v1 pause of 16777215 cycles (17.03 seconds))
     =  838 '00's (v0 pauses)

  upshot: the allocation of the output buffer needs to made accurate or ft will
  crash when optimizing taps full of pauses!.

*/
void convert_to_v0(void)
{
   int i,j,z,c,total;
   unsigned char b, *tmp;

   if(tap.version==0)
      return;

   msgout("\nConverting to TAP v0...");

   /* new: first figure out the resulting filesize from the conversion so
      i can accurately allocate space and avoid crashes!...
      (tap files filled with v1 pauses could cause crashes before). */

   total=0;
   for(i=20; i<tap.len; i++)
   {
      b= tap.tmem[i];
      if(b==0)
      {
         z= (unsigned long) tap.tmem[i+1] + (tap.tmem[i+2]<<8) + (tap.tmem[i+3]<<16); /* get cycle count. */
         c= (unsigned long) z/20000;  /* output 1 zero for each 20000 cycles. */
         if(c==0)
            c=1;
         i+=3;
         total+=c;
      }
   }

   tmp= (unsigned char*)malloc(tap.len + total);  /* not accurate but enough! */

   for(i=0; i<20; i++)     /* copy the original header.. */
      tmp[i]= tap.tmem[i];

   for(i=20,j=20; i<tap.len; i++)
   {
      b= tap.tmem[i];
      if(b!=0)
         tmp[j++]= b;  /* non zero bytes go straight into out buffer */

      else  /* we found a zero... */
      {
         z= (unsigned long) tap.tmem[i+1] + (tap.tmem[i+2]<<8) + (tap.tmem[i+3]<<16); /* get cycle count. */
         c= (unsigned long) z/20000;  /* output 1 zero for each 20000 cycles.  */

         if(c==0)
            tmp[j++]= 0;   /* we must output at least one. */
         else
         {
            do
            {
               tmp[j++]= 0;
               c--;
            }
            while(c!=0);
         }
         i+=3;
      }
   }
   tap.len= j;
   free(tap.tmem);
   tap.tmem= tmp;
   fix_header_size();
   tap.tmem[12]= 0;
   tap.version= 0;
   tap.changed= 1;

   msgout("  Done.");
   analyze();
}
/*---------------------------------------------------------------------------
 Standardize pauses in v1 tap (only adjusts cbm).
*/
void standardize_pauses(void)
{
   int i,o,q;
   int b1,b2,b3, z1,z2,z3 ,zcnt=0;

   if(tap.version!=1)
      return;

   msgout("\nStandardizing pauses between 'C64 ROM tape' files...");

   for(i=0; blk[i]->lt!=0 && blk[i+1]->lt!=0 && blk[i+2]->lt!=0; i++)
   {
      b1= blk[i]->lt;    /* look at next 3 block types in database..  */
      b2= blk[i+1]->lt;
      b3= blk[i+2]->lt;

      if(b1>2 && b2==PAUSE && b3>2)  /* 2 data blocks with pause in the middle? */
      {
         q= 0;

         if(b1==CBM_HEAD && b3==CBM_DATA)
            q= 320000;  /* 0.32 sec */

         if(q!=0)
         {
            o= blk[i+1]->p1;  /* get offset of pause. */
            z1= (q & 0xFF);
            z2= (q & 0xFF00)>>8;
            z3= (q & 0xFF0000)>>16;
            tap.tmem[o+1]= z1;
            tap.tmem[o+2]= z2;
            tap.tmem[o+3]= z3;
            zcnt++;
         }
      }
   }
   sprintf(lin,"  Changed %d.", zcnt);
   msgout(lin);
   
   /*  Note : re-scan is unnecessary, tap structure will be unchanged. */
}
/*---------------------------------------------------------------------------
 replace certain gaps of 5 -> 15 pulses with new pilot byte....
*/
void fix_pilots(void)
{
   int i,cnt,d, wid, sp1,sp2, m[1024][3],mi;
   int b1,b2,b3,j, bpb;
   unsigned char *tmp;

   if(tap.version!=1)
      return;

   msgout("\nFixing broken pilot bytes...");

   wid=5;  /* Fix only broken pilots (gaps) of 5 to 15 pulses.  */

   /* the fixable pattern is ... (PAUSE or Start of TAP), GAP, DATAFILE. */
   do
   {
      mi=0;
      /* clear previous entries... */
      for(i=0;i<1024; i++)
         m[i][0]=0;

      /* create table of broken pilot offsets... */
      for(i=0; blk[i]->lt!=0 && blk[i+1]->lt!=0; i++)
      {
         b1=blk[i]->lt;
         b2=blk[i+1]->lt;

         if(b1==GAP && b2>2)    /* we look for... GAP, Datafile. */
         {

            b3=0;
            if((i-1)>=0)       /* check for a PAUSE before the GAP.. */
               b3=blk[i-1]->lt;

            if(b3==PAUSE || blk[i]->p1==20) /* GAP,Datafile is preceeded by PAUSE or start?.. */
            {
               if(blk[i]->xi==wid)   /* GAP width matches current?..  */
               {
                  m[mi][0]=blk[i]->p1; /* store GAP offset and following datablock type in table. */
                  m[mi][1]=b2;
                  m[mi][2]=8;   /* store default "bits per byte". */

                  if(b2==VISI_T1 || b2==VISI_T2 || b2==VISI_T3 || b2==VISI_T4)
                     m[mi][2] = 8 + (blk[i+1]->xi&7);   /* over-ride 'repair' width for visiload */
                                    
                  if(b2==SUPERTAPE_HEAD || b2==SUPERTAPE_DATA)
                     m[mi][2]=0; /* over-ride 'repair' width for supertape (just cut!) */

                  mi++;
                  m[mi][0]=0;  /* terminate list. */
               }
            }
         }
      }

      if(m[0][0]!=0)    /* there ARE fixable ones.. so fix em... */
      {
         /* first list the fixable pilots...  */
         for(i=0; m[i][0]!=0; i++)
         {
            sprintf(lin,"\n  Fixed %s pilot byte @ $%04X (%d pulses)",ft[m[i][1]].name, m[i][0], wid);
            msgout(lin);
         }

         d=0;
         mi=0;
         tmp = (unsigned char*)malloc(tap.len*2);  /* 2 times buffer for security. */

         sp1=0; /* initial start point is start of tap. */
         do
         {
            bpb = m[mi][2];    /* default is 8 bpb, visiload can vary. */
            sp2 = m[mi++][0];  /* get next stop point (fixable gap offset). */
            if(sp2==0)         /* if there are no more then sp2 = end-of-tap... */
               sp2=tap.len+1;
            for(cnt=sp1; cnt<sp2; cnt++) /* transfer the current range.. */
               tmp[d++] = tap.tmem[cnt];

            if(sp2!=tap.len+1)  /* and insert a new pilot byte here... */
            {
               for(j=0;j<bpb;j++)
                  tmp[d++] = tap.tmem[sp2+j+wid];  /* copy good pilot from source buffer */

               sp1= sp2+wid; /* jump the broken area in source buffer. */
            }

         }
         while(sp2!=tap.len+1);

         tap.len= d-1;
         free(tap.tmem);
         tap.tmem= tmp;   
         fix_header_size();
         tap.changed= 1;

         analyze();
      }
      wid++;
   }
   while(wid<16);

   msgout("  Done.");
}
/*---------------------------------------------------------------------------
 Cut small gaps that occur around pauses, up to 4 pulses before.
 must be a v1 TAP.
*/
void fix_prepausegaps(void)
{
   int i,cuts[2000][2],ci=0;

   if(tap.version!=1)
      return;

   msgout("\nCutting small pre-pause gaps...");
   
   /* look for this pattern... GAP(<5 pulses),PAUSE */

   for(i=0; blk[i]->lt!=0 && blk[i+1]->lt!=0; i++)
   {
      if(blk[i]->lt==GAP && blk[i]->xi<5)  /* gap <5 pulses? */
      {
         if(blk[i+1]->lt == PAUSE)
         {
            cuts[ci][0] = blk[i]->p1;   /* add this gap to the cut list */
            cuts[ci][1] = blk[i]->p4;
            ci++;
         }
      }
   }

   /* now cut in reverse order (saves having to rescan each time)... */
   for(i=ci-1; i>-1; i--)
      cut_range(cuts[i][0], cuts[i][1]);

   sprintf(lin,"  Cut %d.",ci);
   msgout(lin);
   analyze();
}
/*---------------------------------------------------------------------------
 Cut small gaps that occur after pauses.
 Note: Must be a v1 TAP.
*/ 
void fix_postpausegaps(void)
{
   int i,cuts[2000][2],ci=0;

   if(tap.version!=1)
      return;

   msgout("\nCutting small post-pause gaps...");

   /* we look for this pattern... PAUSE, GAP(<5 pulses)  */
   for(i=0; blk[i]->lt!=0 && blk[i+1]->lt!=0; i++)
   {
      if(blk[i]->lt== PAUSE)
      {
         if(blk[i+1]->lt == GAP && blk[i+1]->xi<5)
         {
            cuts[ci][0] = blk[i+1]->p1;   /* add this gap to the cut list */
            cuts[ci][1] = blk[i+1]->p4;
            ci++;
         }
      }
   }

   /* now cut in reverse order (saves having to rescan each time)... */
   for(i=ci-1; i>-1; i--)
      cut_range(cuts[i][0], cuts[i][1]);

   sprintf(lin,"  Cut %d.",ci);
   msgout(lin);
   analyze();
}
/*---------------------------------------------------------------------------
 Insert pauses between block types that should have them.
 returns the number of pauses inserted.
*/
int insert_pauses(void)
{
   int i,c;
   int t1,t2,t3;
   int pin[1000],pi=0,pt,cp;
   unsigned char *buf;

   if(tap.version!=1)
      return 0;

   msgout("\nInserting absent pauses...");

   buf= (unsigned char*)malloc(tap.len*2);

   /* create table of insertion points... */
   for(i=0; blk[i]->lt!=0 && blk[i+1]->lt!=0 && blk[i+2]->lt!=0; i++)
   {
      t1 = blk[i+0]->lt;
      t2 = blk[i+1]->lt;
      t3 = blk[i+2]->lt;

      if(t1==CBM_HEAD && (t2==GAP && blk[i+1]->xi>60 && blk[i+1]->xi<90) && t3==CBM_DATA)
         pin[pi++] = blk[i+1]->p4+1;

      if( (t1==CBM_HEAD && t2==CBM_DATA) ||
          (t1==CBM_DATA && t2==CBM_HEAD) ||

          (t1==BLEEP && t2==BLEEP) ||
          (t1==BLEEP && (t2==GAP && blk[i+1]->xi==8) && t3==BLEEP) ||

          (t1==CYBER_F3 && t2==CYBER_F3) ||
          (t1==CYBER_F3 && t2==GAP && blk[i+1]->xi==8 && t3==CYBER_F3)

        )
           pin[pi++] = blk[i+0]->p4+1;
   }
   pt=pi;

   /* now rewrite the tap to "buf" inserting pauses of
    (20,000 cycles) at the insertion points... */

   if(pt>0)
   {
      c= 0;    /* index into output buffer   */
      pi= 0;   /* index into insertion point table */

      for(i=0; i<tap.len; i++)
      {
         cp = pin[pi];   /* get next insertion point */

         if(i==cp)  /* if we reached the curent ins point... insert pause... */
         {
            buf[c+0]=0;     /* $004E20 = 20,000 cycles. */
            buf[c+1]=0x20;
            buf[c+2]=0x4E;
            buf[c+3]=0x00;
            c+=4;
            pi++;
         }
         buf[c++]= tap.tmem[i];  /* copy pulse to output */
      }
      tap.len= c;

      free(tap.tmem);
      tap.tmem= buf;
      fix_header_size();
      tap.changed= 1;
   }
   sprintf(lin,"  Inserted %d.",pt);
   msgout(lin);
   /* if(pt>0)
      msgout("  PLEASE RE-CLEAN!"); */

   analyze();

   return pt;
}
/*---------------------------------------------------------------------------
 Cut a selected range of bytes from the loaded TAP.
 Note : 'from','upto' and all offsets in-between are removed.
*/
void cut_range(int from, int upto)
{
   int i,j;
   unsigned char *buf;

   if(from<20 || from>tap.len)
      return;
   if(upto<20 || upto>tap.len)
      return;
   if(from>upto)
      return;
 
   /* checking done. both offsets are inside the tap, 'from' is either less
    than or equal to 'upto'. */

   buf= (unsigned char*)malloc(tap.len+1024);

   for(i=0,j=0; i<from; i++)   /* copy first range (all before 'from')... */
      buf[j++] = tap.tmem[i];
   if(upto!=tap.len)          /* copy second range (all after 'upto' if anything) */
   {
      for(i=upto+1; i<tap.len+1; i++)
         buf[j++] = tap.tmem[i];
   }
   tap.len=j-1;      /* set the new tap length */
   free(tap.tmem);   /* repoint *tap.tmem to this new buffer. */
   tap.tmem = buf;
   fix_header_size();
   tap.changed=1;

   /* Note: no rescan, the gap cutting functions should cut backwards through
      their 'hit-lists' to avoid rescanning each time */

   return;
}
/*---------------------------------------------------------------------------
 Cut gaps that occur after data files. this can be equivalent to cutting
 pre-pause gaps of any size <20. but it works whether there is a pause or not.
*/
void cut_postdata_gaps(void)
{
   int i,cuts[2000][2],ci=0;

   /* we look for this pattern...    DATAFILE, GAP(<20 pulses)  !SUPERTAPE  */

   for(i=0; blk[i]->lt!=0 && blk[i+1]->lt!=0; i++)
   {
      if(blk[i+1]->lt==GAP && blk[i+1]->xi<20)  /* gap <20 pulses? */
      {
         if(blk[i]->lt >2)
         {
            cuts[ci][0] = blk[i+1]->p1;   /* add this gap to the cut list */
            cuts[ci][1] = blk[i+1]->p4;
            ci++;
         }
      }
   }

   msgout("\nCutting post-data garbage...");

   /*
   for(i=0; i<ci; i++)
   {
      sprintf(lin,"$%04X -> $%04X", cuts[i][0], cuts[i][1]);
      msgout(lin);
   } */
   if(i==0)
   {
      msgout("  None found.");
      return;
   }
   /* now cut in reverse order (saves having to rescan each time)... */
   for(i=ci-1; i>-1; i--)
      cut_range(cuts[i][0], cuts[i][1]);

   sprintf(lin,"  Cut %d.",ci);   
   msgout("  Done.");
   analyze();
}
/*---------------------------------------------------------------------------
  Adds a trailing pause to the (v1) tap file in tap.tmem.
  the tap should have been previously clipped and converted to v1.
*/
void add_trailpause(void)
{
   int i;
   unsigned char *buf;

   if(tap.version!=1)
      return;

   msgout("\nAdding 5 second trailing pause...");

   buf= (unsigned char*)malloc(tap.len+1024);

   for(i=0; i<tap.len; i++)   /* copy existing tap to buffer... */
      buf[i] = tap.tmem[i];

   buf[i+0]=0;       /* $4B2B20 = 5 seconds */
   buf[i+1]=0x20;
   buf[i+2]=0x2B;
   buf[i+3]=0x4B;

   tap.len+=4;
   free(tap.tmem);
   tap.tmem= buf;
   fix_header_size();
   tap.changed= 1;

   msgout("  Done.");
   
   analyze();
}
/*---------------------------------------------------------------------------*/
void fill_cbm_tone(void)
{
   int i,j,t1,t2,t3;

   /* look for...
      CBM_DATA, GAP (80ish), PAUSE, fill the gap with ft[CBM_HEAD].sp.
   */

   for(i=0; blk[i+0]->lt!=0 &&  blk[i+1]->lt!=0 &&  blk[i+2]->lt!=0; i++)
   {
      t1 = blk[i+0]->lt;
      t2 = blk[i+1]->lt;
      t3 = blk[i+2]->lt;

      if(t1==CBM_DATA || t1==CBM_HEAD  && t2==GAP && t3==PAUSE)
      {
         if(blk[i+1]->xi>70 && blk[i+1]->xi<85)   /* gap is about 80 pulses? */
         {
            tap.changed=1;
            for(j=0; j<blk[i+1]->xi; j++)
               tap.tmem[(blk[i+1]->p1)+j]= ft[CBM_HEAD].sp;
         }
      }
   }
   /* Note: length has not changed but the gap SHOULD now disappear
    on rescan. (will be recognised as CBM tone). */

   if(tap.changed)
   {
      msgout("\nOverwrote CBM pilot tone gap(s).");
      analyze();
   }
}
/*---------------------------------------------------------------------------
 Detects corrupted Bleepload pilots and corrects them.
*/
void fix_bleep_pilots(void)
{
   int i,j,pt[1000],pi=0, s,e;
   int t,n,r,byt;

   if(tap.version!=1)
      return;

   msgout("\nLooking for Bleepload (block 0) pre-pilot corruptions...");

   /* we look for this pattern... GAP(x pulses), BLEEPLOAD(n pilot bytes) */

   /* we can fix where (x/8)+n == 256 or near. */

   for(i=0; blk[i]->lt!=0 && blk[i+1]->lt!=0; i++)
   {
      if(blk[i]->lt==GAP)
      {
         if(blk[i+1]->lt==BLEEP || blk[i+1]->lt==BLEEP_SPC)
         {
            t = ((blk[i+1]->p2 - blk[i+1]->p1)/8) -2;  /* get existing bleep pilot len */
            n = blk[i]->xi;                           /* get gap len */
            r = (n/8)+t;                             /* compute combined pilot total */
            byt = readttbyte(blk[i+1]->p1, ft[BLEEP].lp, ft[BLEEP].sp, ft[BLEEP].tp, MSbF);

            if(r>240 && r<260 && byt==0xFF)  /* total is roughly 256 bytes? */
            {
               sprintf(lin,"\n  block no=%d | existing pilot + (gap/8) = %d pilots",i+1,r);
               msgout(lin);

               pt[pi] = i;   /* record block number of gap in table. */
               pi++;
            }
         }
      }
   }
   if(pi>0)
   {
      for(i=0; i<pi; i++)   /* overwrite all found... */
      {
         s = blk[pt[i]]->p1;
         e = blk[pt[i]]->p4;
         for(j=s; j<e+1; j++)
            tap.tmem[j]= ft[BLEEP].lp;
      }
      tap.changed = 1;
      sprintf(lin,"\n   Found %d damaged bleepload pre-pilots and fixed them all.",pi);
      msgout(lin);
      
      analyze();
   }
   else
      msgout("\n  None found.");
}








