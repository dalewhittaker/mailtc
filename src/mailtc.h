/* mailtc.h
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

#ifndef __MAILTC_H__
#define __MAILTC_H__

#include <glib-object.h>

#define MAILTC_DEFINE_TYPE(TN, t_n, t_p)                   G_DEFINE_TYPE (TN, t_n, g_type_from_name (#t_p))
#define MAILTC_DEFINE_EXTENSION(TN, t_n)                   MAILTC_DEFINE_TYPE (TN, t_n, MailtcExtension)
#define MAILTC_EXTENSION_CHECK_CLASS_CAST(klass, t_p, t_c) G_TYPE_CHECK_CLASS_CAST ((klass), g_type_from_name (#t_p), t_c)
#define MAILTC_BASE_EXTENSION_CLASS(klass)                 MAILTC_EXTENSION_CHECK_CLASS_CAST ((klass), MailtcExtension, MailtcExtensionClass)
#define MAILTC_OBJECT_GET_PROPERTY(obj, name, prop)        g_object_get (G_OBJECT (obj), name, &prop, NULL)
#define MAILTC_OBJECT_SET_PROPERTY(obj, name, prop)        g_object_set (G_OBJECT (obj), name, prop, NULL)

#define MAILTC_EXTENSION_SYMBOL_INIT                       "extension_init"

#define MAILTC_EXTENSION_PROPERTY_COMPATIBILITY            "compatibility"
#define MAILTC_EXTENSION_PROPERTY_NAME                     "name"
#define MAILTC_EXTENSION_PROPERTY_AUTHOR                   "author"
#define MAILTC_EXTENSION_PROPERTY_DESCRIPTION              "description"
#define MAILTC_EXTENSION_PROPERTY_DIRECTORY                "directory"
#define MAILTC_EXTENSION_PROPERTY_MODULE                   "module"
#define MAILTC_EXTENSION_PROPERTY_PROTOCOLS                "protocols"

#define MAILTC_ACCOUNT_PROPERTY_NAME                       "name"
#define MAILTC_ACCOUNT_PROPERTY_SERVER                     "server"
#define MAILTC_ACCOUNT_PROPERTY_PORT                       "port"
#define MAILTC_ACCOUNT_PROPERTY_USER                       "user"
#define MAILTC_ACCOUNT_PROPERTY_PASSWORD                   "password"
#define MAILTC_ACCOUNT_PROPERTY_PROTOCOL                   "protocol"
#define MAILTC_ACCOUNT_PROPERTY_MODULE                     "module"
#define MAILTC_ACCOUNT_PROPERTY_EXTENSION                  "extension"
#define MAILTC_ACCOUNT_PROPERTY_ICON_COLOUR                "iconcolour"
#define MAILTC_ACCOUNT_PROPERTY_PRIVATE                    "private"

#define MAILTC_EXTENSION_GET_DIRECTORY(obj, prop)          MAILTC_OBJECT_GET_PROPERTY (obj, MAILTC_EXTENSION_PROPERTY_DIRECTORY, prop)
#define MAILTC_EXTENSION_SET_DIRECTORY(obj, prop)          MAILTC_OBJECT_SET_PROPERTY (obj, MAILTC_EXTENSION_PROPERTY_DIRECTORY, prop)

#define MAILTC_ACCOUNT_GET_USER(obj, prop)                 MAILTC_OBJECT_GET_PROPERTY (obj, MAILTC_ACCOUNT_PROPERTY_USER, prop)
#define MAILTC_ACCOUNT_SET_USER(obj, prop)                 MAILTC_OBJECT_SET_PROPERTY (obj, MAILTC_ACCOUNT_PROPERTY_USER, prop)
#define MAILTC_ACCOUNT_GET_PASSWORD(obj, prop)             MAILTC_OBJECT_GET_PROPERTY (obj, MAILTC_ACCOUNT_PROPERTY_PASSWORD, prop)
#define MAILTC_ACCOUNT_SET_PASSWORD(obj, prop)             MAILTC_OBJECT_SET_PROPERTY (obj, MAILTC_ACCOUNT_PROPERTY_PASSWORD, prop)
#define MAILTC_ACCOUNT_GET_PRIVATE(obj, prop)              MAILTC_OBJECT_GET_PROPERTY (obj, MAILTC_ACCOUNT_PROPERTY_PRIVATE, prop)
#define MAILTC_ACCOUNT_SET_PRIVATE(obj, prop)              MAILTC_OBJECT_SET_PROPERTY (obj, MAILTC_ACCOUNT_PROPERTY_PRIVATE, prop)

typedef struct _MailtcProtocol         MailtcProtocol;
typedef struct _MailtcExtension        MailtcExtension;
typedef struct _MailtcExtensionClass   MailtcExtensionClass;
typedef struct _MailtcExtensionPrivate MailtcExtensionPrivate;

typedef gboolean
(*MailtcExtensionAddAccountFunc)       (MailtcExtension* extension, GObject* account);

typedef gboolean
(*MailtcExtensionRemoveAccountFunc)    (MailtcExtension* extension, GObject* account);

typedef gboolean
(*MailtcExtensionReadMessagesFunc)     (MailtcExtension* extension, GObject* account);

typedef gint64
(*MailtcExtensionGetMessagesFunc)      (MailtcExtension* extension, GObject* account, gboolean debug);

struct _MailtcProtocol
{
    gchar* name;
    guint port;
};

struct _MailtcExtension
{
    GObject parent_instance;
    struct _MailtcExtensionPrivate* priv;
};

struct _MailtcExtensionClass
{
    GObjectClass parent_class;

    MailtcExtensionAddAccountFunc    add_account;
    MailtcExtensionRemoveAccountFunc remove_account;
    MailtcExtensionReadMessagesFunc  read_messages;
    MailtcExtensionGetMessagesFunc   get_messages;
};

#endif /* __MAILTC_H__ */

