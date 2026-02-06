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
#include <windows.h>
#include "orkdec_files.h"



void winerr(void) {
    char    *error = NULL;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        GetLastError(),
        0,
        (char *)&error,
        0,
        NULL);
    if(error) {
        printf("\nError: %s\n", error);
        LocalFree(error);
    }
    printf("\nPress RETURN to quit\n");
    fgetc(stdin);
    exit(1);
}



int main(int argc, char *argv[]) {
	STARTUPINFO         si;
	PROCESS_INFORMATION pi;
    void    *dllmem;
    char    mypath[2000],
            *cmd = NULL,
            *dll;

    fputs(TITLE, stdout);

    if(argc < 2) {
        printf("\n"
            "Usage: %s <command> [dll]\n"
            "\n"
            "In short, just drag warhammer*.exe or exigo.exe over this executable and it\n"
            "will automatically launch the game for dumping all the filenames read by the\n"
            "game into a text file for orkdec.\n"
            "\n", argv[0]);
        printf("\nPress RETURN to quit\n");
        fgetc(stdin);
        exit(1);
    }

    cmd = argv[1];

    dll = DLL;
    if(argc > 2) dll = argv[2];

    if(!strchr(dll, ':')) {
        GetModuleFileName(NULL, mypath, sizeof(mypath) - 64);
        char *p = strrchr(mypath, '\\');
        if(p) *p = 0;
        strcat(mypath, "\\");
        strcat(mypath, dll);
        dll = mypath;
    }

    FILE *fd;
    fd = fopen(dll, "rb");
    if(!fd) {
        printf("\nError: the dll %s is not available\n", dll);
        goto quit;
    }
    fclose(fd);

    GetStartupInfo(&si);
    printf("execute:\n%s\n", cmd);
    if(!CreateProcess(NULL, cmd, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) winerr();

    dllmem = VirtualAllocEx(pi.hProcess, NULL, strlen(dll) + 1, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if(!dllmem) winerr();

    if(!WriteProcessMemory(pi.hProcess, dllmem, dll, strlen(dll) + 1, NULL)) winerr();

    if(!CreateRemoteThread(pi.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("Kernel32.dll"), "LoadLibraryA"), dllmem, 0, NULL)) winerr();

    ResumeThread(pi.hThread);

    printf("\nDLL %s injected\n", dll);
quit:
    printf("\n"
        "Press RETURN to quit\n"
        "this window is no longer needed so you can quit now\n");
    fgetc(stdin);
    return(0);
}


