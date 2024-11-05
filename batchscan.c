/*-----------------------------------------------------------------------------
  batchscan.c
  
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
    
   Implements the file & directory searching/storage system for scanning
   multiple TAP files.
-----------------------------------------------------------------------------*/

#include "main.h"
#include "filesearch.h"
#include "mydefs.h"

#define MAXTAPS 8000
#define FIELDS 15

/*---------------------------------------------------------------------------
 Search for all tap files in directory 'rootdir', create a report of all taps
 found there (and optionally in any subdirectories).

 'rootdir' is the directory to scan for tap files.

 Set includesubdirs >0 to include the whole directory tree above rootdir in the scan.

 Set 'doscan' >0 to perform an actual scan, or 0 to just count the tap files
 available under 'rootdir' and return the amount.

 On success, returns the number of files found.
 On failure, returns -1.
*/
int batchscan(char *rootdir, int includesubdirs, int doscan)
{
   FILE *fp,*csv;
   int i,j,x,total_taps,failed_taps, total_scanned, swaps;
   struct node *dl,*fl,*t;
   struct tap_t *taps[MAXTAPS];
   struct tap_t *tmp;
   char fullpath[256]= "";
   char temp[256]= "";
   char cstr[2][256]= {"PASS","FAIL"};
   time_t t1,t2;
   char tfields[FIELDS][512];   /* temp fields */
   int fieldszs[FIELDS];
   char fields[FIELDS][512]=
   {"Name","Detected","Rec","Hdr","Opt","Chks","Read","Files",
    "Gaps","CRC32","Ver","Var","Size","cbmCRC32","LoaderID"};

   /* set default field sizes to lengths of field headers... */
   for(i=0; i<FIELDS; i++)
      fieldszs[i]= strlen(fields[i]);


   if(chdir(rootdir)==-1)     /* check rootdir exists */
   {
      sprintf(lin,"\n\nError: batchscan: directory %s does not exist.",rootdir);
      msgout(lin);
      return -1;
   }

   getcwd(fullpath,256);   /* get FULL path.
                           initially it may have been relative to users current dir */
 
   /* add separator to path if required... */
   if(fullpath[strlen(fullpath)-1]!='\\') 
      strcat(fullpath,"\\");  
 
   /* allocate storage for each taps gathered info... */
   for(i=0; i<MAXTAPS; i++)
   {
      taps[i]= (struct tap_t*)malloc(sizeof(struct tap_t));
      if(taps[i]==NULL)
      {
         msgout("Batchscan: Malloc Failure!");
         getch();
         exit(1);
      }   
   }
   msgout("\nSearching please wait...");

   time(&t1);  /* record current time so we can compute time taken. */

   /* build dir & file lists... */
   dl= get_dir_list(fullpath);

   if(includesubdirs == FALSE)
      fl= get_file_list("*.tap",dl, ROOTONLY);
   else
      fl= get_file_list("*.tap",dl, ROOTALL);
         
   clip_list(fl);
   sort_list(fl);  
      
   /* chdir(exedir); 
   save_list(fl, "batchfiles.txt"); */ 
            
   free_list(dl);
   
   /* copy tap file names to taps[] array... */
   total_taps= 0;
      
   for(t=fl->link,i=0; t!=NULL; i++)  /* note: t=fl->link skips the 1st entry "root name". */
   {
      if(i<MAXTAPS)   
      {
         strcpy(taps[i]->path, t->name);   /* copy all tap paths to the database.. */
         total_taps++;
      }
      t=t->link;
   }
   
   free_list(fl);
   
   /* all tap paths+names are now copied to the tap_tr array (taps) and the original
      filelist is freed . */



   if(total_taps>0)
   {
      sprintf(lin,"\n  Found %d TAP files.",total_taps);
      msgout(lin);

      if(doscan)
      {
         /* note: we use a temp file then rename it the the name of outfile
         because the frontend cannot open a file that is in use. */
         chdir(exedir);

         deleteworkfiles();

         fp= fopen(tempftbatchreportname,"w+t");   /* create a new report file... */
         if(fp==NULL)
            return -1;

         csv= fopen("batch.csv","w+t");   /* create a new csv file... */
         if(csv==NULL)
            return -1;

         j= total_taps;  /* countdown for the taps remaining. */
         failed_taps= 0;

         batchmode= 1;
         aborted= 0;
         
         /* Analyze each tap file in turn... */

         for(i=0; i<total_taps && !aborted; i++)
         {
            chdir(fullpath);  /* switch to the root directory */
          
            sprintf(lin,"\n\nTesting : %s",taps[i]->path);   /* display current tap name */
            msgout(lin);
            sprintf(lin,"\n%d remaining.", j--);  /* and amount remaining */
            msgout(lin);
 
            if(load_tap(taps[i]->path))    /* note : tap.name is updated to be filename only. */
            {
               if(analyze())
               {
                  report();
   
                  strcpy(taps[i]->path, tap.path);        
                  strcpy(taps[i]->name, tap.name);
                  taps[i]->len= tap.len;
                  taps[i]->detected_percent= tap.detected_percent;
                  taps[i]->purity= tap.purity;
                  taps[i]->total_data_files = tap.total_data_files;
                  taps[i]->total_gaps = tap.total_gaps;
                  taps[i]->fdate= tap.fdate;
                  taps[i]->version= tap.version;
                  taps[i]->crc= tap.crc;
                  taps[i]->cbmcrc= tap.cbmcrc;
                  taps[i]->cbmid= tap.cbmid;
                  strcpy(taps[i]->cbmname,tap.cbmname);
                  taps[i]->tst_hd= tap.tst_hd;
                  taps[i]->tst_rc= tap.tst_rc;
                  taps[i]->tst_op= tap.tst_op;
                  taps[i]->tst_cs= tap.tst_cs;
                  taps[i]->tst_rd= tap.tst_rd;   
   
                  if(tap.tst_hd || tap.tst_rc || tap.tst_op || tap.tst_cs || tap.tst_rd)
                     failed_taps++; 
               }
               else
                  failed_taps++;   /* was not a valid tap file */      
            }
         }
         total_scanned= i;   /* user may have aborted hence this!.  */


         /* sort taps[] by cbmcrc... */
         if(sortbycrc)
         {
            do
            {
               swaps=0;
               for(i=0; i<total_scanned-1; i++)
               {
                  if(taps[i]->cbmcrc > taps[i+1]->cbmcrc)
                  {
                     tmp= taps[i];
                     taps[i]= taps[i+1];
                     taps[i+1]= tmp;
                     swaps++;
                  }
               }
            }
            while(swaps!=0);
         }
         /*--------------------------------------------------------------------*/
         /* sort taps[] by filename... */
         else
         {
            do
            {
               swaps=0;
               for(i=0; i<total_scanned-1; i++)
               {
                  if(strcmp(taps[i]->path, taps[i+1]->path) >0)   /* sort by filename A-Z. */
                  {
                     tmp= taps[i];
                     taps[i]= taps[i+1];
                     taps[i+1]= tmp;
                     swaps++;
                  }
               }
            }
            while(swaps!=0);
         }


         /*--------------------------------------------------------------------*/
         /* prepare and print the report... */

         fprintf(fp,"\nFINAL TAP Batch Report\n");
         fprintf(fp,"\nPath= %s",rootdir);
         fprintf(fp,"\nRead Tolerance= %d",tol-1);
         fprintf(fp,"\n"); 

         /* pad each field header to match its longest data item... */
         for(i=0; i<total_scanned; i++)
         {
            sprintf(tfields[0],"%s",taps[i]->path);
            sprintf(tfields[1],"%d%%",taps[i]->detected_percent);
            sprintf(tfields[2],"%s",cstr[taps[i]->tst_rc]);
            sprintf(tfields[3],"%s",cstr[taps[i]->tst_hd]);
            sprintf(tfields[4],"%s",cstr[taps[i]->tst_op]);
            sprintf(tfields[5],"%s",cstr[taps[i]->tst_cs]);
            sprintf(tfields[6],"%s",cstr[taps[i]->tst_rd]);
            sprintf(tfields[7],"%d",taps[i]->total_data_files);
            sprintf(tfields[8],"%d",taps[i]->total_gaps);
            sprintf(tfields[9],"%08X",taps[i]->crc);
            sprintf(tfields[10],"%d",taps[i]->version);
            sprintf(tfields[11],"%d",taps[i]->purity);
            sprintf(tfields[12],"%d",taps[i]->len);
            sprintf(tfields[13],"%08X",taps[i]->cbmcrc);
            sprintf(tfields[14],"%s",knam[taps[i]->cbmid]);

            for(j=0 ; j<FIELDS; j++)  /* add padding if necessary... */
            {
               if(strlen(tfields[j]) > strlen(fields[j]))
                  padstring(fields[j], strlen(tfields[j]));
            }
         }

         /* print field headers to file... */
         fprintf(fp,"\n");
         for(i=0; i<FIELDS; i++)
            fprintf(fp,"%s  ",fields[i]);
         fprintf(fp,"\n");
             

         /* print data fields... */
         for(i=0; i<total_scanned; i++)
         {
            sprintf(tfields[0],"%s",taps[i]->path);
            sprintf(tfields[1],"%d%%",taps[i]->detected_percent);
            sprintf(tfields[2],"%s",cstr[taps[i]->tst_rc]);
            sprintf(tfields[3],"%s",cstr[taps[i]->tst_hd]);
            sprintf(tfields[4],"%s",cstr[taps[i]->tst_op]);
            sprintf(tfields[5],"%s",cstr[taps[i]->tst_cs]);
            sprintf(tfields[6],"%s",cstr[taps[i]->tst_rd]);
            sprintf(tfields[7],"%d",taps[i]->total_data_files);
            sprintf(tfields[8],"%d",taps[i]->total_gaps);
            sprintf(tfields[9],"%08X",taps[i]->crc);
            sprintf(tfields[10],"%d",taps[i]->version);
            sprintf(tfields[11],"%d",taps[i]->purity);
            sprintf(tfields[12],"%d",taps[i]->len);
            sprintf(tfields[13],"%08X",taps[i]->cbmcrc);
            sprintf(tfields[14],"%s",knam[taps[i]->cbmid]);

            
            /* print data fields to CSV file... */
            for(j=0; j<FIELDS; j++)
               fprintf(csv,"\"%s\",  ",tfields[j]); 
            fprintf(csv,"\n");
            
            /* pad each data field to match its field header if necessary... */
            for(j=0 ; j<FIELDS; j++)
            {
               if(strlen(tfields[j]) < strlen(fields[j]))
                  padstring(tfields[j], strlen(fields[j]));
            }
            
            /* print data fields to file... */
            fprintf(fp,"\n");
            for(j=0; j<FIELDS; j++)
               fprintf(fp,"%s  ",tfields[j]);
                        
         }
         
         time(&t2);
         time2str(t2-t1,lin);
         fprintf(fp,"\n\nOperation completed in %s.",lin);
                
         fprintf(fp,"\n\nTotal TAPs : %d ", i);
         fprintf(fp,"\nFailed : %d ", failed_taps);

         /* print key to table headers... */
         fprintf(fp,"\n\n");
         fprintf(fp,"\nName     - The name of the tap file.");
         fprintf(fp,"\nDetected - The percentage of the tap file that contains known files.");
         fprintf(fp,"\nRec      - Recognized : Test result for capacity of the tap filled with known files (see 'Detected').");
         fprintf(fp,"\nHdr      - Header : Test result for the tap file header.");
         fprintf(fp,"\nOpt      - Optimized : Test result for the optimization status of known files stored in the tap.");
         fprintf(fp,"\nChks     - Checksums : Test result for all checksums found in known files stored in the tap.");
         fprintf(fp,"\nRead     - Read Errors : Test result for read errors present in known files stored in the tap.");
         fprintf(fp,"\nFiles    - The total number of valid files found in the tap.");
         fprintf(fp,"\nGaps     - The total number of gaps found in the tap.");
         fprintf(fp,"\nCRC32    - The overall CRC32 checksum of all files stored in the tap.");
         fprintf(fp,"\nVer      - Version : The tap file version.");
         fprintf(fp,"\nVar      - Variations : The number of unique byte values present in the data section of the tap file.");
         fprintf(fp,"\nSize     - The size of the tap file in bytes.");
         fprintf(fp,"\ncbmCRC32 - The CRC32 checksum of the first CBM DATA file (if any) found in the tap.");
         fprintf(fp,"\nLoaderID - The name of the loader program contained in the first CBM DATA file (if exists\\is known).");
         fprintf(fp,"\n\n");
         fprintf(fp,"\nGenerated by Final TAP %0.2f console - (c) Subchrist Software. 2001-%d", FTVERSION,FTYEAR);
         fprintf(fp,"\n\n");
         fclose(fp);
         
         fclose(csv);

         chdir(exedir);
         sprintf(lin,"ren %s %s",tempftbatchreportname,ftbatchreportname);
         system(lin);
           
         /* print path/name of batch report to screen.. */
         sprintf(temp,fullpath);
         strcat(temp,ftbatchreportname);
         sprintf(lin,"\n\nSaved : %s",temp);
         msgout(lin);

         aborted= 0;
      }      
   }
   else
      msgout("\nError: No taps were found.");
   
   /* cleanup...    */
   for(i=0; i<MAXTAPS; i++)
      free(taps[i]);

   return total_taps;
}

