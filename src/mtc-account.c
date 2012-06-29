/* mtc-account.c
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

#include "mtc-account.h"

#define MAILTC_ACCOUNT_SET_STRING(account,property) \
    mailtc_object_set_string (G_OBJECT (account), MAILTC_TYPE_ACCOUNT, \
                              #property, &account->property, property)

#define MAILTC_ACCOUNT_SET_UINT(account,property) \
    mailtc_object_set_uint (G_OBJECT (account), MAILTC_TYPE_ACCOUNT, \
                            #property, &account->property, property)

#define MAILTC_ACCOUNT_SET_COLOUR(account,property) \
    mailtc_object_set_colour (G_OBJECT (account), MAILTC_TYPE_ACCOUNT, \
                            #property, &account->property, property)

struct _MailtcAccount
{
    GObject parent_instance;

    GdkColor iconcolour;
    gchar* name;
    gchar* server;
    gchar* user;
    gchar* password;
    guint port;
    guint protocol;
};

struct _MailtcAccountClass
{
    GObjectClass parent_class;
};

G_DEFINE_TYPE (MailtcAccount, mailtc_account, G_TYPE_OBJECT)

enum
{
    PROP_0,
    PROP_NAME,
    PROP_SERVER,
    PROP_PORT,
    PROP_USER,
    PROP_PASSWORD,
    PROP_PROTOCOL,
    PROP_MODULE,
    PROP_ICON_COLOUR
};

static void
mailtc_object_set_string (GObject*     obj,
                          GType        objtype,
                          const gchar* name,
                          gchar**      value,
                          const gchar* newvalue)
{
    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (obj, objtype));

    if (g_strcmp0 (newvalue, *value) != 0)
    {
        g_free (*value);
        *value = g_strdup (newvalue);

        g_object_notify (obj, name);
    }
}

static void
mailtc_object_set_uint (GObject*     obj,
                        GType        objtype,
                        const gchar* name,
                        guint*       value,
                        const guint  newvalue)
{
    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (obj, objtype));

    if (newvalue != *value)
    {
        *value = newvalue;

        g_object_notify (obj, name);
    }
}

static void
mailtc_object_set_colour (GObject*        obj,
                          GType           objtype,
                          const gchar*    name,
                          GdkColor*       colour,
                          const GdkColor* newcolour)
{
    GdkColor defaultcolour;

    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (obj, objtype));

    if (!newcolour)
    {
        defaultcolour.red = defaultcolour.green = defaultcolour.blue = 0xFFFF;
        newcolour = &defaultcolour;
    }
    if (!gdk_color_equal (newcolour, colour))
    {
        colour->red = newcolour->red;
        colour->green = newcolour->green;
        colour->blue = newcolour->blue;

        g_object_notify (obj, name);
    }
}

void
mailtc_account_set_name (MailtcAccount* account,
                         const gchar*   name)
{
    MAILTC_ACCOUNT_SET_STRING (account, name);
}

const gchar*
mailtc_account_get_name (MailtcAccount* account)
{
    g_assert (MAILTC_IS_ACCOUNT (account));

    return account->name;
}

void
mailtc_account_set_server (MailtcAccount* account,
                           const gchar*   server)
{
    MAILTC_ACCOUNT_SET_STRING (account, server);
}

const gchar*
mailtc_account_get_server (MailtcAccount* account)
{
    g_assert (MAILTC_IS_ACCOUNT (account));

    return account->server;
}

void
mailtc_account_set_port (MailtcAccount* account,
                         guint          port)
{
    MAILTC_ACCOUNT_SET_UINT (account, port);
}

guint
mailtc_account_get_port (MailtcAccount* account)
{
    g_assert (MAILTC_IS_ACCOUNT (account));

    return account->port;
}

void
mailtc_account_set_user (MailtcAccount* account,
                         const gchar*   user)
{
    MAILTC_ACCOUNT_SET_STRING (account, user);
}

const gchar*
mailtc_account_get_user (MailtcAccount* account)
{
    g_assert (MAILTC_IS_ACCOUNT (account));

    return account->user;
}

void
mailtc_account_set_password (MailtcAccount* account,
                             const gchar*   password)
{
    MAILTC_ACCOUNT_SET_STRING (account, password);
}

const gchar*
mailtc_account_get_password (MailtcAccount* account)
{
    g_assert (MAILTC_IS_ACCOUNT (account));

    return account->password;
}

void
mailtc_account_set_protocol (MailtcAccount* account,
                             guint          protocol)
{
    MAILTC_ACCOUNT_SET_UINT (account, protocol);
}

guint
mailtc_account_get_protocol (MailtcAccount* account)
{
    g_assert (MAILTC_IS_ACCOUNT (account));

    return account->protocol;
}

void
mailtc_account_set_iconcolour (MailtcAccount*  account,
                               const GdkColor* iconcolour)
{
    MAILTC_ACCOUNT_SET_COLOUR (account, iconcolour);
}

void
mailtc_account_get_iconcolour (MailtcAccount* account,
                               GdkColor*      iconcolour)
{
    g_assert (MAILTC_IS_ACCOUNT (account));

    iconcolour->red = account->iconcolour.red;
    iconcolour->green = account->iconcolour.green;
    iconcolour->blue = account->iconcolour.blue;
}

static void
mailtc_account_set_property (GObject*      object,
                             guint         prop_id,
                             const GValue* value,
                             GParamSpec*   pspec)
{
    MailtcAccount* account = MAILTC_ACCOUNT (object);

    switch (prop_id)
    {
        case PROP_NAME:
            mailtc_account_set_name (account, g_value_get_string (value));
            break;

        case PROP_SERVER:
            mailtc_account_set_server (account, g_value_get_string (value));
            break;

        case PROP_PORT:
            mailtc_account_set_port (account, g_value_get_uint (value));
            break;

        case PROP_USER:
            mailtc_account_set_user (account, g_value_get_string (value));
            break;

        case PROP_PASSWORD:
            mailtc_account_set_password (account, g_value_get_string (value));
            break;

        case PROP_PROTOCOL:
            mailtc_account_set_protocol (account, g_value_get_uint (value));
            break;

        case PROP_ICON_COLOUR:
            mailtc_account_set_iconcolour (account, g_value_get_boxed (value));
            break;

            /* FIXME also need to do module */
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_account_get_property (GObject*    object,
                             guint       prop_id,
                             GValue*     value,
                             GParamSpec* pspec)
{
    MailtcAccount* account = MAILTC_ACCOUNT (object);
    GdkColor colour;

    switch (prop_id)
    {
        case PROP_NAME:
            g_value_set_string (value, mailtc_account_get_name (account));
            break;

        case PROP_SERVER:
            g_value_set_string (value, mailtc_account_get_server (account));
            break;

        case PROP_PORT:
            g_value_set_uint (value, mailtc_account_get_port (account));
            break;

        case PROP_USER:
            g_value_set_string (value, mailtc_account_get_user (account));
            break;

        case PROP_PASSWORD:
            g_value_set_string (value, mailtc_account_get_password (account));
            break;

        case PROP_PROTOCOL:
            g_value_set_uint (value, mailtc_account_get_protocol (account));
            break;

        case PROP_ICON_COLOUR:
            mailtc_account_get_iconcolour (account, &colour);
            g_value_set_boxed (value, &colour);
            break;

            /* FIXME also need to do module */

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_account_finalize (GObject* object)
{
    MailtcAccount* account;

    account = MAILTC_ACCOUNT (object);

    g_free (account->password);
    g_free (account->user);
    g_free (account->server);
    g_free (account->name);

    G_OBJECT_CLASS (mailtc_account_parent_class)->finalize (object);
}

static void
mailtc_account_class_init (MailtcAccountClass* class)
{
    GObjectClass* gobject_class;
    GParamFlags common_flags;

    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->finalize = mailtc_account_finalize;
    gobject_class->set_property = mailtc_account_set_property;
    gobject_class->get_property = mailtc_account_get_property;

    common_flags = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS;

    g_object_class_install_property (gobject_class,
                                     PROP_NAME,
                                     g_param_spec_string (
                                     MAILTC_ACCOUNT_PROPERTY_NAME,
                                     "Name",
                                     "The account name",
                                     NULL,
                                     common_flags | G_PARAM_CONSTRUCT));

    g_object_class_install_property (gobject_class,
                                     PROP_SERVER,
                                     g_param_spec_string (
                                     MAILTC_ACCOUNT_PROPERTY_SERVER,
                                     "Server",
                                     "The account server",
                                     NULL,
                                     common_flags | G_PARAM_CONSTRUCT));

    g_object_class_install_property (gobject_class,
                                     PROP_PORT,
                                     g_param_spec_uint (
                                     MAILTC_ACCOUNT_PROPERTY_PORT,
                                     "Port",
                                     "The account port",
                                     0,
                                     G_MAXUINT,
                                     0,
                                     common_flags | G_PARAM_CONSTRUCT));

    g_object_class_install_property (gobject_class,
                                     PROP_USER,
                                     g_param_spec_string (
                                     MAILTC_ACCOUNT_PROPERTY_USER,
                                     "User",
                                     "The account user",
                                     NULL,
                                     common_flags | G_PARAM_CONSTRUCT));

    g_object_class_install_property (gobject_class,
                                     PROP_PASSWORD,
                                     g_param_spec_string (
                                     MAILTC_ACCOUNT_PROPERTY_PASSWORD,
                                     "Password",
                                     "The account password",
                                     NULL,
                                     common_flags | G_PARAM_CONSTRUCT));

    g_object_class_install_property (gobject_class,
                                     PROP_PORT,
                                     g_param_spec_uint (
                                     MAILTC_ACCOUNT_PROPERTY_PROTOCOL,
                                     "Protocol",
                                     "The account protocol",
                                     0,
                                     G_MAXUINT,
                                     0,
                                     common_flags | G_PARAM_CONSTRUCT));

    g_object_class_install_property (gobject_class,
                                     PROP_ICON_COLOUR,
                                     g_param_spec_boxed (
                                     MAILTC_ACCOUNT_PROPERTY_ICON_COLOUR,
                                     "Iconcolour",
                                     "The icon colour",
                                     GDK_TYPE_COLOR,
                                     common_flags));

}

static void
mailtc_account_init (MailtcAccount* account)
{
    (void) account;
}

MailtcAccount*
mailtc_account_new (void)
{
    return g_object_new (MAILTC_TYPE_ACCOUNT, NULL);
}

