/* This file is part of Te Tuhi Video Game System.
 *
 * Copyright (C) 2008 Douglas Bagnall
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef GOT_DBDEBUG_H
#define GOT_DBDEBUG_H 1



#define debug(format, ...) fprintf (stderr, (format),## __VA_ARGS__); fflush(stderr)
#define flush_debug() fflush(stderr)

#define _message debug

#define debug_lineno() debug("%-25s  line %4d \n", __func__, __LINE__ )

#ifdef VERBOSE
#if VERBOSE
#define vdebug(format, ...) fprintf (stderr, (format),## __VA_ARGS__)
#else
#define vdebug(format, ...) 
#endif
#else
#define vdebug(format, ...) 
#endif

#endif
