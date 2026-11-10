/*
 * rbn_win_delay_load.c - Windows only (stage 124): where the add-on finds the
 * Node-API functions.
 *
 * The add-on is linked against node.lib, an import library for "node.exe", and
 * that import is delay-loaded (/DELAYLOAD:node.exe). The hook below answers
 * the loader's request for that DLL with the running process itself, whatever
 * it is called: node.exe in Node, electron.exe (or the app's renamed
 * executable) in Electron, which exports the same napi_* functions. So one
 * .node serves both, and the application's executable may be renamed.
 */
#ifdef _MSC_VER

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <delayimp.h>
#include <string.h>

/* The delay-load helper calls this for each of its events. */
static FARPROC WINAPI rbn_load_exe_hook(unsigned int event, PDelayLoadInfo info)
{
    if (event != dliNotePreLoadLibrary) return NULL;
    if (_stricmp(info->szDll, "node.exe") != 0) return NULL;
    return (FARPROC)GetModuleHandleA(NULL);
}

const PfnDliHook __pfnDliNotifyHook2 = rbn_load_exe_hook;

#endif /* _MSC_VER */
