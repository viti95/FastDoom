/*
 * OPTIONS.H - FastDoom text mode launcher, command line options
 */

#ifndef OPTIONS_H
#define OPTIONS_H

/*
 * A command line option that can be passed to the game
 * executables. def_value is NULL for the simple flags; for the
 * options that take a value it is non NULL (always an empty
 * string: there is no default, the user must type the value in
 * the prompt when the option is enabled).
 */
typedef struct {
    const char *arg;
    const char *desc;
    const char *def_value;
} opt_t;

/* The available options (defined in options.c). */
#define NUMOPTS 97

extern const opt_t opts[NUMOPTS];

/* Max length of an option value (the characters after the option
   name on the command line). */
#define OPT_VALUE_MAX 32

/* DOS command lines are short: build_command() stops appending if
   the command would grow beyond this many characters. */
#define MAX_CMD_LEN 120

void load_options(void);
void save_options(void);
void save_launch_exe(const char *exe);
int load_launch_exe(char *exe, int size);
int is_game_exe(const char *exe);
void build_command(const char *exe, char *cmd);
int command_length(const char *exe);

/* options_menu() return values */
#define OM_BACK   0 /* went back (Esc/Left) */
#define OM_QUIT   1 /* quit the launcher (Q) */
#define OM_SAVED  2 /* saved with S and closed */

int options_menu(void);

#endif /* OPTIONS_H */
