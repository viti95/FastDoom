/*
 * TEXTS.H - FastDoom text mode launcher, common texts
 *
 * The strings shared by the launcher screens. Keep the menu titles
 * here so they stay consistent across the code.
 */

#ifndef TEXTS_H
#define TEXTS_H

/* The program name, used in every menu title. */
#define TEXT_LAUNCHER "FastDoom launcher"

/* Arrow glyphs for the on screen help lines. In the IBM PC text
   mode (code page 437) the ASCII control codes 0x18-0x1B are
   displayed as arrows. */
#define ARROW_UP    "\x18"
#define ARROW_DOWN  "\x19"
#define ARROW_RIGHT "\x1A"
#define ARROW_LEFT  "\x1B"

/* Menu titles. */
#define TEXT_TITLE_MAIN    TEXT_LAUNCHER
#define TEXT_TITLE_OPTIONS TEXT_LAUNCHER " (Options)"

/* Single level launcher menu titles. */
#define TEXT_TITLE_SINGLE_IWAD   TEXT_LAUNCHER " (Single level, IWAD)"
#define TEXT_TITLE_SINGLE_PWAD   TEXT_LAUNCHER " (Single level, PWAD)"
#define TEXT_TITLE_SINGLE_LEVEL  TEXT_LAUNCHER " (Single level, level)"
#define TEXT_TITLE_SINGLE_SKILL  TEXT_LAUNCHER " (Single level, skill)"

#endif /* TEXTS_H */
