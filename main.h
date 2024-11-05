/*---------------------------------------------------------------------------

  main.h

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

   /* in main.c... */
   void search_tap(void);
   void describe_file(int row);
   
   void describe_files(void);
   int readttbit(int pos, int lp, int sp, int tp);
   int readttbyte(int pos, int lp, int sp, int tp, int endi);
   int find_pilot(int pos, int fmt);
   int load_tap(char *name);
   int save_tap(char *name);
   void unload_tap(void);
   int analyze(void);
   void report(void);
   void clean(void);
   int check_signature(void);
   int check_version(void);
   int check_size(void);
   float get_duration(int p1, int p2);
   int get_pulse_stats(void);
   void get_file_stats(void);
   int addblockdef(int type,int sof,int sod,int eod,int eof,int x1);
   void sort_blocks(void);
   int count_bootparts(void);
   void gap_search(void);
   void gap_describe(int row);
   int is_accounted(int x);
   int compute_overall_crc(void);
   int count_rpulses(void);
   int count_unopt_pulses(int slot);
   int count_opt_files(void);
   int count_good_checksums(void);
   int count_pauses(void);
   int is_pulse(int ofst, int width);
   int is_pause_param(int off);
   void make_prgs(void);
   int save_prgs(void);
   void copy_loader_table(void);
   void reset_loader_table(void);
   void show_loader_table(void);
   int find_decode_block(int lt, int num);
   int add_read_error(int addr);
   void print_results(char *buf);
   void print_database(char *buf);
   void print_pulse_stats(char *buf);
   void print_file_stats(char *buf);
   
   /* general utilities... (could be put into their own file really) */
   
   void msgout(char *str);
   int find_seq(unsigned char *buf,int bufsz, unsigned char *seq,int seqsz);
   void getfilename(char *dest, char *fullpath);
   char* pet2text(char *dest, char *src);
   void trimstring(char *str);
   void padstring(char *str, int wid);
   void time2str(int secs, char *buf);
   
   void deleteworkfiles(void);

   /* in clean.c...  really an extension of main.c so no separate header */
   void fix_header_size(void);
   void clip_ends(void);
   void unify_pauses(void);
   void clean_files(void);
   void fix_boot_pilot(void);
   void convert_to_v1(void);
   void convert_to_v0(void);
   void standardize_pauses(void);
   void fix_pilots(void);
   void fix_prepausegaps(void);
   void fix_postpausegaps(void);
   int insert_pauses(void);
   void cut_range(int from, int upto);
   void cut_postdata_gaps(void);
   void add_trailpause(void);
   void fill_cbm_tone(void);
   void fix_bleep_pilots(void);

   /* in loader_id.c...  really an extension of main.c so no separate header */
   int idloader(unsigned long crc);

   /* in batchscan.c... really an extension of main.c so no separate header */
   int batchscan(char *rootdir, int includesubdirs, int doscan);
   
   
   
   
   
   
   
   
   
