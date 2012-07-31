/* mtc-util.h
 * Copyright (C) 2009-2012 Dale Whittaker <dayul@users.sf.net>
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

#ifndef __MAILTC_UTIL_H__
#define __MAILTC_UTIL_H__

#include <gdk/gdk.h>

G_BEGIN_DECLS

#define mailtc_message g_message
#define mailtc_warning g_warning
#define mailtc_error g_critical
#define mailtc_fatal g_error

gchar*
mailtc_current_time         (void);

void
mailtc_info                 (const gchar* format,
                             ...);

void
mailtc_gerror               (GError**     error);

void
mailtc_gerror_warn          (GError**     error);

void
mailtc_log                  (GIOChannel*  log,
                             const gchar* message);

gchar*
mailtc_directory            (void);

void
mailtc_run_command          (const gchar* command);

gboolean
mailtc_quit                 (void);

void
mailtc_object_set_string    (GObject*     obj,
                             GType        objtype,
                             const gchar* name,
                             gchar**      value,
                             const gchar* newvalue);

void
mailtc_object_set_uint      (GObject*     obj,
                             GType        objtype,
                             const gchar* name,
                             guint*       value,
                             guint        newvalue);

void
mailtc_object_set_boolean   (GObject*     obj,
                             GType        objtype,
                             const gchar* name,
                             gboolean*    value,
                             gboolean     newvalue);

void
mailtc_object_set_object    (GObject*     obj,
                             GType        objtype,
                             const gchar* name,
                             GObject**    value,
                             GObject*     newvalue);

void
mailtc_object_set_colour    (GObject*        obj,
                             GType           objtype,
                             const gchar*    name,
                             GdkColor*       colour,
                             const GdkColor* newcolour);

void
mailtc_object_set_array     (GObject*     obj,
                             GType        objtype,
                             const gchar* name,
                             GArray**     value,
                             GArray*      newvalue);

void
mailtc_object_set_ptr_array (GObject*     obj,
                             GType        objtype,
                             const gchar* name,
                             GPtrArray**  value,
                             GPtrArray*   newvalue);

void
mailtc_object_set_pointer   (GObject*     obj,
                             GType        objtype,
                             const gchar* name,
                             gpointer*    value,
                             gpointer     newvalue);

G_END_DECLS

#endif /* __MAILTC_UTIL_H__ */

