/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * dia-win-remote.c
 * Copyright (C) 2002, 2004, 2006 Edward G. Bruck <ebruck@users.sourceforge.net>
 * Copyright (C) 2006, 2009 Steffen Macke <sdteffen@sdteffen.de>
 *
 * dia-win-remote is a program that allows the user running 2000/XP/Vista 
 * to open a dia file in an already running Dia process. If Dia 
 * isn't running, dia-win-remote will launch it and open the requested 
 * file(s).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * INCLUDES:
 */
#include <stdio.h> 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>

/**
 * PROTOTYPES:
 */
int LaunchDia(char* pszCmdLine);
int DragAndDropDia(HWND hWnd);
BOOL CALLBACK FindDiaWindow(HWND hWnd, LPARAM lParam);
void LoadRegSettings();
char gszDiaExe[_MAX_PATH] = {0};   /* exe name */
char gszVersion[] = "dia-win-remote 1.0.0";
BOOL gUseRegVal = FALSE;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                      LPSTR lpCmdLine, int nCmdShow)
{
    HWND hWnd = NULL; /* Dia's window */

    /**
     * load registry setting if it's available
     */
    LoadRegSettings();

    /**
     * was anything found in the registry?
     */
    gUseRegVal = (strlen(gszDiaExe) !=0);
    if (!gUseRegVal)
    {
        if (__argc < 3)
        {
            MessageBox(NULL, "Usage: dia-win-remote.exe diaw.exe file1 file2 ...",
                       gszVersion, MB_ICONSTOP);
            return -1;
        }
    }
    else
    {
        if (__argc < 2)
        {
            MessageBox(NULL, "Usage: dia-win-remote.exe file1 file2 ...",
                       gszVersion, MB_ICONSTOP);
            return -1;
        }
    }

    /**
     * Can't just look for "Dia" as other languages use
     * different title text. Lets do this the hard way
     * and examine all the windows.
     */
    EnumWindows(FindDiaWindow, (LPARAM)&hWnd);

    /**
     * Is Dia running?
     */
    if (hWnd != NULL)
    {
        return DragAndDropDia(hWnd);
    }
    else
    {
	/* pass second parameter */ 
        if (!gUseRegVal)
            return LaunchDia(strchr(lpCmdLine, ' '));
        else
            return LaunchDia(lpCmdLine);
    }
}


/* Find path to Dia in the registry and then launch it with the
   passed command line.*/
int LaunchDia(char* pszCmdLine)
{
    HKEY hRegData;
    char szAppPath[_MAX_PATH];   /* path to exe */
	char szDiaKey[255*2]; /* just incase * 2 */
    DWORD dwSize;
    DWORD dwType = 0;
    DWORD dwDisp;
    int iRetCode=-1;

    /* Read path to Dia */
    sprintf(szDiaKey, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\%s", 
            (gUseRegVal) ? gszDiaExe : __argv[1]);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szDiaKey, 0, KEY_READ, &hRegData) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szAppPath);
        if (RegQueryValueEx(hRegData, "", 0, &dwType,
                            (PBYTE)szAppPath, (LPDWORD)&dwSize) == ERROR_SUCCESS)
        {
            /**
	     * Extract the path from where diaw.exe was launched. Could
             * use the path in registry, but you never know where in the
             * string it will be.
	     */
            char szPath[_MAX_PATH]={0};
            char *pEnd = strrchr(szAppPath, '\\');

            /* Could we find last slash? */
            if (pEnd != NULL)
            {
                int len = pEnd-szAppPath;
                
                /* just incase */
                if (len > 0 && len < _MAX_PATH)
                {
                    strncpy(szPath, szAppPath, len);
                }
            }
          
            /**
	     * Try launching Dia with the passed params.
	     */
            if (ShellExecute(NULL, "open", szAppPath,
                             pszCmdLine, szPath, SW_SHOW) <= (HINSTANCE)32)
            {
                MessageBox(NULL, "Failed to launch Dia!", gszVersion, MB_ICONEXCLAMATION);
            }
            else
            {
                iRetCode = 0;
            }
        }
    }
    else
    {
        MessageBox(NULL, "Error reading registry!", gszVersion, MB_ICONSTOP);
    }

    /* close the key */
    RegCloseKey(hRegData);

    return iRetCode;
}


/**
 * Dia is running, so we simulate a drag & drop event.
 * Uses __argv & __argc for list of files to drop.
 */
int DragAndDropDia(HWND hWnd)
{
    int iNumFiles   = (gUseRegVal) ? __argc-1 : __argc-2;
    int iCurBytePos = sizeof(DROPFILES);
    LPDROPFILES pDropFiles;
    HGLOBAL hGlobal;
    int i;

    /* May use more memory than is needed... oh well. */
    hGlobal = GlobalAlloc(GHND | GMEM_SHARE,
                          sizeof(DROPFILES) +
                          (_MAX_PATH * iNumFiles) + 1);

    /* memory failure? */
    if (hGlobal == NULL)
        return -1;

    /* lock the memory */
    pDropFiles = (LPDROPFILES)GlobalLock(hGlobal);

    /* set offset where the file list begins */ 
    pDropFiles->pFiles = sizeof(DROPFILES);

    /* no wide chars and drop point is in client coordinates */
    pDropFiles->fWide = FALSE;
    pDropFiles->pt.x = pDropFiles->pt.y = 50;
    pDropFiles->fNC = FALSE;

    for(i=(gUseRegVal) ? 1 : 2; i < __argc; ++i)
    {
        strcpy(((LPSTR)(pDropFiles) + iCurBytePos), __argv[i]);
        /**
	 * Move the current position beyond the file name copied.
         * +1 for NULL terminator
	 */
        iCurBytePos += strlen(__argv[i]) + 1;
    }

    ((LPSTR)(pDropFiles))[iCurBytePos] = 0;
    GlobalUnlock(hGlobal);

    /* Force dia to the foreground */
    SetForegroundWindow(hWnd);

    /* restore only if minimized */
    if (IsIconic(hWnd))
    {
        ShowWindow(hWnd, SW_RESTORE);
    }

    /* send the file list */
    PostMessage(hWnd, WM_DROPFILES, (WPARAM)hGlobal, 0);

    return 0;
}

/**
 * Callback to enumerate all windows searching for
 * Dia as the user could be running under a different language.
 */ 
BOOL CALLBACK FindDiaWindow(HWND hWnd, LPARAM lParam)
{
    char szTitle[MAX_PATH + 40]; /* should be enough to find diaw.exe */

    if (GetWindowText(hWnd, szTitle, sizeof(szTitle)) != 0)
    {
        /* look for Dia */
        char* pszToken = strtok(szTitle, " ");
        while(pszToken != NULL)
        {
            if (stricmp(pszToken,"diaw.exe") == 0)
            {
                /* sanity check class name */         
                char szClass[20]; /* should be enough */

                if (GetClassName(hWnd, szClass, sizeof(szClass)) != 0)
                {
                    /* For some reason the latest revs of Dia have a hidden
                     * window that we may be brought to the front which causes Dia 
                     * to appear only in the tray. 
		     */ 
                    if (IsWindowVisible(hWnd))
                    {
                        /* if ok, then stop enumerating */
                        if (stricmp(szClass, "gdkWindowTopLevel") == 0)
                        {
                            *((HWND*)lParam) = hWnd;
                            return FALSE;
                        }
                    }
                }
            }
            /* get next token */
            pszToken = strtok(NULL, " ");
        }
    }
     
    return TRUE;
}

void LoadRegSettings()
{   
    HKEY hRegData;

    /** Many users have had problems using older versions and wanting to send
     * files from a third party application. Having this value in the registry
     * will allow users to send files without specifying the dia version to run.
     */
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\GNU\\dia-win-remote", 
                     0, KEY_READ, &hRegData) == ERROR_SUCCESS)
    {
        DWORD dwSize = sizeof(gszDiaExe);
        DWORD dwType = 0;
        RegQueryValueEx(hRegData, "", 0, &dwType,
                        (PBYTE)gszDiaExe, (LPDWORD)&dwSize);
    }

    /* close the key */
    RegCloseKey(hRegData);
}

