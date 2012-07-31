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

#include <glib-object.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_EXTENSION                   (mailtc_extension_get_type ())
#define MAILTC_EXTENSION(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAILTC_TYPE_EXTENSION, MailtcExtension))
#define MAILTC_EXTENSION_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass),  MAILTC_TYPE_EXTENSION, MailtcExtensionClass))
#define MAILTC_IS_EXTENSION(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAILTC_TYPE_EXTENSION))
#define MAILTC_IS_EXTENSION_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass),  MAILTC_TYPE_EXTENSION))
#define MAILTC_EXTENSION_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj),  MAILTC_TYPE_EXTENSION, MailtcExtensionClass))

#define MAILTC_DEFINE_TYPE(TN, t_n, t_p)        G_DEFINE_TYPE (TN, t_n, g_type_from_name (#t_p))
#define MAILTC_DEFINE_EXTENSION(TN, t_n)        MAILTC_DEFINE_TYPE (TN, t_n, MailtcExtension)

#define MAILTC_EXTENSION_SYMBOL_INIT            "extension_init"

#define MAILTC_EXTENSION_PROPERTY_COMPATIBILITY "compatibility"
#define MAILTC_EXTENSION_PROPERTY_NAME          "name"
#define MAILTC_EXTENSION_PROPERTY_AUTHOR        "author"
#define MAILTC_EXTENSION_PROPERTY_DESCRIPTION   "description"
#define MAILTC_EXTENSION_PROPERTY_DIRECTORY     "directory"
#define MAILTC_EXTENSION_PROPERTY_MODULE        "module"
#define MAILTC_EXTENSION_PROPERTY_PROTOCOLS     "protocols"

#define MAILTC_EXTENSION_SIGNAL_ADD_ACCOUNT     "add-account"
#define MAILTC_EXTENSION_SIGNAL_REMOVE_ACCOUNT  "remove-account"
#define MAILTC_EXTENSION_SIGNAL_GET_MESSAGES    "get-messages"
#define MAILTC_EXTENSION_SIGNAL_READ_MESSAGES   "read-messages"

#define MAILTC_EXTENSION_GET_PROTOCOL(extension, i) \
    &g_array_index (mailtc_extension_get_protocols ((extension)), MailtcProtocol, (i))

typedef struct _MailtcExtension
{
    GObject parent_instance;
    /* FIXME these should go in private. */
    GObject* module; /* FIXME once in private this can be MailtcModule */
    GArray* protocols;
    gchar* compatibility;
    gchar* name;
    gchar* author;
    gchar* description;
    gchar* directory;
} MailtcExtension;

typedef struct _MailtcExtensionClass
{
    GObjectClass parent_class;

    gboolean (*add_account)    (GObject* account);
    gboolean (*remove_account) (GObject* account);
    gboolean (*read_messages)  (GObject* account);
    gint64   (*get_messages)   (GObject* account, gboolean debug);
} MailtcExtensionClass;

typedef struct
{
    gchar* name;
    guint port;
} MailtcProtocol;

typedef MailtcExtension*
(*MailtcExtensionInitFunc)               (void);

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
                                          GObject*         account);

gboolean
mailtc_extension_remove_account          (MailtcExtension* extension,
                                          GObject*         account);

gboolean
mailtc_extension_read_messages           (MailtcExtension* extension,
                                          GObject*         account);

gint64
mailtc_extension_get_messages            (MailtcExtension* extension,
                                          GObject*         account,
                                          gboolean         debug);

GType
mailtc_extension_get_type                (void);

MailtcExtension*
mailtc_extension_new                     (void);

G_END_DECLS

#endif /* __MAILTC_EXTENSION_H__ */

