
-------------------------------------------------------------------------------

 FINAL TAP - Console Version.

 (c) 2001-2006, Stewart Wilson, Subchrist Software.

 Last Updated : March 25, 2006.

-------------------------------------------------------------------------------


Final TAP is a tool for testing and remastering 'Commodore 64' software tapes 
once they have been successfully converted to TAP files (digital files 
representing the data found on casette tapes by means of pulsewidth measurements).

Rather than remastering blindly (beautifying), Final TAP performs an exhaustive 
search for known file formats within the TAP file, once any files are located they
are data extracted, read tested and checksum verified (where available) and those
files may then be perfectly optimized to 'factory settings'.

The reason for doing this is that TAP files are converted from casette tapes which
stretch with age and use, many original C64 tapes are now more than 20 years old 
and the magnetic signal on most will have faded and possibly become damaged, 
this will of course be mirrored in the TAP files made from these tapes. 

Final TAP will be able to tell whether and where the tape has been corrupted and 
attempt to fix it up by means of noise removal and cleaning of the signal.

The result in most cases is an immaculate, validated digital copy of the tape
which may be written back to a casette (using appropriate hardware or via 
a soundcard) or simply used with a C64 emulator such as VICE or CCS64.  


Please note that your success rate with this sofware will very much depend on the
quality of the TAP files that you use, for best results the TAP files should be made
using a parallel port connection between your PC and an original C2N casette deck
using a C64S adaptor and Markus Brenner's MTAP tool.

For more information on this subject visit these web sites...


http://markus.brenner.de/

http://tapes.c64.no/

-------------------------------------------------------------------------------




Final TAP usage...


Final TAP should be run from the command line.

Navigate to the program's directory to begin, then type "ftcon" to show usage information

Alternatively, you may drag and drop a TAP file directly to the programs icon using
windows Explorer, this will cause a check of the tap file to be performed using all the
default program options.


 
___________________________________________________________

Final TAP 2.76 Console - (C) 2001-2006 Subchrist Software.
___________________________________________________________

Usage:-

ft [option][parameter]

options...

 -t   [tap]     Test TAP.
 -o   [tap]     Optimize TAP.
 -b   [dir]     Batch test.
 -au  [tap]     Convert TAP to Sun AU audio file (44kHz).
 -wav [tap]     Convert TAP to Microsoft WAV audio file (44kHz).
 -rs  [tap]     Corrects the 'size' field of a TAPs header.
 -ct0 [tap]     Convert TAP to version 0 format.
 -ct1 [tap]     Convert TAP to version 1 format.

 -tol [0-15]    Set pulsewidth read tolerance, default=10.
 -debug         Allows detected files to overlap.
 -noid          Disable scanning for only the 1st ID'd loader.
 -noc64eof      C64 ROM scanner will not expect EOF markers.
 -docyberfault  Report Cyberload F3 bad checksums of $04.
 -boostclean    Raise cleaning threshold.
 -noaddpause    Dont add a pause to the file end after clean.
 -sine          Make audio converter use sine waves.
 -prgunite      Connect neighbouring PRG's into a single file.
 -extvisipatch  Extract Visiload loader patch files.
 -incsubdirs    Make batch scan include subdirectories.
 -sortbycrc     Batch scan sorts report by cbmcrc values.




Notes:- 

  -After testing, Final TAP will save a report to the TAP's directory, the report 
   file's name will be "ftreport.txt".  

  -After batch testing, Final TAP will save a report to the current directory, the
   report file's name is always "ftbatch.txt".  

  -After optimizing, Final TAP will save the cleaned TAP file to a file called "out.tap"


The following options are made available in case of file 'misdetection' which
can sometimes occur, ie. if you scan a tape and all files found on it are 
Novaload apart from one Ocean file then you should disable the Ocean scanner 
and try again.


 -noaces               Disable "Ace Of Aces" scanner.
 -noanirog             Disable "Anirog" scanner.
 -noatlantis           Disable "Atlantis" scanner.
 -noaudiogenic         Disable "Audiogenic" scanner.
 -nobleep              Disable "Bleepload" scanner.
 -noburner             Disable "Burner" scanner.
 -nochr                Disable "CHR" scanner.
 -nocyber              Disable "Cyberload" scanner.
 -noenigma             Disable "Enigma Variations" scanner.
 -nofire               Disable "Firebird" scanner.
 -noflash              Disable "Flashload" scanner.
 -nofree               Disable "Freeload" scanner.
 -nohit                Disable "Hitload" scanner.
 -nohitec              Disable "Hi -Tec" scanner.
 -nojet                Disable "Jetload" scanner.
 -noik                 Disable "International Karate" scanner.
 -nomicro              Disable "Microload" scanner.
 -nonova               Disable "Novaload" scanner.
 -noocean              Disable "Ocean" scanner.
 -nooceannew1t1        Disable "Ocean New (Threshold 1)" scanner.
 -nooceannew1t2        Disable "Ocean New (Threshold 2)" scanner.
 -nooceannew2          Disable "Ocean New 2" scanner. 
 -nopalacef1           Disable "Palace (Format 1)" scanner.
 -nopalacef2           Disable "Palace (Format 2)" scanner.
 -nopav                Disable "Pavloda" scanner.
 -norackit             Disable "Rackit" scanner.
 -noraster             Disable "Rasterload" scanner.
 -noseuck              Disable "SEUCK" scanner.
 -nosnake50            Disable "Snakeload 5.0" scanner. 
 -nosnake51            Disable "Snakeload 5.1" scanner. 
 -nospav               Disable "Super Pavloda" scanner. 
 -nosuper              Disable "Supertape" scanner. 
 -notdif1              Disable "Tengen Domark Imageworks (Format 1)" scanner. 
 -noturbo              Disable "Turbotape" scanner. 
 -noturr               Disable "Turrican" scanner. 
 -nousgold             Disable "US Gold" scanner. 
 -novirgin             Disable "Virgin" scanner. 
 -novisi               Disable "Visiload" scanner. 
 -nowild               Disable "Wildload" scanner. 

 -noall                Disable ALL scanners apart from 'C64 ROM Tape'.



Miscellaeneous 'expert' options...


 -ec           Export decrypted Cyberload loaders.

 -sortcrc      Makes the batch report be sorted by cbmCRC, handy for identifying new loaders.

 -sine         Forces audio conversion to use sine waves rather than square.
               (Which may (or may not) help when writing back to real tape, let me know please, im
               unable to test this, all I know is that they sound more muffled!). 

 -info         Makes Final TAP output a small text file containing version information.
               (can be used by a frontend)

-------------------------------------------------------------------------------

FAQ...

Q. Is Final TAP's optimizer safe?

A. Final TAP only affects parts of the TAP file that it recognizes, this means that only known file
   types and pauses will be affected by optimization.

   Everything else in the TAP file will remain unchanged which is why you may struggle to reach a
   PASS condition (100% recognized) with some 'dirty' TAPs.

   If your TAP file(s) should fail to achieve a 100% recognized condition, this is because they
   either contain file types that FT does not yet support or they contain areas of 'noise' that 
   could be harmful to remove.

   From FT's point of view, both of the above things are simply 'unrecognized files'.




Q. How can I test or clean 'B' sides that contain no loader but require loader variables from the 
   'A' side?.

A. Final TAP preserves loader variables when multiple operations are specified, so for loaders like
   Cyberload that have slightly varying formats for different tapes you should do something like
   this...

   ft -t LastNinjaA.tap -t LastNinjaB.tap

   This allows FT to discover the Cyberload variables from the 'A' side (contains the loader program)
   and test the B side (data files only) using those same variables. 

   
















