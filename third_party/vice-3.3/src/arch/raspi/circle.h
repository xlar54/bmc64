/*
 * circle.h
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

#ifndef VICE_CIRCLE_H
#define VICE_CIRCLE_H

// The way we are organized for the time being is
// for circle to embed vice.  This file defines the
// types circle layer needs to know about to
// give the vice layer what it needs and also the
// functions circle layer need to provide for vice to
// call back into it.  Vice types/includes should not be
// added here.

#include <sys/types.h>

#define MACHINE_TIMING_NTSC_HDMI 0
#define MACHINE_TIMING_PAL_HDMI 1
#define MACHINE_TIMING_NTSC_COMPOSITE 2
#define MACHINE_TIMING_PAL_COMPOSITE 3
#define MACHINE_TIMING_PAL_CUSTOM 4
#define MACHINE_TIMING_NTSC_CUSTOM 5

#define USB_PREF_ANALOG 0
#define USB_PREF_HAT 1

// Make sure does not exceed max choices in ui.h
#define NUM_BUTTON_ASSIGNMENTS 15
#define BTN_ASSIGN_UNDEF 0
#define BTN_ASSIGN_FIRE 1
#define BTN_ASSIGN_MENU 2
#define BTN_ASSIGN_WARP 3
#define BTN_ASSIGN_STATUS_TOGGLE 4
#define BTN_ASSIGN_SWAP_PORTS 5

// Directions and POTs are not available for hotkeys, only buttons
#define BTN_ASSIGN_UP 6
#define BTN_ASSIGN_DOWN 7
#define BTN_ASSIGN_LEFT 8
#define BTN_ASSIGN_RIGHT 9
#define BTN_ASSIGN_POTX 10
#define BTN_ASSIGN_POTY 11

// Back to functions available to anything
#define BTN_ASSIGN_TAPE_MENU 12
#define BTN_ASSIGN_CART_MENU 13
#define BTN_ASSIGN_CART_FREEZE 14

// potx and poty occupy 8 bits in joy int values passed
// to joy update calls
#define POTX_BIT_MASK 0x1fe0
#define POTY_BIT_MASK 0x1fe000

extern int pot_x_high_value;
extern int pot_x_low_value;
extern int pot_y_high_value;
extern int pot_y_low_value;

// TODO: replace this with a direct call from kernel
typedef void (*raspi_key_handler)(long key);

extern int circle_get_machine_timing();
extern uint8_t* circle_get_fb();
extern int circle_get_fb_pitch();
extern void circle_sleep(long);
extern void circle_set_palette(uint8_t, uint16_t);
extern void circle_update_palette();
extern int circle_get_display_w();
extern int circle_get_display_h();
extern unsigned long circle_get_ticks();
extern void circle_set_fb_y(int);
extern void circle_wait_vsync();
extern void circle_yield();
extern void circle_poll_joysticks(int port, int is_interrupt);
extern void circle_check_gpio();

extern void joy_set_gamepad_info(int num_pads, int num_buttons[2], int axes[2], int hats[2]);

extern void circle_joy_usb(unsigned device, int value);
extern void circle_joy_gpio(unsigned device, int value);

extern int circle_joy_need_gpio(int device);
extern void circle_usb_pref(int device, int *usb_pref, int* x_axis, int *y_axis, float *x_thresh, float *y_thresh);
extern int circle_ui_activated(void);
extern void circle_ui_key_interrupt(long key, int pressed);
extern void circle_emu_key_interrupt(long key, int pressed);

extern int menu_wants_raw_usb(void);
extern void menu_raw_usb(int device, unsigned buttons, const int hats[6], const int axes[16]);

extern int circle_button_function(int dev, unsigned button_value);
extern int circle_add_button_values(int dev, unsigned button_value);

extern void circle_lock_acquire();
extern void circle_lock_release();

extern void circle_key_pressed(long key);
extern void circle_key_released(long key);

extern void circle_set_demo_mode(int is_demo);
extern void circle_boot_complete();

void circle_emu_quick_func_interrupt(int button_assignment);

#endif
