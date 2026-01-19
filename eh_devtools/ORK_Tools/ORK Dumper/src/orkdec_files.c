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
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <direct.h>
#include <windows.h>

#include "orkdec_files.h"

#define MAINPROG
#include "disasm.h"

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;



FILE    *fd = NULL;



#define BACKUP_REGISTERS    8



void __cdecl orkdec_files(void *stack, ...) {
    static int more_args = -1;  // work-around for Battle March
    int     i,
            filenamesz;
    u8      *filename;

    for(i = 0; i < BACKUP_REGISTERS; i++) {
                                    stack = (u8 *)stack + sizeof(void *);
    }
    /* ret_addr */                  stack = (u8 *)stack + sizeof(void *);
    /* arg 1 */                     stack = (u8 *)stack + sizeof(void *);
    if(more_args > 0)               stack += more_args;
    /* keyset1 */                   stack = (u8 *)stack + sizeof(void *);
    /* keyset2 */                   stack = (u8 *)stack + sizeof(void *);
    /* keyset3 */                   stack = (u8 *)stack + sizeof(void *);
    filename   = *(u8 **)stack;     stack = (u8 *)stack + sizeof(void *);
    filenamesz = *(int *)stack;     stack = (u8 *)stack + sizeof(void *);

    if(more_args < 0) {
        // the additional argument of BM will lead to filenamesz containing the filename value
        more_args = 0;
        for(;;) {
            if((filenamesz > 4) && (filenamesz <= MAX_PATH)) {
                break;  // probably found
            }
            filename   = *(u8 **)((u8 *)stack - sizeof(void *));  // filename = filenamesz
            filenamesz = *(int *)stack;
                                    stack = (u8 *)stack + sizeof(void *);
            more_args += sizeof(void *);
        }
    }

    fprintf(fd, "%.*s\r\n", filenamesz, filename);
    fflush(fd);
}



void __cdecl orkdec_files2(void *stack, ...) {
    int     i,
            filenamesz;
    u8      *filename;

    for(i = 0; i < BACKUP_REGISTERS; i++) {
                                    stack = (u8 *)stack + sizeof(void *);
    }
    /* ret_addr */                  stack = (u8 *)stack + sizeof(void *);
    filename   = *(u8 **)stack;     stack = (u8 *)stack + sizeof(void *);
    filenamesz = *(int *)stack;     stack = (u8 *)stack + sizeof(void *);

    fprintf(fd, "%.*s\r\n", filenamesz, filename);
    fflush(fd);
}



int jmp(u8 *buff, void *from, void *to) {
    buff[0] = 0xe9;
    *(u32 *)(buff + 1) = to - (from + 5);
    return(5);
}

int call(u8 *buff, void *from, void *to) {
    buff[0] = 0xe8;
    *(u32 *)(buff + 1) = to - (from + 5);
    return(5);
}

int mm(u8 *dst, u8 *src, int len) {
    memcpy(dst, src, len);
    return(len);
}

void *find_bytes(void *buff, int buffsz, u8 *data, int datalen) {
    u8      *p,
            *l;

    l = buff + buffsz - datalen;
    for(p = buff; p <= l; p++) {
        if(!memcmp(p, data, datalen)) return(p);
    }
    return(NULL);
}



int get_current_section(IMAGE_NT_HEADERS *nthdr, IMAGE_SECTION_HEADER **sechdr, u32 file) {
    int     i;

    for(i = 0; i < nthdr->FileHeader.NumberOfSections; i++) {
        if((file >= sechdr[i]->PointerToRawData) && (file < (sechdr[i]->PointerToRawData + sechdr[i]->SizeOfRawData))) {
            return(i);
        }
    }
    return(0);
}

void *parse_PE(void *filemem, u32 *filesize) {
    IMAGE_DOS_HEADER        *doshdr;
    IMAGE_NT_HEADERS        *nthdr;
    IMAGE_SECTION_HEADER    **sechdr;
    int     i,
            sec;
    u8      *p;

    p = filemem;
    doshdr  = (IMAGE_DOS_HEADER *)p;    p += sizeof(IMAGE_DOS_HEADER);
    /*dosdata = p;*/                    p += doshdr->e_lfanew - sizeof(IMAGE_DOS_HEADER);
    nthdr   = (IMAGE_NT_HEADERS *)p;    p += sizeof(IMAGE_NT_HEADERS);

    if((doshdr->e_magic != IMAGE_DOS_SIGNATURE) || (nthdr->Signature != IMAGE_NT_SIGNATURE)) {
        return(NULL);
    }
    if((nthdr->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) || (nthdr->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)) {
        return(NULL);
    }

    sechdr = calloc(sizeof(IMAGE_SECTION_HEADER *), nthdr->FileHeader.NumberOfSections);
    for(i = 0; i < nthdr->FileHeader.NumberOfSections; i++) {
        sechdr[i] = (IMAGE_SECTION_HEADER *)p;
        p += sizeof(IMAGE_SECTION_HEADER);
    }

    sec = get_current_section(nthdr, sechdr, nthdr->OptionalHeader.AddressOfEntryPoint);
    if(sec < 0) {
        fprintf(stderr, "\nAlert: the entrypoint is outside the file, I scan the first section\n");
        sec = 0;
    }

    filemem  += sechdr[sec]->PointerToRawData;
    *filesize = sechdr[sec]->SizeOfRawData;

    free(sechdr);
    return(filemem);
}



// directly from orkdec
typedef struct {
    uint32_t    k1;
    uint32_t    k2;
    uint32_t    k3;
} ork_key_t;
int ork_key_file_init(ork_key_t *key, uint8_t *fname, int ork_keyset) {
    if(ork_keyset == 0) {
        key->k1 = 0x488BCEA3;
        key->k2 = 0xFB598BF2;
        key->k3 = 0xF89197A5;
    } else if(ork_keyset == 1) {
        key->k1 = 0xd3f7454d;
        key->k2 = 0x7da7b991;
        key->k3 = 0xc38e2892;
    } else if(ork_keyset == 2) {
        key->k1 = 0xDD4F2048;
        key->k2 = 0x9189C8FB;
        key->k3 = 0x4A7CA962;
    } else {
        return(-1);
    }

    //ork_dec(0, key, fname, strlen(fname));
    return(0);
}



void orkdec_init_fail(void) {
    MessageBox(0,
        TITLE
        "Error: the needed function has not been found in the process\n"
        "Remember that you need to use the no-cd executable because the original is encrypted\n",
        HEAD,
        MB_OK | MB_TASKMODAL);
    exit(1);
}



void orkdec_init(void) {
    u8      *orkdec_off     = NULL;
    u8      *orkdec_jmp     = NULL;

    ork_key_t   key;
    DWORD   oldvp;
    u32     memsize;
    int     i,
            len,
            patch_method,
            restore_bytes;
    u8      buff[MAX_PATH + 512],
            off_str[100] = "",
            mypath[2000],
            tmp[32],
            *p;
    void    *mem,
            *func;

    mem = parse_PE(GetModuleHandle(NULL), &memsize);
    if(!mem) orkdec_init_fail();

    static int init = 0;    // the disasm engine is not necessary in this game but it's good for copy&paste in other proejcts
    static t_config    asm_cfg;
    t_disasm da;
    if(!init) {
        init = 1;
        Preparedisasm();
        memset(&asm_cfg, 0, sizeof(asm_cfg));
        asm_cfg.showmemsize = 1;    // Always show memory size
    }

    for(patch_method = 0;; patch_method++) {
        if(patch_method == 0) {
            func = orkdec_files;

            // find the constant keys
            for(i = 0;; i++) {
                if(ork_key_file_init(&key, NULL, i) < 0) {
                    orkdec_init_fail();
                }

                p = buff;
                *p++ = 0x68;    *(u32 *)p = key.k3; p += sizeof(u32);   // push KEY3
                *p++ = 0x68;    *(u32 *)p = key.k2; p += sizeof(u32);   // push KEY2
                *p++ = 0x68;    *(u32 *)p = key.k1; p += sizeof(u32);   // push KEY1

                orkdec_off = find_bytes((void *)mem, memsize, (void *)buff, p - buff);
                if(orkdec_off) {
                    orkdec_off += (p - buff);
                    break;
                }
            }

            // find the call
            for(;;) {
                len = Disasm(orkdec_off, memsize - ((void *)orkdec_off - mem), (int)orkdec_off, &da, DA_TEXT, &asm_cfg, NULL);
                if((da.cmdtype & D_CMDTYPE) == D_CALL) break;
                orkdec_off += len;
            }

            orkdec_off += 1;    // 0xe8
            orkdec_off += sizeof(u32) + (*(u32 *)orkdec_off);   // go in the function

        } else if(patch_method == 1) {
            func = orkdec_files2;

            p = buff;
            *p++ = 0x04;
            *(u32 *)p = 0xa4d6cf31; p += sizeof(u32);
            orkdec_off = find_bytes((void *)mem, memsize, (void *)buff, p - buff);
            if(!orkdec_off) break;  // skip
            while((void *)orkdec_off >= mem) {
                if((orkdec_off[0] == 0xcc) && (orkdec_off[1] == 0xcc)) break;
                orkdec_off--;
            }
            orkdec_off += 2;

        } else {
            break;
        }
        if(!orkdec_off) break;

        // reserve space for the jmp to the patch
        p = orkdec_off;
        restore_bytes = 0;
        while(restore_bytes < (1 + sizeof(u32))) {
            len = Disasm(p, memsize - ((void *)p - mem), (int)p, &da, DA_TEXT, &asm_cfg, NULL);
            p += len;
            restore_bytes += len;
        }
        memcpy(tmp, orkdec_off, restore_bytes);

        // reserve space for the patch
        SYSTEM_INFO sSysInfo;
        GetSystemInfo(&sSysInfo);
        orkdec_jmp = VirtualAlloc(
            NULL,
            sSysInfo.dwPageSize,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_EXECUTE_READWRITE);    // write for memcpy

        // jmp to patch
        p = buff;
        p += jmp(p, (void *)orkdec_off, (void *)orkdec_jmp);
        while((p - buff) < restore_bytes) *p++ = 0x90;
        VirtualProtect((void *)orkdec_off, p - buff, PAGE_EXECUTE_READWRITE, &oldvp);
        WriteProcessMemory(GetCurrentProcess(), (void *)orkdec_off, buff, p - buff, NULL);
        VirtualProtect((void *)orkdec_off, p - buff, oldvp, &oldvp);

        // patch
        p = orkdec_jmp;
        for(i = 0; i < BACKUP_REGISTERS; i++) {
            *p++ = 0x50 + i;
        }
        *p++ = 0x54;
        p += call(p, p, (void *)func);
        *p++ = 0x5c;
        for(i = BACKUP_REGISTERS - 1; i >= 0; i--) {
            *p++ = 0x58 + i;
        }
        p += mm(p, (void *)tmp, restore_bytes);
        p += jmp(p, (void *)p, (void *)orkdec_off + restore_bytes);

        sprintf(off_str + strlen(off_str), "%p   (%08x)\n", orkdec_off, (void *)orkdec_off - mem);
    }

    // file
    GetModuleFileName(GetModuleHandle(DLL), mypath, sizeof(mypath) - 64);
    p = strrchr(mypath, '\\');
    if(p) *p = 0;
    sprintf(mypath + strlen(mypath), "\\%08x_orkdec_files.txt", (u32)time(NULL));

    sprintf((char *)buff,
        TITLE
        "DLL successfully injected and ready to work.\n"
        "\n"
        "Filename function:\n"
        "%s\n"
        "\n"
        "Dump file:\n"
        "%s"
        "\n"
        "Do you want to continue?\n",
        off_str,
        mypath);

    if(MessageBox(0,
        (char *)buff,
        HEAD,
        MB_OKCANCEL | MB_TASKMODAL) == IDCANCEL) exit(1);

    fd = fopen(mypath, "wb");
    if(!fd) exit(1);
}



BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReason, LPVOID lpReserved) {
    switch(ulReason) {
        case DLL_PROCESS_ATTACH: {
            DisableThreadLibraryCalls(hModule);
            orkdec_init();
            break;
        }
        case DLL_PROCESS_DETACH: {
            if(fd) fclose(fd);
            break;
        }
        default: break;
    }
    return(TRUE);
}


