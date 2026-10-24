/*
 * r_drawh.h
 *
 * Horizontal column rendering ("horiz" method), after r_drawt.c from
 * the ZDoom 1.22 source.
 *
 * Columns are expanded into a small temporary buffer holding four
 * interleaved columns, then flushed to the screen in wide (word /
 * dword) chunks with span alignment between the four columns.
 *
 * C89 reference implementation. The ASM version can replace the
 * individual functions (R_DrawColumnHoriz, rt_Draw1col / rt_Draw2cols
 * / rt_Draw4cols) later.
 */

#ifndef __R_DRAWH__
#define __R_DRAWH__

/*
 * Expands the current column (dc_yl..dc_yh at dc_x, using dc_source /
 * dc_texturemid / dc_iscale / dc_colormap) into the temporary buffer.
 * Same interface as R_DrawColumn().
 */
extern void R_DrawColumnHoriz(void);

/* Fills the current column with dc_color into the temporary buffer. */
extern void R_FillColumnHoriz(void);

/* Resets the span read pointers; call before starting a new group
 * of columns. */
extern void rt_InitCols(void);

/* Flushes the pending spans of one / two / four temp columns to the
 * screen. hx is the temp column (0..3), sx the screen x. sx must be
 * word-aligned for rt_Draw2cols() and dword-aligned for
 * rt_Draw4cols(). */
extern void rt_Draw1col(int hx, int sx);
extern void rt_Draw2cols(int hx, int sx);
extern void rt_Draw4cols(int sx);

/* Flushes a complete group of four columns starting at x0 (x0 must
 * be a multiple of 4). Call it inside the per-column rendering loop
 * as soon as the fourth column of the group has been expanded. */
extern void R_HorizGroupFlush(int x0);

/* Flushes the pending tail of the current group (columns after the
 * last complete group, up to xend). Call at the end of a rendering
 * range (segment loop, masked seg range). */
extern void R_HorizSegEnd(int xend);

/* Per-frame initialization (row pointers, span rings). Call once per
 * frame before rendering. */
extern void R_HorizInitFrame(void);

/* One-time initialization. */
extern void R_HorizInit(void);

#endif /* __R_DRAWH__ */
