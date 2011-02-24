/* mtc-plugin.h
 * Copyright (C) 2009-2011 Dale Whittaker <dayul@users.sf.net>
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

#ifndef __MAILTC_PLUGIN_H__
#define __MAILTC_PLUGIN_H__

#include "mtc.h"
#include <gmodule.h>

G_BEGIN_DECLS

gboolean
mailtc_load_plugins   (mtc_config* config,
                       GError**    error);

gboolean
mailtc_unload_plugins (mtc_config* config,
                       GError**    error);

G_END_DECLS

#endif /* __MAILTC_PLUGIN_H__ */

