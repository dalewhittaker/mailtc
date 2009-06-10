/* mtc-file.h
 * Copyright (C) 2009 Dale Whittaker <dayul@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __MAILTC_FILE_H__
#define __MAILTC_FILE_H__

#include "mtc.h"

#if HAVE_OPENSSL
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#endif

G_BEGIN_DECLS

#ifdef MAXPATHLEN
#define MAILTC_PATH_LENGTH MAXPATHLEN
#elif defined (PATH_MAX)
#define MAILTC_PATH_LENGTH PATH_MAX
#else
#define MAILTC_PATH_LENGTH 2048
#endif

gboolean
mailtc_save_config (mtc_config*  config,
                    GError**     error);

gboolean
mailtc_load_config (mtc_config*  config,
                    GError**     error);

gchar*
mailtc_file        (mtc_config*  config,
                    const gchar* filename);

G_END_DECLS

#endif /* __MAILTC_FILE_H__ */

