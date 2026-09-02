/*
 * OPTIONS.H - FastDoom text mode launcher, command line options
 */

#ifndef OPTIONS_H
#define OPTIONS_H

/*
 * A command line option that can be passed to the game
 * executables.
 */
typedef struct {
    const char *arg;
    const char *desc;
} opt_t;

/* The available options (defined in options.c). */
#define NUMOPTS 50

extern const opt_t opts[NUMOPTS];

/* DOS command lines are short: build_command() stops appending if
   the command would grow beyond this many characters. */
#define MAX_CMD_LEN 120

void load_options(void);
void save_options(void);
int is_game_exe(const char *exe);
void build_command(const char *exe, char *cmd);
int options_menu(void);

#endif /* OPTIONS_H */
