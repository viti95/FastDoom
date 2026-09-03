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

/* Menu titles. */
#define TEXT_TITLE_MAIN    TEXT_LAUNCHER
#define TEXT_TITLE_OPTIONS TEXT_LAUNCHER " (Options)"

/* Single level launcher menu titles. */
#define TEXT_TITLE_SINGLE_IWAD   TEXT_LAUNCHER " (Single level, IWAD)"
#define TEXT_TITLE_SINGLE_PWAD   TEXT_LAUNCHER " (Single level, PWAD)"
#define TEXT_TITLE_SINGLE_LEVEL  TEXT_LAUNCHER " (Single level, level)"
#define TEXT_TITLE_SINGLE_SKILL  TEXT_LAUNCHER " (Single level, skill)"

#endif /* TEXTS_H */
