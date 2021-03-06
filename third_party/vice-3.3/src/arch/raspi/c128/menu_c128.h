/*
 * menu_c128.h
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

unsigned long calculate_timing(double fps);

void set_color_brightness(int value);
void set_color_contrast(int value);
void set_color_gamma(int value);
void set_color_tint(int value);
void set_video_cache(int value);
void set_hw_scale(int value);

int get_color_brightness(void);
int get_color_contrast(void);
int get_color_gamma(void);
int get_color_tint(void);
