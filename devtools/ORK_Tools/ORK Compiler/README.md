# Usage: 

orkcmp.exe [options] <file.ORK> <file_to_compress/list.LST>

Options:

-d DIR   sets DIR folder for files in list

-s ORK   sets source ork file

-o       automatically overwrites entry if already exists

-c       use best possible compression for archive

-g XXX   generates ORK file for target game
   MOC   Mark of Chaos Retail (default)
   DOC   Mark of Chaos MP Demo
   AOX   Armies of Exigo Retail (default)
   DOX   Armies of Exigo SP Demo
   
-v       verbose mode

-q       quiet mode

# Description:

ORK files encrypter and compressor 0.1.1

Based on orkdec by Luigi Auriemma

The ORK files are archives without an index for the filenames they contain and
each file in them is encrypted using just its name as password so without
knowing it you cannot extract it.
If you want to know the names of these files (no names, no extraction) you need
to use ORK Dumper.

orkdec_extended.exe is re-compiled, modified orkcmp.exe where it is possible to actually re-pack modifications into the base Data.ork

# Example calls:

## use this to create new data file
./orkcmp.exe -g AOX -d "C:\Exigo_files" "C:\Armies of Exigo\Development\OutputDataORK\Data.ork" "C:\Armies of Exigo\Development\file_list.txt"

## use this to add more files to existing data file
./orkdec_extended.exe -g AOX -o -s "C:\Games\Armies of Exigo\SourceData.ork" -d "C:\Exigo_files" "C:\Armies of Exigo\Development\OutputDataORK\Data.ork" "C:\Armies of Exigo\Development\file_list.txt"

# License
GPL License (as specified by the original author)

## Authors:
Luigi Auriemma (https://aluigi.altervista.org, aluigi@autistici.org)
c/o Zoinkity (nefariousdogooder@yahoo.com)
Modified by: Suwak

See LICENSE for more info