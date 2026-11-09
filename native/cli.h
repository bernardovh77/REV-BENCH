/*
 * cli.h - what rb_cli's commands share (stage 121, when `sound` moved into
 * native/sound.c to drive the engine through rb.h alone).
 */
#ifndef RB_CLI_H
#define RB_CLI_H

#include <stdio.h>

/* rb_cli's help text, to f. */
void cli_usage(FILE *f);

/* The option's value after argv[*i] (moving *i on), or NULL if there's none. */
const char *cli_take_value(int argc, char **argv, int *i);

/* A quality level's number (RB_QUALITY_*) from its name, or -1. */
int cli_quality_from(const char *name);

/* Parses a finite number: 0, or -1 if s isn't one. */
int cli_to_double(const char *s, double *out);

/* rb_cli sound (native/sound.c): the exit code. */
int cli_sound(int argc, char **argv);

#endif /* RB_CLI_H */
