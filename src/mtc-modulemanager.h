/* mtc-modulemanager.h
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

#ifndef __MAILTC_MODULE_MANAGER_H__
#define __MAILTC_MODULE_MANAGER_H__

#include "mtc-module.h"
#include "mtc-extension.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_MODULE_MANAGER      (mailtc_module_manager_get_type ())

G_DECLARE_FINAL_TYPE                    (MailtcModuleManager, mailtc_module_manager, MAILTC, MODULE_MANAGER, GObject)

gboolean
mailtc_module_manager_unload            (MailtcModuleManager* manager,
                                         GError**             error);

gboolean
mailtc_module_manager_load              (MailtcModuleManager* manager,
                                         GError**             error);

MailtcExtension*
mailtc_module_manager_find_extension    (MailtcModuleManager* manager,
                                         const gchar*         module_name,
                                         const gchar*         extension_name);

MailtcModule*
mailtc_module_manager_find_module       (MailtcModuleManager* manager,
                                         const gchar*         module_name,
                                         const gchar*         extension_name);

void
mailtc_module_manager_foreach_extension (MailtcModuleManager* manager,
                                         GFunc                func,
                                         gpointer             user_data);

MailtcModuleManager*
mailtc_module_manager_new               (gchar*   directory,
                                         GError** error);

G_END_DECLS

#endif /* __MAILTC_MODULE_MANAGER_H__ */

