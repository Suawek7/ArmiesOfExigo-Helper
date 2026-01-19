/*
    Orkcmp, c/o Zoinkity (nefariousdogooder@yahoo.com)--no rights reserved.
    Based on orkdec, copyright 2007 Luigi Auriemma under GPL.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

    http://www.gnu.org/licenses/gpl.txt
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <zlib.h>       /* -lz*/
#include "ork_algo.h"

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

#ifdef WIN32
#include <direct.h>
#define PATHSLASH   '\\'
#define make_dir(A) mkdir(A);

#else
#include <unistd.h>
#include <dirent.h>

#define stricmp     strcasecmp
#define strnicmp    strncasecmp
#define stristr     strcasestr
#define PATHSLASH   '/'
#define make_dir(A) mkdir(name, 0755);
#endif

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;



#define VER     "0.1.1"



static uint8_t ork_new[3][22] = {{
        0x00, 0x00, 0x00, 0x00, 0x67, 0x6A, 0xBF, 0x0F, 
        0x5B, 0x3F, 0xA9, 0x04, 0x47, 0x6F, 0x6F, 0x64, 
        0x20, 0x6C, 0x75, 0x63, 0x6B, 0x21
        },
       {0x00, 0x00, 0x00, 0x00, 0x67, 0x6A, 0xBF, 0x0F, 
        0x8F, 0x4D, 0x57, 0x7E, 0x47, 0x6F, 0x6F, 0x64, 
        0x20, 0x6C, 0x75, 0x63, 0x6B, 0x21
        },
       {0x00, 0x00, 0x00, 0x00, 0x67, 0x6A, 0xBF, 0x0F, 
        0xD3, 0x97, 0xB1, 0xFD, 0x47, 0x6F, 0x6F, 0x64, 
        0x20, 0x6C, 0x75, 0x63, 0x6B, 0x21
        }
       };



void delimit(u8 *data);
int list_file(FILE *fd, ork_index_t *idx, u32 files, u8 *fname);
void check_keyset(FILE *fd);
int ork_extract(FILE *fd, ork_index_t *idx, u32 files, u8 *fname, u8 **retbuff, u32 *retsize);
int ork_encode(FILE *fd, ork_index_t *idx, u32 files, u8 *fname, u8 **retbuff, u32 *retsize);
u8 *create_dir(u8 *name);
void fd_write(u8 *name, u8 *data, int datasz);
void fd_read(u8 *name, u8 *data, int datasz);
u8 *strduplow(u8 *data);
int yes_no(void);
void std_err(void);
int ork_append(FILE *fd, ork_index_t *idx, u32 files, u8 *fname);
int ork_append_index(FILE *fd, ork_index_t *idx, u32 files);



u32     extracted   = 0;
int     keyset      = 0,
        quiet       = 1,
        arc_type    = Z_DEFAULT_COMPRESSION,
        overwrite   = 0;
u8      *folder     = NULL;

#define SHH(A)         if(quiet<A) 

int main(int argc, char *argv[]) {
    ork_key_t   ork_key;
    ork_index_t *ork_index;
    FILE    *fd,
            *src;
    u32     i,
            files       = 0,
            offset;
    u8      sign[10],
            *source     = NULL,
            *infile     = NULL,
            *outfile    = NULL;

    setbuf(stdout, NULL);

    fputs("\n"
        "ORK files encrypter and compressor "VER"\n"
        "c/o Zoinkity (nefariousdogooder@yahoo.com)\n"
        "Based on orkdec by Luigi Auriemma\n"
        "e-mail: aluigi@autistici.org\n"
        "web:    aluigi.org\n"
        "\n", stdout);

    if(argc < 3) {
help:
        printf("\n"
            "Usage: %s [options] <file.ORK> <file_to_compress/list.LST>\n"
            "\n"
            "Options:\n"
            "-d DIR   sets DIR folder for files in list\n"
            "-s ORK   sets source ork file\n"
            "-o       automatically overwrites entry if already exists\n"
            "-c       use best possible compression for archive\n"
            "-g XXX   generates ORK file for target game\n"
            "   MOC   Mark of Chaos Retail (default)\n"
            "   DOC   Mark of Chaos MP Demo\n"
            "   AOX   Armies of Exigo Retail (default)\n"
            "   DOX   Armies of Exigo SP Demo\n"
            "-v       verbose mode\n"
            "-q       quiet mode\n"
            "\n"
            "The ORK files are archives without an index for the filenames they contain and\n"
            "each file in them is encrypted using just its name as password so without\n"
            "knowing it you cannot extract it.\n"
            "If you want to know the names of these files (no names, no extraction) you need\n"
            "to use my orkdec_files program available here:\n"
            "\n"
            "  http://aluigi.org/papers.htm#others-file\n"
            "\n", argv[0]);
        exit(1);
    }

    argc -= 2;
    for(i = 1; i < argc; i++) {
        if(((argv[i][0] != '-') && (argv[i][0] != '/')) || (strlen(argv[i]) != 2)) {
            printf("\nError: wrong argument (%s)\n", argv[i]);
            exit(1);
        }
        switch(argv[i][1]) {
            case 'd': folder    = argv[++i];          break;
            case 's': source    = argv[++i];          break;
            case 'o': overwrite = 1;                  break;
            case 'v': quiet     = 0;                  break;
            case 'q': quiet     = 2;                  break;
            case 'c': arc_type  = Z_BEST_COMPRESSION; break;
            case '?': 
            case 'h': goto help;                      break;
            case 'g': argv[i++];
                      if(!strnicmp(argv[i],"MOC",3) && !strnicmp(argv[i],"AOX",3)) keyset=0;
                      if(!strnicmp(argv[i],"DOX",3))                               keyset=1;
                      if(!strnicmp(argv[i],"DOC",3))                               keyset=2;
                      if(keyset) printf("\nKeyset set for %s.\n",keyset>1 ? "W:MOC Demo":"AoX Demo");
                      else       printf("\nKeyset set to retail (default) %s.\n",argv[i]);
                      break;
            default: {
                printf("\nError: wrong argument (%s)\n", argv[i]);
                exit(1);
            }
        }
    }

    outfile  = argv[argc];
    infile   = argv[argc + 1];
    
    /*open source archive*/
    if(source) {
        src = fopen(source, "rb");
        if(!src) std_err();
        }
    else {
        src = tmpfile();
        if(!src) std_err();
        fwrite(ork_new[keyset],22,1,src);
        } 
    
    SHH(2) printf("- open file %s\n", outfile);
    fd = fopen(outfile, "wb+");
    if(!fd) std_err();
        
    /*shuffle this about to make a data appender, instead of complete regeneration*/
    if(fseek(src, -sizeof(sign), SEEK_END) < 0) std_err();
    fread(sign, sizeof(sign), 1, src);
    if(memcmp(sign, ORK_SIGN, sizeof(sign))) {
        printf("\nError: the file doesn't seem a valid ORK archive!\n");
        DBG printf("\t%s (%u) vs %s (%u)\n",sign,sizeof(sign),ORK_SIGN,sizeof(ORK_SIGN));
        exit(1);
    }

    if(fseek(src, 4, SEEK_SET) < 0) std_err();

    fread(&offset, 4, 1, src);
    offset ^= ORK_OFFSET;

    if(quiet<1 || DEBUGGERY) printf("-- index offset 0x%08x\n", offset);
    if(fseek(src, offset, SEEK_CUR) < 0) std_err();

    SHH(2) printf("-- check the archive type\n");
    check_keyset(src);

    SHH(1) printf("-- initialize the index keys\n");
    ork_key_index_init(&ork_key, keyset);

    ork_read(src, &ork_key, (void *)&files, 4);

    ork_index = malloc(sizeof(ork_index_t) * files+1);
    if(!ork_index) std_err();

    SHH(2) printf("-- read index\n");

    for(i = 0; i < files; i++) {
        ork_read(src, &ork_key, (void *)&ork_index[i], sizeof(ork_index_t));

        ork_index[i].size    = ork_num(i, ork_index[i].size);
        ork_index[i].zipsize = ork_num(i, ork_index[i].zipsize);
        ork_index[i].offset  = ork_num(i, ork_index[i].offset);
    }
    DBG printf("DEBUG: #entries %u\n",files);
    
    /*this is the cheap and sloppy method--blanket copy data to output*/
    SHH(1) printf("- copying original data\n");
    fseek(src,0,SEEK_SET);
    for(i = 0; i < (offset+8); i++) fputc(fgetc(src),fd);
    fclose(src);

        /* list_file and change folder*/    
/*    if(!list_file(src, ork_index, files, outfile)) {
        goto quit;
    }
*/
    if(folder) {
        SHH(2) printf("\n- input folder %s\n", folder);
        if(chdir(folder) < 0) std_err();
    }
        
    SHH(2) printf("\n- assembling files and index\n");
    ork_append(fd, ork_index, files, infile);
        

quit:
    if(fd) fclose(fd);
    if(ork_index) free(ork_index);
    SHH(2) printf("\n- %u files appended\n", extracted);
    return(0);
}



void delimit(u8 *data) {
    while(*data && (*data != '\n') && (*data != '\r') && (*data != ';')) data++;
    *data = 0;
}



int list_file(FILE *fd, ork_index_t *idx, u32 files, u8 *fname) {
    FILE    *fdl;
    u8      buff[MAX_PATH + 1],
            *p;

    p = strrchr(fname, '.');
    if(!p) return(-1);
    if(stricmp(p + 1, "lst") && stricmp(p + 1, "txt")) return(-1);

    SHH(2) printf("- use %s as a list of files to extract\n", fname);
    fdl = fopen(fname, "rb");
    if(folder) {
        SHH(2) printf("- output folder %s\n", folder);
        if(chdir(folder) < 0) std_err();
    }
    if(!fdl) return(-1);

    while(fgets(buff, sizeof(buff), fdl)) {
        delimit(buff);

        ork_extract(fd, idx, files, buff, NULL, NULL);
    }

    fclose(fdl);
    return(0);
}


/*for generation from list*/
int ork_append(FILE *fd, ork_index_t *idx, u32 files, u8 *fname) {
    FILE    *fdl;
    u32     offset;
    u8      buff[MAX_PATH + 1],
            *p;

    /*detect if a list or a string*/
    fdl = NULL;
    p = strrchr(fname, '.');
    if(p) {
        if(!stricmp(p + 1, "lst") || !stricmp(p + 1, "txt")) {
            SHH(2) printf("- use %s\n  as a list of files to encode\n", fname);
            fdl = fopen(fname, "rb");
            }
        }
    if(!fdl) {
        p = malloc(L_tmpnam);
        if(!p)   std_err();
        tmpnam(p);
        fdl = fopen(p, "wb+");
        if(!fdl) std_err();
        SHH(2) printf("- encoding %s in ORK archive\n",fname);
        fwrite(fname,strlen(fname),1,fdl);
        fseek(fdl,0,SEEK_SET);
        }

    while(fgets(buff, sizeof(buff), fdl)) {
        delimit(buff);

        DBG printf("Debug: reallocating for %u files\n",files+1);
        // Update the realloc usage in ork_append
        ork_index_t *temp_idx = realloc(idx, sizeof(ork_index_t) * (files + 1));
        if (!temp_idx) {
            std_err(); // Handle allocation error
        } else {
            idx = temp_idx; // Update idx only if realloc succeeded
        }
        
        if(!idx) std_err();
        
        DBG printf("Debug: %5u @ %8X\t%s\n",files,ftell(fd),buff);
        if(!ork_encode(fd, idx, files, buff, NULL, NULL)) files++;
        DBG printf("Debug: data copy\t%u\t%08X %08X\t%08X-%08X\t%08X\n",files-1,idx[files-1].filecrc,idx[files-1].namecrc,idx[files-1].size,idx[files-1].zipsize,idx[files-1].offset);
    }

    fclose(fdl);
    
    /*while within scope, append the index.  
      One level up is out of scope for realloc'd data*/
    /*find index offset*/
    fseek(fd,0,SEEK_END);
    offset = (ftell(fd)-8) ^ ORK_OFFSET;
    
    /*copy the index to file*/
    SHH(1) printf("\n- copying %u entries to index\n",files);
    ork_append_index(fd, idx, files);
    
    /*We have ORK_SIGN*/
    SHH(1) printf("- We have ORK_SIGN\n");
    fwrite(&ORK_SIGN,sizeof(ORK_SIGN)-1,1,fd);
    
    /*set index offset in file*/
    SHH(1) printf("- Setting index offset to %08X (%08X)\n",offset,offset^ORK_OFFSET);
    fseek(fd,4,SEEK_SET);
    fwrite(&offset,1,4,fd);
    
    return(0);
}



int check_duplicates(u32 crc) {
    static u32  *crct   = NULL;
    static int  crctblk = 4096,
                crctsz  = 0;
    int         i;

    for(i = 0; i < crctsz; i++) {
        if(crct[i] == crc) return(-1);
    }

    if(!(crctsz % crctblk)) {
        crct = realloc(crct, sizeof(u32) * (crctsz + crctblk));
        if(!crct) std_err();
    }
    crct[crctsz++] = crc;
    return(0);
}



void check_keyset(FILE *fd) {
    ork_key_t   ork_key;
    struct stat xstat;
    u32     files,
            right_size,
            our_size;
    int     set_end;

    keyset = 0;

    fstat(fileno(fd), &xstat);

redo:
    SHH(1) printf("- check key set %u\n", keyset);

    set_end = ork_key_index_init(&ork_key, keyset);
    ork_read(fd, &ork_key, (void *)&files, 4);
    fseek(fd, -4, SEEK_CUR);

    our_size   = files * sizeof(ork_index_t);
    right_size = xstat.st_size - 10 - (ftell(fd) + 4);
        /*       |               |    |
        //       |               |    where the index starts
        //       |               signature
        //       total size of the file
        */

    if(our_size != right_size) {
        SHH(2) printf("- index of %u bytes while should be %u\n", our_size, right_size);
        if(set_end < 0) {
            printf("\n"
                "Error: there is something wrong with the prebuilt keys\n"
                "       exist a series of keys for the retail games (tested Armies of Exigo\n"
                "       and Warhammer Mark of Chaos) and others for the demos\n"
                "       I have tried all the cases I know and the result seems wrong, the size\n"
                "       of the index differs from the size of the ORK archive between the\n"
                "       starting of the index and the final ORK signature.\n"
                "\n");
            exit(1);
        }
        keyset++;
        goto redo;
    }
}



int ork_extract(FILE *fd, ork_index_t *idx, u32 files, u8 *fname, u8 **retbuff, u32 *retsize) {
    ork_key_t   key;
    u32     i,
            crc,
            len,
            zlen;
    int     ret     = -1;
    u8      *zbuff  = NULL,
            *buff   = NULL;

    fname = strduplow(fname);       /* in case it's a fixed buffer*/

    crc = ork_crc(fname, strlen(fname),NULL,NULL);
    if(check_duplicates(crc) < 0) goto quit;

    SHH(1) printf("\n"
        "- search \"%s\" (%08x)\n",
        fname, crc);

    for(i = 0; i < files; i++) {
        if(idx[i].namecrc == crc) break;
    }
    if(i == files) {
        SHH(1) printf("  Error: file not found\n");
        goto quit;
    }
    idx = &idx[i];

    zlen = idx->zipsize;
    len  = idx->size;

    zbuff = malloc(zlen);
    buff  = malloc(len);
    if(!zbuff || !buff) std_err();

    fseek(fd, 8 + idx->offset, SEEK_SET);

    SHH(1) printf("- decrypt data\n");
    ork_key_file_init(&key, fname, keyset);
    ork_read(fd, &key, zbuff, zlen);

    SHH(1) printf("- unzip data\n");
    if(uncompress(buff, (void *)&len, zbuff, zlen) != Z_OK) {
        printf("\n"
            "Error: something wrong during the unpacking of the file\n"
            "       probably the filename is not correct (it's case sensitive!)\n");
        goto quit;
    }

    if(!retbuff) {
        fd_write(fname, buff, len);
    } else {
        *retbuff = buff;
        *retsize = len;
        buff     = NULL;    /* for not freeing it*/
    }
    ret = 0;

quit:
    if(buff)  free(buff);
    if(zbuff) free(zbuff);
    free(fname);
    return(ret);
}


/*presumes idx[files] is a blank entry...*/
int ork_encode(FILE *fd, ork_index_t *idx, u32 files, u8 *fname, u8 **retbuff, u32 *retsize) {
    FILE    *data;
    ork_key_t   key;
    long    size;
    u32     i,
            x,
            offset,
            fcrc,
            crc,
            len,
            zlen;
    int     ret     = -1;
    u8      *zbuff  = NULL,
            *buff   = NULL;

    fname = strduplow(fname);       /* in case it's a fixed buffer*/
    
    fseek(fd,0,SEEK_END);
    size   = ftell(fd);
    offset = size;

    ork_crc(fname, strlen(fname),&fcrc,&crc);
    if(check_duplicates(crc) < 0) goto quit;

    SHH(1) printf("\n"
        "- search \"%s\" (%08x)\n",
        fname, crc);

    for(i = 0; i < files; i++) {
        if(idx[i].namecrc == crc) break;
    }
    if(i < files) {
        if(!overwrite) {
            printf("+ duplicate file found at index %i\n",i);
            printf("+ do you want to overwrite it (y/N)?\n  ");
            fflush(stdin);
            if(yes_no() == 'n') goto quit;
            }
        DBG printf("Debug: replacement\t%u\t%08X %08X\t%08X-%08X\t%08X\n",i,idx[i].filecrc,idx[i].namecrc,idx[i].size,idx[i].zipsize,idx[i].offset);
        offset = idx[i].offset+8;
    }

    data = fopen(fname, "rb");
    if(!data) goto quit;
    
    fseek(data,0,SEEK_END);
    len  = ftell(data);
    zlen = len;
    fclose(data);
    
    zbuff = malloc(zlen);
    buff  = malloc(len);
        
    if(!zbuff || !buff) std_err();
    
    /*copy data*/
    fd_read(fname,buff,len);
    
    SHH(1) printf("-- zip data\n");
    if(compress2(zbuff, (void *)&zlen, buff, len, arc_type) != Z_OK) {
        printf("\n"
            "Error: something wrong during compression\n"
            "       probably the buffer is too small\n");
        goto quit;
    }

    SHH(1) printf("-- encrypting data\n");
    ork_key_file_init(&key, fname, keyset);
    ork_dec(ORK_DEC_CMP, &key, zbuff, zlen);
    
        
    ret = 0;

    if(!retbuff) {
        SHH(1) printf("-- copying to file\n");
        fseek(fd,offset,SEEK_SET);
        if(i<files) {
            ret=1;
            if(idx[i].zipsize == zlen) {
                DBG printf("Debug: offset %08X\tcur %08X\n",offset,ftell(fd));
                fwrite(zbuff, zlen, 1, fd);
            } else {
                SHH(1) printf("--- generating temp copy of data\n");
                realloc(buff,L_tmpnam);
                if(!buff) std_err();
                tmpnam(buff);
                if(!(data=fopen(buff,"wb+"))) std_err();
                
                fseek(fd,idx[i].zipsize,SEEK_CUR);
                size-=ftell(fd);
                DBG printf("Debug: offset %08X\tcur %08X\tsize %X\n",offset,ftell(fd),size);
                for(x=0;x<size;x++) fputc(fgetc(fd),data);
                
                SHH(1) printf("--- resizing file and appending data\n");
                fseek(fd,offset,SEEK_SET);
                if(ftruncate(fileno(fd),offset)) std_err();
                fwrite(zbuff, zlen, 1, fd);
                
                fseek(data,0,SEEK_SET);
                for(x=0;x<size;x++) fputc(fgetc(data),fd);
                
                SHH(1) printf("--- correcting index\n");
                size = zlen-idx[i].zipsize;
                for(x=i+1;x<files;x++) idx[x].offset = idx[x].offset + size;
            }
        }
        else fwrite(zbuff, zlen, 1, fd);
        idx[i].offset  = offset-8;
    } else {
        idx[i].offset  = 0;
        *retbuff = zbuff;
        *retsize = zlen;
        zbuff    = NULL;    /* for not freeing it*/
    }

    idx[i].filecrc = fcrc;
    idx[i].namecrc = crc;
    idx[i].size    = len;
    idx[i].zipsize = zlen;

    DBG printf("Debug: end append\t%u\t%08X %08X\t%08X-%08X\t%08X\n",i,idx[i].filecrc,idx[i].namecrc,idx[i].size,idx[i].zipsize,idx[i].offset);

quit:
    if(buff)  free(buff);
    if(zbuff) free(zbuff);
    return(ret);
}



int ork_append_index(FILE *fd, ork_index_t *idx, u32 files) {
    u32       i,
              x,y,z,
              offset,
              len     = 4;
    u8        *buff;
    ork_key_t key;
    int       ret     = -1;
    
    offset=ftell(fd);
    
    fwrite(&files,4,1,fd);
    for(i=0;i<files;i++) {
       x=ork_num(i,idx[i].size);
       y=ork_num(i,idx[i].zipsize);
       z=ork_num(i,idx[i].offset);
       fwrite(&idx[i].filecrc,4,1,fd);
       fwrite(&idx[i].namecrc,4,1,fd);
       fwrite(&x,             4,1,fd);
       fwrite(&y,             4,1,fd);
       fwrite(&z,             4,1,fd);
       len+=20;
       DBG printf("Debug: append idx\t%u\t%08X %08X\t%08X-%08X\t%08X\n",i,idx[i].filecrc,idx[i].namecrc,idx[i].size,idx[i].zipsize,idx[i].offset);
       }
    
    SHH(1) printf("-- copying index from temp to buffer\n");
    buff  = malloc(len);    
    if(!buff) std_err();

    fseek(fd,offset,SEEK_SET);
    fread(buff,len,1,fd);
        
    /*initialize the keys, then encode and set buffer to file*/
    SHH(1) printf("-- encoding index keys\n");
    ork_key_index_init(&key,keyset);
    ork_dec(ORK_DEC_CMP,&key,buff,len);
    
    SHH(1) printf("-- copying %u bytes to file\n",len);
    fseek(fd,offset,SEEK_SET);
    fwrite(buff,len,1,fd);
    
    ret=0;
    
quit:
    free(buff);
    /*fclose(temp);*/
    return(ret);
}



u8 *create_dir(u8 *name) {
    u8      *p,
            *l;

    for(p = name; (*p == '\\') || (*p == '/') || (*p == '.'); p++);
    name = p;

    for(;;) {
        if((p[0] && (p[1] == ':')) || ((p[0] == '.') && (p[1] == '.'))) p[0] = '_';

        l = strchr(p, '\\');
        if(!l) {
            l = strchr(p, '/');
            if(!l) break;
        }
        *l = 0;
        p = l + 1;
        make_dir(name);
        *l = PATHSLASH;
    }
    return(name);
}



void fd_write(u8 *name, u8 *data, int datasz) {
    FILE    *fd;

    name = create_dir(name);
    SHH(2) printf("- create file %s\n", name);
    if(!overwrite) {
        fd = fopen(name, "rb");
        if(fd) {
            fclose(fd);
            printf("- file already exists, do you want to overwrite it (y/N)?\n  ");
            fflush(stdin);
            if(yes_no() == 'n') return;
        }
    }
    fd = fopen(name, "wb");
    if(!fd) std_err();
    fwrite(data, datasz, 1, fd);
    fclose(fd);
    extracted++;
}



void fd_read(u8 *name, u8 *data, int datasz) {
    FILE    *fd;

    SHH(2) printf("- reading file %s\n", name);
    fd = fopen(name, "rb");
    if(!fd) std_err();
    fread(data, datasz, 1, fd);
    fclose(fd);
    extracted++;
}



u8 *strduplow(u8 *data) {
    u8      *p;

    data = (u8*) strdup(data);
    for(p = data; *p; p++) {
        *p = tolower(*p);
    }
    return(data);
}



int yes_no(void) {
    int     yn;

    for(;;) {
        yn = tolower(fgetc(stdin));
        if(yn == 'y') return('y');
        if(yn == 'n') return('n');
    }
}



void std_err(void) {
    perror("\nError");
    exit(1);
}


