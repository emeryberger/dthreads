#ifndef _DEBUG_H_
#define _DEBUG_H_

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*
 * @file   debug.h
 * @brief  debug macro.
 * @author Charlie Curtsinger <http://www.cs.umass.edu/~charlie>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <stdio.h>
#include <assert.h>

#ifdef DEBUG
    #define DEBUG(...) fprintf(stderr, "%20s:%-4d: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
#else
    #define DEBUG(_fmt, ...)
#endif

#ifdef CHECK_SCHEDULE
  #define PRINT_SCHEDULE(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
#else
  #define PRINT_SCHEDULE(_fmt, ...)
#endif

#endif
