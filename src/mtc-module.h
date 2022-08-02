/* mtc-module.h
 * Copyright (C) 2009-2022 Dale Whittaker
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

#ifndef __MAILTC_MODULE_H__
#define __MAILTC_MODULE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_MODULE (mailtc_module_get_type ())

G_DECLARE_FINAL_TYPE       (MailtcModule, mailtc_module, MAILTC, MODULE, GObject)

MailtcModule*
mailtc_module_new          (void);

gboolean
mailtc_module_unload       (MailtcModule* module,
                            GError**      error);

gboolean
mailtc_module_load         (MailtcModule* module,
                            gchar*        filename,
                            GError**      error);

gboolean
mailtc_module_symbol       (MailtcModule* module,
                            const gchar*  symbol_name,
                            gpointer*     symbol,
                            GError**      error);

const gchar*
mailtc_module_get_name     (MailtcModule* module);

gboolean
mailtc_module_supported    (GError** error);

gboolean
mailtc_module_filename     (const gchar* filename);

G_END_DECLS

#endif /* __MAILTC_MODULE_H__ */

