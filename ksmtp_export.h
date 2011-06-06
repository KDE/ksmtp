/*
  Copyright 2010 BetterInbox <contact@betterinbox.com>
      Author: Gregory Schlomoff <gregory.schlomoff@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef KSMTP_EXPORT_H
#define KSMTP_EXPORT_H

#include <kdemacros.h>

#ifndef KSMTP_EXPORT
# if defined(KDEPIM_STATIC_LIBS)
   /* No export/import for static libraries */
#  define KSMTP_EXPORT
# elif defined(MAKE_KSMTP_LIB)
   /* We are building this library */
#  define KSMTP_EXPORT KDE_EXPORT
# else
   /* We are using this library */
#  define KSMTP_EXPORT KDE_IMPORT
# endif
#endif

#endif