/*
 * win_sampler.c - a small sampling profiler for Windows (stage 118).
 *
 *   cl /nologo /O2 /Zi tools\win_sampler.c /Fe:win_sampler.exe dbghelp.lib winmm.lib
 *   win_sampler [-i MS] [-n TOP] [-o FILE] -- program.exe args...
 *
 * Windows' own sampler (WPR, xperf) comes with the Windows ADK, which a
 * developer's machine may not have. This one needs only DbgHelp, which ships
 * with Windows: it starts the program, and every MS milliseconds (default 1)
 * suspends its main thread, reads where it was (the instruction pointer),
 * and lets it go. When the program ends, each sample is turned into the name
 * of the function that holds it, with the program's .pdb (an MSVC build with
 * /Zi, as CMake's Release build has) and the samples are counted by
 * function: the flat profile, as gprof's. With link-time optimisation
 * (/GL) the compiler has inlined small functions into their callers, so a
 * sample counts for the function it ended up in.
 *
 * It samples the main thread only (rb_bench_render and rb_cli run on one),
 * which costs the program a little speed. The program's own output goes on
 * as it is; the profile follows it on stderr, or into FILE.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct entry {
    char name[256];
    long count;
} entry;

static int by_count(const void *a, const void *b)
{
    return (int)(((const entry *)b)->count - ((const entry *)a)->count);
}

int main(int argc, char **argv)
{
    int interval = 1, top = 30, split = -1;
    const char *out_path = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--")) {
            split = i + 1;
            break;
        } else if (!strcmp(argv[i], "-i") && i + 1 < argc) {
            interval = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-n") && i + 1 < argc) {
            top = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-o") && i + 1 < argc) {
            out_path = argv[++i];
        }
    }
    if (split < 0 || split >= argc || interval < 1) {
        fprintf(stderr, "usage: win_sampler [-i MS] [-n TOP] [-o FILE] -- program.exe args...\n");
        return 2;
    }
    char cmd[8192] = "";
    for (int i = split; i < argc; i++) {
        strcat(cmd, i > split ? " \"" : "\"");
        strcat(cmd, argv[i]);
        strcat(cmd, "\"");
    }
    STARTUPINFOA si = { sizeof si };
    PROCESS_INFORMATION pi;
    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "win_sampler: cannot start %s\n", argv[split]);
        return 1;
    }
    timeBeginPeriod(1);
    char dir[MAX_PATH];
    strncpy(dir, argv[split], sizeof dir - 1);
    dir[sizeof dir - 1] = 0;
    char *slash = strrchr(dir, 92);   /* the backslash */
    char *fslash = strrchr(dir, '/');
    if (fslash > slash) slash = fslash;
    if (slash) *slash = 0; else strcpy(dir, ".");
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);
    const long max_samples = 4000000;
    DWORD64 *rip = malloc((size_t)max_samples * sizeof *rip);
    long n = 0;
    int symbols = 0;
    while (n < max_samples && WaitForSingleObject(pi.hProcess, (DWORD)interval) == WAIT_TIMEOUT) {
        if (SuspendThread(pi.hThread) == (DWORD)-1) continue;
        CONTEXT ctx;
        ctx.ContextFlags = CONTEXT_CONTROL;
        if (GetThreadContext(pi.hThread, &ctx)) rip[n++] = ctx.Rip;
        ResumeThread(pi.hThread);
        /* Once the program has loaded, DbgHelp reads its modules' symbols:
         * with the process handle as the session's key, which it keeps. */
        if (!symbols && n == 200) symbols = SymInitialize(pi.hProcess, dir, TRUE);
    }
    if (!symbols) symbols = SymInitialize(pi.hProcess, dir, TRUE);
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    timeEndPeriod(1);
    HANDLE self = pi.hProcess;

    entry *table = calloc((size_t)(n > 0 ? n : 1), sizeof *table);
    long used = 0;
    char buf[sizeof(SYMBOL_INFO) + 256];
    for (long k = 0; k < n; k++) {
        SYMBOL_INFO *sym = (SYMBOL_INFO *)buf;
        memset(sym, 0, sizeof *sym);
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 255;
        DWORD64 disp;
        const char *name = SymFromAddr(self, rip[k], &disp, sym) ? sym->Name : "(unknown)";
        long j = 0;
        while (j < used && strcmp(table[j].name, name)) j++;
        if (j == used) {
            strncpy(table[used].name, name, sizeof table[used].name - 1);
            used++;
        }
        table[j].count++;
    }
    qsort(table, (size_t)used, sizeof *table, by_count);
    FILE *out = out_path ? fopen(out_path, "w") : stderr;
    if (!out) out = stderr;
    fprintf(out, "win_sampler: %ld samples at %d ms; exit code %lu\n", n, interval, code);
    fprintf(out, "  %%time  samples  function\n");
    for (long j = 0; j < used && j < top; j++)
        fprintf(out, "%7.2f %8ld  %s\n", 100.0 * table[j].count / (n > 0 ? n : 1), table[j].count, table[j].name);
    if (out != stderr) fclose(out);
    return (int)code;
}
