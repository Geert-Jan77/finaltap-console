/*---------------------------------------------------------------------------
  filesearch.h

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
*/   

#define ROOTONLY 0     
#define ROOTALL 1

struct node
{
   char *name;
   struct node *link;
};
        
struct node* make_node(char *name);
struct node* get_dir_list(char *rootdir);
struct node* get_file_list(char *mask, struct node *dirs, int searchtype);
void show_list(struct node *r);
int save_list(struct node *r, char *fname);
void free_list(struct node *r);
void sort_list(struct node *r);
void clip_list(struct node *r);




