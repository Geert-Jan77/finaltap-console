/*-----------------------------------------------------------------------
  scanners.h   

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

-----------------------------------------------------------------------*/   
    
   void pause_search(void);
   void pause_describe(int row);
  
   int cbm_readbit(int pos);
   int cbm_readbyte(int pos);
   void cbm_search(void);
   int cbm_describe(int row);
    
   void turbotape_search(void);
   int turbotape_describe(int row);
    
   void freeload_search(void);
   int freeload_describe(int row);
    
   void usgold_search(void);
   int usgold_describe(int row);
    
   void nova_search(void);
   int nova_describe(int row);
    
   void nova_spc_search(void);
   int nova_spc_describe(int row);
    
   void aces_search(void);
   int aces_describe(int row);
    
   void wild_search(void);
   int wild_describe(int row);
    
   void chr_search(void);
   int chr_describe(int row);
    
   void ocean_search(void);
   int ocean_describe(int row);
    
   void raster_search(void);
   int raster_describe(int row);
    
   int visiload_readbyte(int pos, int endi,int abits);
   void visiload_search(void);
   int visiload_describe(int row);
    
   void cyberload_f1_search(void);
   int cyberload_f1_describe(int row);
    
   void cyberload_f2_search(void);
   int cyberload_f2_describe(int row);
    
   void cyberload_f3_search(void);
   int cyberload_f3_describe(int row);
    
   void cyberload_f4_search(void);
   int cyberload_f4_describe(int row);
    
   void bleep_search(void);
   int bleep_describe(int row);
    
   void bleep_spc_search(void);
   int bleep_spc_describe(int row);
    
   void hitload_search(void);
   int hitload_describe(int row);
    
   void micro_search(void);
   int micro_describe(int row);
    
   void burner_search(void);
   int burner_describe(int row);
    
   void rackit_search(void);
   int rackit_describe(int row);
    
   int superpav_readbyte(int sp, int mp, int lp, int pos, int mode);
   void superpav_search(void);
   int superpav_describe(int row);
    
   void anirog_search(void);
   int anirog_describe(int row);
    
   int supertape_readbyte(int pos, int mode);
   void supertape_search(void);
   int supertape_describe(int row);
    
   int pav_readbyte(int pos);
   void pav_search(void);
   int pav_describe(int row);
    
   void ik_search(void);
   int ik_describe(int row);
    
   void firebird_search(void);
   int firebird_describe(int row);
    
   void turrican_search(void);
   int turrican_describe(int row);
    
   void seuck1_search(void);
   int seuck1_describe(int row);
    
   void jetload_search(void);
   int jetload_describe(int row);
    
   void flashload_search(void);
   int flashload_describe(int row);
    
   void virgin_search(void);
   int virgin_describe(int row);
    
   void hitec_search(void);
   int hitec_describe(int row);
    
   void tdi_search(void);
   int tdi_describe(int row);
    
   void oceannew1t1_search(void);
   int oceannew1t1_describe(int row);
    
   void oceannew1t2_search(void);
   int oceannew1t2_describe(int row);
    
   void atlantis_search(void);
   int atlantis_describe(int row);
    
   void snakeload51_search(void);
   int snakeload51_describe(int row);
    
   void snakeload50t1_search(void);
   int snakeload50t1_describe(int row);
    
   void snakeload50t2_search(void);
   int snakeload50t2_describe(int row);
    
   void palacef1_search(void);
   int palacef1_describe(int row);
    
   void palacef2_search(void);
   int palacef2_describe(int row);
    
   void oceannew2_search(void);
   int oceannew2_describe(int row);
    
   void enigma_search(void);
   int enigma_describe(int row);

   void audiogenic_search(void);
   int audiogenic_describe(int row);
   
   
   
   
   
   
   
   
   
   
   
   
   
