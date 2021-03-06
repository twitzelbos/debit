/*
 * Copyright (C) 2006, 2007 Jean-Baptiste Note <jean-baptiste.note@m4x.org>
 *
 * This file is part of debit.
 *
 * Debit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Debit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with debit.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Pure-drawing of the sites */
#include <math.h> /* for M_PI */
#include <cairo.h>
#include "debitlog.h"
#include "sites.h"
#include "bitdraw.h"

/* for timing analysis */
#include <glib.h>

#define NAME_OFFSET_X 20.0
#define NAME_OFFSET_Y 20.0
#define NAME_FONT_SIZE 8.0
#define NAME_FONT_TYPE "bitstream vera sans mono"

#define SITE_MARGIN_X 10.0
#define SITE_MARGIN_Y 10.0

#define LUT_WIDTH  10.0
#define LUT_HEIGHT 10.0
#define LUT_BASE_X 80.0
#define LUT_BASE_Y 20.0
#define LUT_DX     00.0
#define LUT_DY     10.0

/* draw clb with no positioning */
static inline void
_draw_box(cairo_t *cr) {
  cairo_rectangle (cr, SITE_MARGIN_X, SITE_MARGIN_Y,
		   SITE_WIDTH - 2 * SITE_MARGIN_X,
		   SITE_HEIGHT - 2 * SITE_MARGIN_Y);
  cairo_stroke (cr);
}

static inline void
_draw_switchbox(cairo_t *cr) {
  /* state is ? */
  cairo_arc (cr, SWITCH_CENTER_X, SWITCH_CENTER_Y, SWITCH_RADIUS, 0, 2 * M_PI);
  cairo_stroke (cr);
}

static inline void
_draw_luts(cairo_t *cr) {
  unsigned i;
  double x = LUT_BASE_X, y = LUT_BASE_Y;
  for (i = 0; i < 4; i++) {
    x += LUT_DX;
    y += LUT_DY;
    cairo_rectangle (cr, x, y, LUT_WIDTH, LUT_HEIGHT);
    cairo_stroke (cr);
  }
}

static inline void
_draw_name(cairo_t *cr, const csite_descr_t *site) {
  gchar name[MAX_SITE_NLEN];
  /* XXX */
  snprint_csite(name, ARRAY_SIZE(name), site, 0, 0);
  //g_print("printing %s", name);
  cairo_move_to(cr, NAME_OFFSET_X, NAME_OFFSET_Y);
  cairo_show_text(cr, name);
  cairo_stroke(cr);
}

static void
_draw_clb_pattern(cairo_t *cr) {
  _draw_box(cr);
  _draw_switchbox(cr);
  _draw_luts(cr);
}

static void
_draw_clb(const drawing_context_t *ctx,
	  const csite_descr_t *site) {
  cairo_t *cr = ctx->cr;

  _draw_clb_pattern(cr);

  /* if text is enabled */
  if (ctx->text)
    _draw_name(cr, site);
}

static cairo_pattern_t *
draw_clb_pattern(drawing_context_t *ctx) {
  cairo_t *cr = ctx->cr;
  const double zoom = ctx->zoom;
  cairo_pattern_t *pat;
  unsigned width = SITE_WIDTH*zoom, height = SITE_HEIGHT*zoom;

  cairo_save (cr);

  cairo_set_source_rgb (cr, 0., 0., 0.);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_clip (cr);
  cairo_scale (cr, zoom, zoom);

  cairo_push_group (cr);

/*   cairo_rectangle (cr, 0, 0, SITE_WIDTH, SITE_HEIGHT); */
/*   cairo_set_source_rgb (cr, 0., 0., 0.); */
/*   cairo_paint (cr); */

  cairo_set_line_width (cr, 1.0);
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);

  _draw_clb_pattern (cr);
  pat = cairo_pop_group (cr);

  cairo_restore (cr);

  return pat;
}

static void
_draw_clb_compose(const drawing_context_t *ctx,
		  const csite_descr_t *site) {
  cairo_t *cr = ctx->cr;
  cairo_pattern_t *site_pattern = ctx->site_sing_patterns[CLB];

  /* now let's draw !
     NB: could use the *whole* thing as pattern */

  /* clip */
  cairo_save (cr);
  /* slower with the clip */
/*   cairo_rectangle (cr, 0, 0, SITE_WIDTH, SITE_HEIGHT); */
/*   cairo_clip (cr); */
  cairo_set_source (cr, site_pattern);
  cairo_paint (cr);

  cairo_restore (cr);

  /* if text is enabled */
  if (ctx->text)
    _draw_name(cr, site);
}

/* Drawing of regular LUT */
typedef void (*site_draw_t)(const drawing_context_t *ctx,
			    const csite_descr_t *site);

static site_draw_t
draw_table_compositing[NR_SITE_TYPE] = {
  [CLB] = _draw_clb_compose,
};

void
_draw_site_compose(const drawing_context_t *ctx,
		   const csite_descr_t *site) {
  site_draw_t fun = draw_table_compositing[site->type];
  if (fun) {
    fun(ctx, site);
  }
}

static void
draw_site_compose(unsigned x, unsigned y,
		  csite_descr_t *site, gpointer data) {
  drawing_context_t *ctx = data;
  cairo_t *cr = ctx->cr;
  double dx = x * SITE_WIDTH, dy = y * SITE_HEIGHT;
  cairo_save (cr);
  cairo_translate(cr, dx, dy);
  _draw_site_compose(ctx, site);
  cairo_restore (cr);
}

static site_draw_t
draw_table_vectorized[NR_SITE_TYPE] = {
  [CLB] = _draw_clb,
};

static void
draw_site_vector(unsigned x, unsigned y,
		 csite_descr_t *site, gpointer data) {
  drawing_context_t *ctx = data;
  site_draw_t fun = draw_table_vectorized[site->type];
  if (fun) {
    cairo_t *cr = ctx->cr;
    double dx = x * SITE_WIDTH, dy = y * SITE_HEIGHT;
    /* move to the right place */
    cairo_translate(cr, dx, dy);
    fun(ctx, site);
    cairo_translate(cr, -dx, -dy);
  }
}

__attribute__((unused)) static cairo_pattern_t *
draw_full_clb_pattern(drawing_context_t *ctx,
		      chip_descr_t *chip) {
  cairo_t *cr = ctx->cr;

  cairo_push_group (cr);
  /* draw the thing only using vector operations */
  iterate_over_sites(chip, draw_site_vector, ctx);
  return cairo_pop_group (cr);
}

void
generate_patterns(drawing_context_t *ctx) {
  /* redraw the thing at the right zoom level */
  ctx->site_sing_patterns[CLB] = draw_clb_pattern(ctx);
}

void
destroy_patterns(drawing_context_t *ctx) {
  cairo_pattern_destroy (ctx->site_sing_patterns[CLB]);
  ctx->site_sing_patterns[CLB] = NULL;
}

static void
draw_chip_for_window(drawing_context_t *ctx, const chip_descr_t *chip) {
  cairo_t *cr = ctx->cr;
  double zoom = ctx->zoom;

  cairo_save (cr);

  cairo_scale (cr, zoom, zoom);

  cairo_translate (cr, -ctx->x_offset, -ctx->y_offset);
  iterate_over_sites(chip, draw_site_compose, ctx);

  cairo_restore (cr);
}

/* \brief Draw a fully vectorized chip layout
 *
 * This version is needed for PDF dump
 *
 */

static void
_draw_chip_vectorized(drawing_context_t *ctx,
		      const chip_descr_t *chip) {
  cairo_t *cr = ctx->cr;

  debit_log(L_DRAW, "vectorized chip draw");
  cairo_rectangle(cr, 0., 0.,
		  chip->width * SITE_WIDTH,
		  chip->height * SITE_HEIGHT);
  cairo_clip (cr);

  /* paint the clip region */
  cairo_set_source_rgb (cr, 0., 0., 0.);
  cairo_paint (cr);

  /* draw everything in white. This could be per-site or per-site-type */
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_set_line_width (cr, 1);

  cairo_select_font_face(cr, NAME_FONT_TYPE,
			 CAIRO_FONT_SLANT_NORMAL,
			 CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, NAME_FONT_SIZE);

  iterate_over_sites(chip, draw_site_vector, ctx);
}

/* Drawing of the whole bunch */
void
draw_chip(drawing_context_t *ctx, const chip_descr_t *chip) {
  cairo_t *cr = ctx->cr;

  debit_log(L_DRAW, "Start of draw chip");

  /* This should be after zoom, in user coordinates */

  /* paint the clip region */
  cairo_set_source_rgb (cr, 0., 0., 0.);
  cairo_paint (cr);

  /* draw everything in white. This could be per-site or per-site-type */
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_set_line_width (cr, 1);

  cairo_select_font_face(cr, NAME_FONT_TYPE,
			 CAIRO_FONT_SLANT_NORMAL,
			 CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, NAME_FONT_SIZE);

  /* initialize patterns -- once and for all ? */
/*   if (!ctx->site_sing_patterns[CLB]) */
/*     ctx->site_sing_patterns[CLB] = draw_clb_pattern(ctx); */

/*   if (!ctx->site_full_patterns[CLB]) */
/*     ctx->site_full_patterns[CLB] = draw_full_clb_pattern(ctx, chip); */
/*   cairo_save (cr); */
/*   cairo_set_source (cr, ctx->site_full_patterns[CLB]); */
/*   cairo_paint (cr); */
/*   cairo_restore (cr); */

  /* move from the context */
  draw_chip_for_window(ctx, chip);

  debit_log(L_DRAW, "End of draw chip");
}

/* create a context from all of a parsed bitstream */
drawing_context_t *
drawing_context_create(void) {
  drawing_context_t *ctx = g_new(drawing_context_t, 1);
  init_drawing_context(ctx);
  return ctx;
}

static inline void
safe_cairo_pattern_destroy(cairo_pattern_t *pat) {
  if (pat)
    cairo_pattern_destroy(pat);
}

void
drawing_context_destroy(drawing_context_t *ctx) {
  unsigned i;
  /* cleanup the patterns */
  for (i = 0; i < NR_SITE_TYPE; i++) {
    safe_cairo_pattern_destroy(ctx->site_sing_patterns[i]);
    safe_cairo_pattern_destroy(ctx->site_line_patterns[i]);
    safe_cairo_pattern_destroy(ctx->site_full_patterns[i]);
  }
  g_free(ctx);
}

/*
   Then callbacks for redrawing etc, as needed for the windowing
   environment. For now pdf-only, so that's it
*/
void
draw_cairo_chip(cairo_t *cr, const chip_descr_t *chip) {
  drawing_context_t ctx;
  init_drawing_context(&ctx);
  set_cairo_context(&ctx, cr);
  _draw_chip_vectorized(&ctx, chip);
}
