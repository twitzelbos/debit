/*
 * (C) Copyright 2006 Jean-Baptiste Note <jean-baptiste.note@m4x.org>
 * All rights reserved.
 */

#ifndef _HAS_BITDRAW_H
#define _HAS_BITDRAW_H

#include <cairo.h>
#include <glib.h>
#include "sites.h"
#include "wiring.h"
#include "localpips.h"
#include "bitstream_parser.h"
#include "analysis.h"

#define SWITCH_CENTER_X 50.0
#define SWITCH_CENTER_Y 50.0
#define SWITCH_RADIUS   15.0

typedef struct _drawing_context {
  cairo_t *cr;
  gboolean text;
  /* cairo information */
  cairo_pattern_t *site_sing_patterns[NR_SITE_TYPE];
  cairo_pattern_t *site_line_patterns[NR_SITE_TYPE];
  cairo_pattern_t *site_full_patterns[NR_SITE_TYPE];

  /* And I also want lazy patterns for wires */
  cairo_pattern_t **pip_patterns;

  /* Drawing parameters */
  gint x_offset;
  gint y_offset;
  double zoom;

  /* structural information */
} drawing_context_t;

/* All sites have the same width and height. Simpler. */
#define SITE_WIDTH 100.0
#define SITE_HEIGHT 100.0

drawing_context_t *
create_drawing_context(const cairo_t *cr);

cairo_t *
destroy_drawing_context(drawing_context_t *ctx);

//void draw_chip(drawing_context_t *ctx, chip_descr_t *chip);
void draw_chip_monitored(drawing_context_t *ctx, const chip_descr_t *chip);
/* draw the chip layout */
//void draw_surface_chip(cairo_surface_t *sr, const chip_descr_t *chip);
void draw_cairo_chip(cairo_t *cr, const chip_descr_t *chip);

/* wire drawing primitives -- this is where the real work happens */
void draw_all_wires(drawing_context_t *ctx, const bitstream_analyzed_t *nlz);
void draw_cairo_wires(cairo_t *sr, const bitstream_analyzed_t *nlz);

/* bad, this. The drawing context should be separated from the cairo_t */
drawing_context_t *drawing_context_create();
void drawing_context_destroy(drawing_context_t *ctx);

static inline
void set_cairo_context(drawing_context_t *ctx, cairo_t *cr) {
  ctx->cr = cr;
}

#endif /* _HAS_BITDRAW_H */