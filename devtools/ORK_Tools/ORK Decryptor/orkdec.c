/*
    Copyright 2007-2015 Luigi Auriemma

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
#include <zlib.h>       // -lz
#include "ork_algo.h"

#ifndef MAX_PATH
#define MAX_PATH 260
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



#define VER     "0.1.3"



void delimit(u8 *data);
int list_file(FILE *fd, ork_index_t *idx, u32 files, u8 *fname);
void check_keyset(FILE *fd);
void special_mode(FILE *fd, ork_index_t *idx, u32 files);
int orc_extract(FILE *fd, u8 *password);
int ork_extract(FILE *fd, ork_index_t *idx, u32 files, u8 *fname, u8 **retbuff, u32 *retsize);
u8 *create_dir(u8 *name);
void fd_write(u8 *name, u8 *data, int datasz);
u8 *strduplow(u8 *data);
int yes_no(void);
void std_err(void);



int     g_keyset,
        g_overwrite = 0,
        g_extracted = 0;
u8      *g_folder   = NULL;



u8      *g_orc_keys[] = {
            "WgQgJ3vyYvAAkHHhFdueKh2ssZli8Ko4", // Might & Magic Heroes VI
            "q7BS7XalNIWp8HNzkclPKZlhdm1vVCc6", // Might & Magic Heroes VI demo
            "QKVOognDaUlxyJHko8el6XlXknGfaRfv", // Supernova
            NULL
        };



#define ZIP_PK1 0x04034b50	/* local_dir */
#define ZIP_PK2	0x02014b50	/* central_dir */
#define ZIP_PK3	0x06054b50	/* end_central_dir */

#pragma pack(2)
typedef struct {
    u32 magic;
    u16 ver;
    u16 flag;
    u16 method;
    u32 timedate;
    u32 crc;
    u32 comp_size;
    u32 uncomp_size;
    u16 name_len;
    u16 extra_len;
    /* filename */
    /* extra */
} local_dir_t;

typedef struct {
    u32 magic;
    u16 ver_made;
    u16 ver_ext;
    u16 flag;
    u16 method;
    u32 timedate;
    u32 crc;
    u32 comp_size;
    u32 uncomp_size;
    u16 name_len;
    u16 extra_len;
    u16 comm_len;
    u16 disk_start;
    u16 int_attr;
    u32 ext_attr;
    u32 offset;
    /* filename */
    /* extra */
    /* comment */
} central_dir_t;

typedef struct {
    u32 magic;
    u16 disk_num;
    u16 disk_num_start;
    u16 tot_ent_disk;
    u16 tot_ent_dir;
    u32 dir_size;
    u32 offset;
    u16 comm_len;
    /* comment */
} end_central_dir_t;
#pragma pack()



int main(int argc, char *argv[]) {
    ork_key_t   ork_key;
    ork_index_t *ork_index  = NULL;
    FILE    *fd;
    u32     i,
            files,
            offset,
            magic;
    int     min_arg     = 2;
    u8      sign[10],
            *infile,
            *listfile,
            *ext;

    setbuf(stdout, NULL);

    fputs("\n"
        "ORK files decrypter and extractor "VER"\n"
        "by Luigi Auriemma\n"
        "e-mail: me@aluigi.org\n"
        "web:    aluigi.org\n"
        "\n", stdout);

    infile = argv[argc - 1];
    ext = strrchr(infile, '.');
    if(ext) {
        if(!stricmp(ext, ".orc")) min_arg = 1;
    }

    if(argc < (1 + min_arg)) {
        printf("\n"
            "Usage: %s [options] <file.ORK/ORC> <file_to_extract/list.LST>\n"
            "\n"
            "Options:\n"
            "-d DIR   all the files will be extracted in the DIR folder\n"
            "-o       automatically overwrite any file if already exist\n"
            "\n"
            "The ORK files are archives without an index for the archived filenames.\n"
            "Each file is encrypted using just its name as password so without knowing this\n"
            "name you cannot extract it.\n"
            "If you want to know the names of these files (no names, no extraction) you need\n"
            "to use my orkdec_files program available here:\n"
            "  http://aluigi.org/papers.htm#orkdec_files\n"
            "Use the list file \"\" to have some raw information and list of the files.\n"
            "\n"
            "The ORC files used in Might & Magic Heroes VI and Supernova instead are just\n"
            "encrypted ZIP files so no \"file_to_extract\" is necessary and you can just\n"
            "specify a \"\".\n"
            "\n", argv[0]);
        exit(1);
    }

    argc -= min_arg;
    for(i = 1; i < argc; i++) {
        if(((argv[i][0] != '-') && (argv[i][0] != '/')) || (strlen(argv[i]) != 2)) {
            printf("\nError: wrong argument (%s)\n", argv[i]);
            exit(1);
        }
        switch(argv[i][1]) {
            case 'd': g_folder      = argv[++i];    break;
            case 'o': g_overwrite   = 1;            break;
            default: {
                printf("\nError: wrong argument (%s)\n", argv[i]);
                exit(1);
            }
        }
    }

    infile = argv[argc];
    ext = strrchr(infile, '.');
    if(!ext) ext = "";

    listfile = NULL;
    if(min_arg >= 2) {
        listfile = argv[argc + 1];
        if(!listfile[0]) listfile = NULL;
    }

    printf("- open file %s\n", infile);
    fd = fopen(infile, "rb");
    if(!fd) std_err();

    if(!stricmp(ext, ".orc")) {

        // Might & Magic Heroes VI
        // Supernova

        for(i = 0; g_orc_keys[i]; i++) {
            orc_read(fd, NULL, (void *)&magic, sizeof(magic), 0, g_orc_keys[i]);
            if(magic == ZIP_PK1) break;  // ZIP local directory
        }
        if(!g_orc_keys[i]) {
            printf("\nError: no valid ORC keys available in the database, contact me.\n");
            exit(1);
        }
        fseek(fd, 0, SEEK_SET);

        if(g_folder) {
            printf("- output folder %s\n", g_folder);
            if(chdir(g_folder) < 0) std_err();
        }

        orc_extract(fd, g_orc_keys[i]);

    } else {

        // Armies of Exigo
        // Mark of Chaos
        // Battle March

        if(fseek(fd, -(long)sizeof(sign), SEEK_END) < 0) std_err();
        fread(sign, sizeof(sign), 1, fd);
        if(memcmp(sign, ORK_SIGN, sizeof(sign))) {
            printf("\nError: the file doesn't seem a valid ORK archive!\n");
        }

        if(fseek(fd, 4, SEEK_SET) < 0) std_err();

        fread(&offset, 4, 1, fd);
        offset ^= ORK_OFFSET;

        printf("- index offset 0x%08x\n", offset);
        if(fseek(fd, offset, SEEK_CUR) < 0) std_err();

        printf("- check if the archive is for the demo\n");
        check_keyset(fd);

        printf("- initialize the index keys\n");
        ork_key_index_init(&ork_key, g_keyset);

        ork_read(fd, &ork_key, (void *)&files, 4);

        ork_index = malloc(sizeof(ork_index_t) * files);
        if(!ork_index) std_err();

        printf("- read index\n");

        for(i = 0; i < files; i++) {
            ork_read(fd, &ork_key, (void *)&ork_index[i], sizeof(ork_index_t));

            ork_index[i].size    = ork_num(i, ork_index[i].size);
            ork_index[i].zipsize = ork_num(i, ork_index[i].zipsize);
            ork_index[i].offset  = ork_num(i, ork_index[i].offset);
        }

            // list_file and change folder
        if(!list_file(fd, ork_index, files, listfile)) {
            goto quit;
        }

        if(listfile[0]) {
            ork_extract(fd, ork_index, files, listfile, NULL, NULL);

        } else {
            printf("\n"
                "  num   CRC???   name_CRC offset   file_size  zip_size\n"
                "--------------------------------------------------------\n");

            for(i = 0; i < files; i++) {
                printf("  %-5u %08x %08x %08x %-10u %-10u\n",
                    i,
                    ork_index[i].filecrc,
                    ork_index[i].namecrc,
                    ork_index[i].offset,
                    ork_index[i].size,
                    ork_index[i].zipsize);
            }
        }
    }

quit:
    fclose(fd);
    if(ork_index) free(ork_index);
    printf("\n- %u files extracted\n", g_extracted);
    return(0);
}



void delimit(u8 *data) {
    while(*data && (*data != '\n') && (*data != '\r')) data++;
    *data = 0;
}



int list_file(FILE *fd, ork_index_t *idx, u32 files, u8 *fname) {
    FILE    *fdl;
    u8      buff[MAX_PATH + 1],
            *p;

    p = strrchr(fname, '.');
    if(!p) return(-1);
    if(stricmp(p + 1, "lst") && stricmp(p + 1, "txt")) return(-1);

    printf("- use %s as a list of files to extract\n", fname);
    fdl = fopen(fname, "rb");
    if(g_folder) {
        printf("- output folder %s\n", g_folder);
        if(chdir(g_folder) < 0) std_err();
    }
    if(!fdl) return(-1);

    while(fgets(buff, sizeof(buff), fdl)) {
        delimit(buff);

        ork_extract(fd, idx, files, buff, NULL, NULL);
    }

    fclose(fdl);
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

    g_keyset = 0;

    fstat(fileno(fd), &xstat);

redo:
    printf("- check key set %u\n", g_keyset);

    set_end = ork_key_index_init(&ork_key, g_keyset);
    ork_read(fd, &ork_key, (void *)&files, 4);
    fseek(fd, -4, SEEK_CUR);

    our_size   = files * sizeof(ork_index_t);
    right_size = xstat.st_size - 10 - (ftell(fd) + 4);
        //       |               |    |
        //       |               |    where the index starts
        //       |               signature
        //       total size of the file

    if(our_size != right_size) {
        printf("- index of %u bytes while should be %u\n", our_size, right_size);
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
        g_keyset++;
        goto redo;
    }
}



#define UNZIP_BASE(NAME,WBITS) \
int NAME(u8 *in, int insz, u8 *out, int outsz) { \
    static z_stream *z  = NULL; \
    int     ret, \
            sync = Z_FINISH; \
    \
    if(!in && !out) { \
        if(z) { \
            inflateEnd(z); \
            free(z); \
        } \
        z = NULL; \
        return -1; \
    } \
    \
    if(!z) { \
        z = calloc(sizeof(z_stream), 1); \
        if(!z) return -1; \
        memset(z, 0, sizeof(z_stream)); \
        if(inflateInit2(z, WBITS)) { \
            fprintf(stderr, "\nError: zlib initialization error\n"); \
            return -1; \
        } \
    } \
    inflateReset(z); \
    \
    z->next_in   = in; \
    z->avail_in  = insz; \
    z->next_out  = out; \
    z->avail_out = outsz; \
    ret = inflate(z, sync); \
    if(ret != Z_STREAM_END) return -1; \
    return z->total_out; \
}
UNZIP_BASE(unzip_zlib,    15)
UNZIP_BASE(unzip_deflate, -15)



int orc_extract(FILE *fd, u8 *password) {
    local_dir_t         local_dir;
    central_dir_t       central_dir;
    end_central_dir_t   end_central_dir;
    ork_key_t orc_key;
    u32     magic;
    u8      *name   = NULL;
    u8      *zbuff  = NULL,
            *buff   = NULL;

    name = malloc(0xffff + 1);  // for the names
    if(!name) std_err();

    for(;;) {
        // magic scanner, probably useless but it's ok
        for(;;) {
            if(orc_read(fd, NULL, (void *)&magic, sizeof(u32), ftell(fd), password) < 0) goto quit;
            if(magic == ZIP_PK1) {
                break;
            } else if(magic == ZIP_PK2) {
                fseek(fd, -4, SEEK_CUR);
                orc_read(fd, NULL, (void *)&central_dir, sizeof(central_dir), ftell(fd), password);
                fseek(fd, central_dir.name_len + central_dir.extra_len + central_dir.comm_len, SEEK_CUR);
            } else if(magic == ZIP_PK3) {
                fseek(fd, -4, SEEK_CUR);
                orc_read(fd, NULL, (void *)&end_central_dir, sizeof(end_central_dir), ftell(fd), password);
                fseek(fd, end_central_dir.comm_len, SEEK_CUR);
            } else {
                fseek(fd, -3, SEEK_CUR);    // byte per byte
            }
        }

        fseek(fd, -4, SEEK_CUR);
        orc_read(fd, &orc_key, NULL, 0, ftell(fd), password);
        orc_read(fd, &orc_key, (void *)&local_dir, sizeof(local_dir), -1, password);
        if(local_dir.magic != ZIP_PK1) break;

        orc_read(fd, &orc_key, name, local_dir.name_len, -1, password);
        name[local_dir.name_len] = 0;

        fseek(fd, local_dir.extra_len, SEEK_CUR);   // skip if existent (no need to decrypt it)

        // allocate memory
        zbuff = realloc(zbuff, local_dir.comp_size);
        if(!zbuff) std_err();
        buff = realloc(buff, local_dir.uncomp_size);
        if(!buff) std_err();

        if(!local_dir.method) {
            fread(buff, 1, local_dir.uncomp_size, fd);

        } else if(local_dir.method == 8) {
            fread(zbuff, 1, local_dir.comp_size, fd);
            unzip_deflate(zbuff, local_dir.comp_size, buff, local_dir.uncomp_size);

        } else {
            printf("\nError: invalid ZIP method %d\n", local_dir.method);
            exit(1);
        }

        fd_write(name, buff, local_dir.uncomp_size);
    }

quit:
    if(buff) free(buff);
    if(zbuff) free(zbuff);
    if(name) free(name);
    return 0;
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

    fname = strduplow(fname);       // in case it's a fixed buffer

    crc = ork_crc(fname, strlen(fname));
    if(check_duplicates(crc) < 0) goto quit;

    printf("\n"
        "- search \"%s\" (%08x)\n",
        fname, crc);

    for(i = 0; i < files; i++) {
        if(idx[i].namecrc == crc) break;
    }
    if(i == files) {
        printf("  Error: file not found\n");
        goto quit;
    }
    idx = &idx[i];

    zlen = idx->zipsize;
    len  = idx->size;

    zbuff = malloc(zlen);
    buff  = malloc(len);
    if(!zbuff || !buff) std_err();

    fseek(fd, 8 + idx->offset, SEEK_SET);

    printf("- decrypt data\n");
    ork_key_file_init(&key, fname, g_keyset);
    ork_read(fd, &key, zbuff, zlen);

    printf("- unzip data\n");
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
        buff     = NULL;    // for not freeing it
    }
    ret = 0;

quit:
    if(buff)  free(buff);
    if(zbuff) free(zbuff);
    free(fname);
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
    printf("  %s\n", name);
    if(!g_overwrite) {
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
    g_extracted++;
}



u8 *strduplow(u8 *data) {
    u8      *p;

    data = strdup(data);
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


