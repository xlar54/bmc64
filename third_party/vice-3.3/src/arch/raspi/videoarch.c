/*
 * videoarch.c
 *
 * Written by
 *  Randy Rossi <randy.rossi@gmail.com>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "videoarch.h"

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#include "monitor.h"
#include "mem.h"
#include "kbd.h"
#include "keyboard.h"
#include "video.h"
#include "viewport.h"
#include "ui.h"
#include "joy.h"
#include "resources.h"
#include "joyport/joystick.h"
#include "demo.h"
#include "overlay.h"
#include "menu.h"
#include "font.h"
#include "sid.h"
#include "machine.h"
#include "raspi_machine.h"
#include "kbdbuf.h"
#include "menu_tape_osd.h"

// Keep video state shared between compilation units here
struct VideoData video_state;

static unsigned int default_color_palette[] = {
0x00,0x00,0x00,
0xFD,0xFE,0xFC,
0xBE,0x1A,0x24,
0x30,0xE6,0xC6,
0xB4,0x1A,0xE2,
0x1F,0xD2,0x1E,
0x21,0x1B,0xAE,
0xDF,0xF6,0x0A,
0xB8,0x41,0x04,
0x6A,0x33,0x04,
0xFE,0x4A,0x57,
0x42,0x45,0x40,
0x70,0x74,0x6F,
0x59,0xFE,0x59,
0x5F,0x53,0xFE,
0xA4,0xA7,0xA2,
};

static unsigned int vice_color_palette[] = {
0x00,0x00,0x00,
0xFF,0xFF,0xFF,
0x68,0x37,0x2b,
0x70,0xa4,0xb2,
0x6f,0x3d,0x86,
0x58,0x8d,0x43,
0x35,0x28,0x79,
0xb8,0xc7,0x6f,
0x6f,0x4f,0x25,
0x43,0x39,0x00,
0x9a,0x67,0x59,
0x44,0x44,0x44,
0x6c,0x6c,0x6c,
0x9a,0xd2,0x84,
0x6c,0x5e,0xb5,
0x95,0x95,0x95,
};


static unsigned int c64hq_color_palette[] = {
0x0A,0x0A,0x0A,
0xFF,0xF8,0xFF,
0x85,0x1F,0x02,
0x65,0xCD,0xA8,
0xA7,0x3B,0x9F,
0x4D,0xAB,0x19,
0x1A,0x0C,0x92,
0xEB,0xE3,0x53,
0xA9,0x4B,0x02,
0x44,0x1E,0x00,
0xD2,0x80,0x74,
0x46,0x46,0x46,
0x8B,0x8B,0x8B,
0x8E,0xF6,0x8E,
0x4D,0x91,0xD1,
0xBA,0xBA,0xBA,
};


static unsigned int pepto_ntsc_color_palette[] = {
0x00,0x00,0x00,
0xFF,0xFF,0xFF,
0x67,0x37,0x2B,
0x70,0xA3,0xB1,
0x6F,0x3D,0x86,
0x58,0x8C,0x42,
0x34,0x28,0x79,
0xB7,0xC6,0x6E,
0x6F,0x4E,0x25,
0x42,0x38,0x00,
0x99,0x66,0x59,
0x43,0x43,0x43,
0x6B,0x6B,0x6B,
0x9A,0xD1,0x83,
0x6B,0x5E,0xB5,
0x95,0x95,0x95,
};

static unsigned int pepto_pal_color_palette[] = {
0x00,0x00,0x00,
0xFF,0xFF,0xFF,
0x68,0x37,0x2b,
0x70,0xa4,0xb2,
0x6f,0x3d,0x86,
0x58,0x8d,0x43,
0x35,0x28,0x79,
0xb8,0xc7,0x6f,
0x6f,0x4f,0x25,
0x43,0x39,0x00,
0x9a,0x67,0x59,
0x44,0x44,0x44,
0x6c,0x6c,0x6c,
0x9a,0xd2,0x84,
0x6c,0x5e,0xb5,
0x95,0x95,0x95,
};

// We tell vice our clock resolution is the actual vertical
// refresh rate of the machine * some factor. We report our
// tick count when asked for the current time which is incremented
// by that factor after each vertical blank.  Not sure if this
// is really needed anymore since we turned off auto refresh.
// Seems to work fine though.
unsigned long video_ticks = 0;

// So...due to math stuff, whatever value we put for our video tick
// increment here will be how many frames we show before vice decides
// to skip.  Can't really figure out why.  Needs more investigation.
const unsigned long video_tick_inc = 10000;
unsigned long video_freq;
unsigned long video_frame_count;

int raspi_warp = 0;
static int raspi_boot_warp = 1;
static int fix_sid = 0;

extern struct joydev_config joydevs[2];

int pending_emu_key_head = 0;
int pending_emu_key_tail = 0;
long pending_emu_key[16];
int pending_emu_key_pressed[16];

int pending_emu_joy_head = 0;
int pending_emu_joy_tail = 0;
int pending_emu_joy_value[128];
int pending_emu_joy_port[128];
int pending_emu_joy_type[128];

extern int pending_emu_quick_func;

#define COLOR16(red, green, blue)         (((red) & 0x1F) << 11 \
                                        | ((green) & 0x1F) << 6 \
                                        | ((blue) & 0x1F))

// Draw the src buffer into the dst buffer.
void draw(uint8_t *src, int srcw, int srch, int src_pitch,
           uint8_t *dst, int dst_pitch, int dst_off_x, int dst_off_y) {
    // x,y are coordinates in src canvas space
    int x,y;
    for(y=0; y < srch; y++){
       int gy = (y+dst_off_y);
       int p1 = dst_off_x+gy*dst_pitch;
       int yp = y*src_pitch;
       memcpy(dst+p1, src+yp, srcw);
    }
}

static unsigned int* get_palette(int index) {
  switch (index) {
     case PALETTE_DEFAULT:
	return default_color_palette;
	break;
     case PALETTE_VICE:
	return vice_color_palette;
	break;
     case PALETTE_C64HQ:
	return c64hq_color_palette;
	break;
     case PALETTE_PEPTO_NTSC:
	return pepto_ntsc_color_palette;
	break;
     case PALETTE_PEPTO_PAL:
	return pepto_pal_color_palette;
	break;
     default:
        return NULL;
  }
}

// Called by menu when palette changes
void video_canvas_change_palette(int index) {
  if (!video_state.canvas) return;

  video_state.palette_index = index;
  // This will call set_palette below to get called after color controls
  // have been applied to the palette.
  video_color_update_palette(video_state.canvas);
}

// Called when a color setting has changed
void video_color_setting_changed() {
  if (!video_state.canvas) return;

  // This will call set_palette below to get called after color controls
  // have been applied to the palette.
  video_color_update_palette(video_state.canvas);
}

int video_canvas_set_palette(struct video_canvas_s *canvas, palette_t *p) {
  canvas->palette = p;

  for (int i=0; i<16;i++) {
    circle_set_palette(i, COLOR16(p->entries[i].red >> 4,
                                  p->entries[i].green >> 4,
                                  p->entries[i].blue >> 4));
  }
  circle_update_palette();
}

struct video_canvas_s *video_canvas_create(struct video_canvas_s *canvas, unsigned int *width, unsigned int *height, int mapped) {
  *width  = circle_get_display_w();
  *height = circle_get_display_h();
  canvas->draw_buffer->canvas_physical_width = *width;
  canvas->draw_buffer->canvas_physical_height = *height;
  canvas->videoconfig->external_palette = 1;
  canvas->videoconfig->external_palette_name = "RASPI";
  video_state.canvas = canvas;
  video_state.vis_h = canvas->draw_buffer->visible_height;
  video_state.vis_w = canvas->draw_buffer->visible_width;
printf ("vis wh %d %d\n",video_state.vis_w,video_state.vis_h);

  int adj_x = 0;
  if (machine_class == VICE_MACHINE_VIC20) {
  // TODO: I don't understand why this is necessary.
    adj_x = (video_state.vis_w - canvas->geometry->gfx_size.width ) / 2;
  }

  // Figure out what it takes to center our canvas on the display
  video_state.dst_off_x = (video_state.fb_w - video_state.vis_w) / 2;
  video_state.dst_off_y = (video_state.fb_h - video_state.vis_h) / 2;
  if (video_state.dst_off_x < 0) {
     // Truncate the source so we don't clobber our frame buffer limits.
     video_state.dst_off_x = 0;
     video_state.vis_w = video_state.fb_w;
  }
  if (video_state.dst_off_y < 0) {
     // Truncate the source so we don't clobber our frame buffer limits.
     video_state.dst_off_y = 0;
     video_state.vis_h = video_state.fb_h;
  }

  video_state.top_left = canvas->geometry->first_displayed_line *
      canvas->draw_buffer->draw_buffer_width +
          canvas->geometry->extra_offscreen_border_left + adj_x;

  overlay_init(video_state.vis_w, 10);
  return canvas;
}

void video_arch_canvas_init(struct video_canvas_s *canvas){
  int i;

  canvas->video_draw_buffer_callback = NULL;

  uint8_t *fb = circle_get_fb();
  int fb_pitch = circle_get_fb_pitch();
  int h = circle_get_display_h();
  bzero(fb, h*fb_pitch);

  int timing = circle_get_machine_timing();
  set_refresh_rate(timing, canvas);

  video_state.first_refresh = 1;
  video_state.dst_off_x = -1;
  video_state.dst_off_y = -1;

  video_freq = canvas->refreshrate * video_tick_inc;

  int fb_w = circle_get_display_w();
  int fb_h = circle_get_display_h();

  video_state.canvas = canvas;
  video_state.fb_w = fb_w;
  video_state.fb_h = fb_h;
  video_state.dst_pitch = fb_pitch;
  video_state.dst = fb;
  set_video_font(&video_state);
  video_state.palette_index = PALETTE_DEFAULT;
  video_state.offscreen_buffer_y = 0;
  video_state.onscreen_buffer_y = circle_get_display_h();
}

void video_canvas_refresh(struct video_canvas_s *canvas,
                unsigned int xs, unsigned int ys, unsigned int xi,
                unsigned int yi, unsigned int w, unsigned int h) {
  video_state.src = canvas->draw_buffer->draw_buffer;
  video_state.src_pitch = canvas->draw_buffer->draw_buffer_width;

  // TODO: Try to get rid of this
  if (video_state.first_refresh == 1) {
     resources_set_int("WarpMode", 1);
     raspi_boot_warp = 1;
     video_state.first_refresh = 0;
  }
}

unsigned long vsyncarch_frequency(void) {
    return video_freq;
}

unsigned long vsyncarch_gettime(void) {
    return video_ticks;
}

void vsyncarch_init(void) {
   // See video refresh code to see why this is necessary.
   int sid_engine;
   resources_get_int("SidEngine", &sid_engine);
   if (sid_engine == SID_ENGINE_RESID) {
      fix_sid = 1;
   }
}

void vsyncarch_presync(void){
   kbdbuf_flush();
}

void videoarch_swap() {
  // Show the region we just drew.
  circle_set_fb_y(video_state.offscreen_buffer_y);
  // Swap buffer ptr for next frame.
  video_state.onscreen_buffer_y = video_state.offscreen_buffer_y;
  video_state.offscreen_buffer_y = circle_get_display_h() -
                                      video_state.offscreen_buffer_y;
}

void vsyncarch_postsync(void){
  // Sync with display's vertical blank signal.

  draw(video_state.src+video_state.top_left,
       video_state.vis_w,
       video_state.vis_h,
       video_state.src_pitch,
       video_state.dst + video_state.offscreen_buffer_y*video_state.dst_pitch,
       video_state.dst_pitch,
       video_state.dst_off_x, video_state.dst_off_y);

  videoarch_swap();

  // Always draw overlay on visible buffer
  if (overlay_forced() || (overlay_enabled() && overlay_showing)) {
     overlay_check();
     draw(overlay_buf, video_state.vis_w, 10, video_state.vis_w,
       video_state.dst + video_state.onscreen_buffer_y*video_state.dst_pitch,
       video_state.dst_pitch, video_state.dst_off_x, video_state.dst_off_y + video_state.vis_h - 10);
  }

  // This render will handle any OSDs we have. ODSs don't pause emulation.
  if (ui_activated) {
     ui_render_now();
     ui_check_key();
  }

  video_ticks+=video_tick_inc;

  // This yield is important to let the fake kernel 'threads' run.
  circle_yield();

  video_frame_count++;
  if (raspi_boot_warp && video_frame_count > 120) {
     raspi_boot_warp = 0;
     circle_boot_complete();
     resources_set_int("WarpMode", 0);
  }

  // BEGIN UGLY HACK
  // What follows is an ugly hack to get around a small extra delay
  // in the audio buffer when RESID is set and we first boot.  I
  // fought with VICE for a while but eventually just decided to re-init
  // RESID at this point in the boot process to work around the issue.  This
  // gets our audio buffer as close to the 'live' edge as possible.  It's only
  // an issue if RESID is the engine selected for boot.
  if (fix_sid) {
     if (video_frame_count == 121) {
        resources_set_int("SidEngine", SID_ENGINE_FASTSID);
     } else if (video_frame_count == 122) {
        resources_set_int("SidEngine", SID_ENGINE_RESID);
        fix_sid = 0;
     }
  }
  // END UGLY HACK

  // Hold the frame until vsync unless warping
  if (!raspi_boot_warp && !raspi_warp) {
     circle_wait_vsync();
  }

  circle_check_gpio();

  if (joydevs[0].device == JOYDEV_GPIO_0 || joydevs[1].device == JOYDEV_GPIO_0) {
     circle_poll_joysticks(0, 0);
  }
  if (joydevs[0].device == JOYDEV_GPIO_1 || joydevs[1].device == JOYDEV_GPIO_1) {
     circle_poll_joysticks(1, 0);
  }

  int reset_demo = 0;

  // Do key press/releases and joy latches on the main loop.
  circle_lock_acquire();
  while (pending_emu_key_head != pending_emu_key_tail) {
     int i = pending_emu_key_head & 0xf;
     reset_demo = 1;
     if (pending_emu_key_pressed[i]) {
        keyboard_key_pressed(pending_emu_key[i]);
     } else {
        keyboard_key_released(pending_emu_key[i]);
     }
     pending_emu_key_head++;
  }

  while (pending_emu_joy_head != pending_emu_joy_tail) {
     int i = pending_emu_joy_head & 0x7f;
     reset_demo = 1;
     switch (pending_emu_joy_type[i]) {
        case PENDING_EMU_JOY_TYPE_ABSOLUTE:
           joystick_set_value_absolute(pending_emu_joy_port[i],
              pending_emu_joy_value[i] & 0x1f);
           joystick_set_potx((pending_emu_joy_value[i] & POTX_BIT_MASK) >> 5);
           joystick_set_poty((pending_emu_joy_value[i] & POTY_BIT_MASK) >> 13);
           break;
        case PENDING_EMU_JOY_TYPE_AND:
           joystick_set_value_and(pending_emu_joy_port[i],
              pending_emu_joy_value[i] & 0x1f);
           joystick_set_potx_and((pending_emu_joy_value[i] & POTX_BIT_MASK) >> 5);
           joystick_set_poty_and((pending_emu_joy_value[i] & POTY_BIT_MASK) >> 13);
           break;
        case PENDING_EMU_JOY_TYPE_OR:
           joystick_set_value_or(pending_emu_joy_port[i],
              pending_emu_joy_value[i] & 0x1f);
           joystick_set_potx_or((pending_emu_joy_value[i] & POTX_BIT_MASK) >> 5);
           joystick_set_poty_or((pending_emu_joy_value[i] & POTY_BIT_MASK) >> 13);
           break;
        default:
           break;
     }
     pending_emu_joy_head++;
  }

  ui_handle_toggle_or_quick_func();

  circle_lock_release();

  if (reset_demo) {
     demo_reset_timeout();
  }

  if (raspi_demo_mode) {
     demo_check();
  }
}

void vsyncarch_sleep(unsigned long delay) {
  // We don't sleep here. Instead, our pace is governed by the
  // wait for vertical blank in vsyncarch_postsync above. This
  // times our machine properly.
}

// queue a key for press/release for the main loop
void circle_emu_key_interrupt(long key, int pressed) {
  circle_lock_acquire();
  int i = pending_emu_key_tail & 0xf;
  pending_emu_key[i] = key;
  pending_emu_key_pressed[i] = pressed;
  pending_emu_key_tail++;
  circle_lock_release();
}

// queue a joy latch change for the main loop
void circle_emu_joy_interrupt(int type, int port, int value) {
  circle_lock_acquire();
  int i = pending_emu_joy_tail & 0x7f;
  pending_emu_joy_type[i] = type;
  pending_emu_joy_port[i] = port;
  pending_emu_joy_value[i] = value;
  pending_emu_joy_tail++;
  circle_lock_release();
}

void circle_emu_quick_func_interrupt(int button_assignment) {
  pending_emu_quick_func = button_assignment;
}

// Called by our special hook in vice to load palettes from
// memory.
palette_t* raspi_video_load_palette(int num_entries, char* name) {
  palette_t* palette = palette_create(16, NULL);
  unsigned int* pal = get_palette(video_state.palette_index);
  for (int i=0; i<num_entries;i++) {
    palette->entries[i].red = pal[i*3];
    palette->entries[i].green = pal[i*3+1];
    palette->entries[i].blue = pal[i*3+2];
    palette->entries[i].dither = 0;
  }
  return palette;
}

// This should be self contained. Don't rely on anything other
// than the frame buffer which is guaranteed to be available.
void main_exit(void) {
  // We should never get here.  If we do, it's probably
  // becasue essential roms are missing.  So display a message
  // to that effect.

  int i;
  uint8_t *fb = circle_get_fb();
  int fb_pitch = circle_get_fb_pitch();
  int h = circle_get_display_h();
  bzero(fb, h*fb_pitch);

  video_state.font = (uint8_t *)&font8x8_basic;
  for (i = 0; i < 256; ++i) {
     video_state.font_translate[i] = (8 * (i & 0x7f));
  }

  int x = 0;
  int y = 3;
  ui_draw_text_buf("Emulator failed to start.",
     x, y, 1, fb, fb_pitch);
  y+=8;
  ui_draw_text_buf("This most likely means you are missing",
     x, y, 1, fb, fb_pitch);
  y+=8;
  ui_draw_text_buf("ROM files. Or you have specified an",
     x, y, 1, fb, fb_pitch);
  y+=8;
  ui_draw_text_buf("invalid kernal, chargen or basic",
     x, y, 1, fb, fb_pitch);
  y+=8;
  ui_draw_text_buf("ROM in vice.ini.  See documentation.",
     x, y, 1, fb, fb_pitch);
  y+=8;

  circle_set_palette(0, COLOR16(0,0,0));
  circle_set_palette(1, COLOR16(255>>4,255>>4,255>>4));
  circle_update_palette();
}

