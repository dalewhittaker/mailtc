/* mtc-module.h
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

#ifndef __MAILTC_MODULE_H__
#define __MAILTC_MODULE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_MODULE            (mailtc_module_get_type  ())
#define MAILTC_MODULE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAILTC_TYPE_MODULE, MailtcModule))
#define MAILTC_MODULE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  MAILTC_TYPE_MODULE, MailtcModuleClass))
#define MAILTC_IS_MODULE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAILTC_TYPE_MODULE))
#define MAILTC_IS_MODULE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  MAILTC_TYPE_MODULE))
#define MAILTC_MODULE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  MAILTC_TYPE_MODULE, MailtcModuleClass))

typedef struct _MailtcModule        MailtcModule;
typedef struct _MailtcModuleClass   MailtcModuleClass;
typedef struct _MailtcModulePrivate MailtcModulePrivate;

GType
mailtc_module_get_type   (void);

MailtcModule*
mailtc_module_new        (void);

gboolean
mailtc_module_unload     (MailtcModule* module,
                          GError**      error);

gboolean
mailtc_module_load       (MailtcModule* module,
                          gchar*        filename,
                          GError**      error);

gboolean
mailtc_module_symbol     (MailtcModule* module,
                          const gchar*  symbol_name,
                          gpointer*     symbol,
                          GError**      error);

const gchar*
mailtc_module_get_name   (MailtcModule* module);

gboolean
mailtc_module_supported  (GError** error);

gboolean
mailtc_module_filename   (const gchar* filename);

G_END_DECLS

#endif /* __MAILTC_MODULE_H__ */

