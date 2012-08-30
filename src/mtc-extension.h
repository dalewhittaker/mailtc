/* mtc-extension.h
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

#ifndef __MAILTC_EXTENSION_H__
#define __MAILTC_EXTENSION_H__

#include "mailtc.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_EXTENSION                   (mailtc_extension_get_type ())
#define MAILTC_EXTENSION(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAILTC_TYPE_EXTENSION, MailtcExtension))
#define MAILTC_EXTENSION_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass),  MAILTC_TYPE_EXTENSION, MailtcExtensionClass))
#define MAILTC_IS_EXTENSION(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAILTC_TYPE_EXTENSION))
#define MAILTC_IS_EXTENSION_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass),  MAILTC_TYPE_EXTENSION))
#define MAILTC_EXTENSION_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj),  MAILTC_TYPE_EXTENSION, MailtcExtensionClass))

typedef GSList*
(*MailtcExtensionInitFunc)               (const gchar* directory);

gboolean
mailtc_extension_is_valid                (MailtcExtension* extension,
                                          GError**         error);

const gchar*
mailtc_extension_get_compatibility       (MailtcExtension* extension);

const gchar*
mailtc_extension_get_name                (MailtcExtension* extension);

const gchar*
mailtc_extension_get_author              (MailtcExtension* extension);

const gchar*
mailtc_extension_get_description         (MailtcExtension* extension);

void
mailtc_extension_set_directory           (MailtcExtension* extension,
                                          const gchar*     directory);

void
mailtc_extension_set_module              (MailtcExtension* extension,
                                          GObject*         module);

GObject*
mailtc_extension_get_module              (MailtcExtension* extension);

GArray*
mailtc_extension_get_protocols           (MailtcExtension* extension);

gboolean
mailtc_extension_add_account             (MailtcExtension* extension,
                                          GObject*         account,
                                          GError**         error);

gboolean
mailtc_extension_remove_account          (MailtcExtension* extension,
                                          GObject*         account,
                                          GError**         error);

gboolean
mailtc_extension_read_messages           (MailtcExtension* extension,
                                          GObject*         account,
                                          GError**         error);

gint64
mailtc_extension_get_messages            (MailtcExtension* extension,
                                          GObject*         account,
                                          gboolean         debug,
                                          GError**         error);

GType
mailtc_extension_get_type                (void);

MailtcExtension*
mailtc_extension_new                     (void);

G_END_DECLS

#endif /* __MAILTC_EXTENSION_H__ */

