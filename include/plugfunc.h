/* plugfunc.h
 * Copyright (C) 2007 Dale Whittaker <dayul@users.sf.net>
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

#ifndef DW_MAILTC_PLUGFUNC
#define DW_MAILTC_PLUGFUNC

#include "common.h"

/*plugfunc.c functions*/
gboolean plg_load_all(void);
gboolean plg_unload_all(void);
mtc_plugin *plg_find(const gchar *plugin_name);
gchar *plg_name(mtc_plugin *pplugin);

#endif /*DW_MAILTC_PLUGFUNC*/
