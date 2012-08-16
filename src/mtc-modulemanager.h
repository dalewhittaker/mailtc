/* mtc-modulemanager.h
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

#ifndef __MAILTC_MODULE_MANAGER_H__
#define __MAILTC_MODULE_MANAGER_H__

#include "mtc-module.h"
#include "mtc-extension.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_MODULE_MANAGER            (mailtc_module_manager_get_type ())
#define MAILTC_MODULE_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAILTC_TYPE_MODULE_MANAGER, MailtcModuleManager))
#define MAILTC_MODULE_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  MAILTC_TYPE_MODULE_MANAGER, MailtcModuleManagerClass))
#define MAILTC_IS_MODULE_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAILTC_TYPE_MODULE_MANAGER))
#define MAILTC_IS_MODULE_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  MAILTC_TYPE_MODULE_MANAGER))
#define MAILTC_MODULE_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  MAILTC_TYPE_MODULE_MANAGER, MailtcModuleManagerClass))

typedef struct _MailtcModuleManager        MailtcModuleManager;
typedef struct _MailtcModuleManagerClass   MailtcModuleManagerClass;
typedef struct _MailtcModuleManagerPrivate MailtcModuleManagerPrivate;

gboolean
mailtc_module_manager_unload            (MailtcModuleManager* manager,
                                         GError**             error);

gboolean
mailtc_module_manager_load              (MailtcModuleManager* manager,
                                         GError**             error);

MailtcExtension*
mailtc_module_manager_find_extension    (MailtcModuleManager* manager,
                                         const gchar*         module_name);

void
mailtc_module_manager_foreach_extension (MailtcModuleManager* manager,
                                         GFunc                func,
                                         gpointer             user_data);

GType
mailtc_module_manager_get_type          (void);

MailtcModuleManager*
mailtc_module_manager_new               (gchar*   directory,
                                         GError** error);

G_END_DECLS

#endif /* __MAILTC_MODULE_MANAGER_H__ */

