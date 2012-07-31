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

#define MAILTC_ACCOUNT_PROPERTY_NAME            "name"
#define MAILTC_ACCOUNT_PROPERTY_SERVER          "server"
#define MAILTC_ACCOUNT_PROPERTY_PORT            "port"
#define MAILTC_ACCOUNT_PROPERTY_USER            "user"
#define MAILTC_ACCOUNT_PROPERTY_PASSWORD        "password"
#define MAILTC_ACCOUNT_PROPERTY_PROTOCOL        "protocol"
#define MAILTC_ACCOUNT_PROPERTY_EXTENSION       "extension"
#define MAILTC_ACCOUNT_PROPERTY_ICON_COLOUR     "iconcolour"
#define MAILTC_ACCOUNT_PROPERTY_PRIVATE         "private"

typedef struct _MailtcProtocol         MailtcProtocol;
typedef struct _MailtcExtension        MailtcExtension;
typedef struct _MailtcExtensionClass   MailtcExtensionClass;
typedef struct _MailtcExtensionPrivate MailtcExtensionPrivate;

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

    gboolean (*add_account)    (GObject* account);
    gboolean (*remove_account) (GObject* account);
    gboolean (*read_messages)  (GObject* account);
    gint64   (*get_messages)   (GObject* account, gboolean debug);
};

#endif /* __MAILTC_H__ */

