# Usage: 

orkdec.exe [options] <file.ORK/ORC> <file_to_extract/list.LST>

Options:

-d DIR   all the files will be extracted in the DIR folder

-o       automatically overwrite any file if already exist

# Description:

The ORK files are archives without an index for the archived filenames.
Each file is encrypted using just its name as password so without knowing this
name you cannot extract it.
If you want to know the names of these files (no names, no extraction) you need
to use ORK Dumper.

The ORC files used in Might & Magic Heroes VI and Supernova instead are just
encrypted ZIP files so no "file_to_extract" is necessary and you can just
specify a "".

# Example call:

./orkdec.exe -d "C:\Armies of Exigo\Development\Game_files" -o "C:\Games\Armies of Exigo\Data.ork" "C:\Armies of Exigo\Development\file_list.txt"

# License
GPL License (as specified by the original author)

## Authors:

Luigi Auriemma (https://aluigi.altervista.org, aluigi@autistici.org, original author) 
Modified by: Suwak

See LICENSE for more info