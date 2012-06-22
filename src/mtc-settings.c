/* mtc-settings.c
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

#include "mtc-settings.h"
#include "mtc-module.h"
#include "mtc-util.h"
#include "mtc.h"

#include <string.h>
#include <glib/gstdio.h>

#define MAILTC_SETTINGS_GROUP_GLOBAL         "settings"
#define MAILTC_SETTINGS_PROPERTY_ACCOUNTS    "accounts"
#define MAILTC_SETTINGS_PROPERTY_COMMAND     "command"
#define MAILTC_SETTINGS_PROPERTY_FILENAME    "filename"
#define MAILTC_SETTINGS_PROPERTY_ICON_COLOUR "iconcolour"
#define MAILTC_SETTINGS_PROPERTY_INTERVAL    "interval"
#define MAILTC_SETTINGS_PROPERTY_MODULES     "modules"
#define MAILTC_SETTINGS_PROPERTY_NET_ERROR   "neterror"

#define MAILTC_ACCOUNT_PROPERTY_NAME         "name"
#define MAILTC_ACCOUNT_PROPERTY_SERVER       "server"
#define MAILTC_ACCOUNT_PROPERTY_PORT         "port"
#define MAILTC_ACCOUNT_PROPERTY_USER         "user"
#define MAILTC_ACCOUNT_PROPERTY_PASSWORD     "password"
#define MAILTC_ACCOUNT_PROPERTY_PROTOCOL     "protocol"
#define MAILTC_ACCOUNT_PROPERTY_MODULE       "plugin"
#define MAILTC_ACCOUNT_PROPERTY_ICON_COLOUR  "iconcolour"

struct _MailtcSettingsPrivate
{
    GKeyFile* key_file;
    GError* error;
};

struct _MailtcSettings
{
    GObject parent_instance;

    MailtcSettingsPrivate* priv;
    GPtrArray* accounts;
    GPtrArray* modules;
    GdkColor iconcolour;
    guint interval;
    guint neterror;
    gchar* command;
    gchar* filename;
};

struct _MailtcSettingsClass
{
    GObjectClass parent_class;
};

enum
{
    PROP_0,
    PROP_ACCOUNTS,
    PROP_FILENAME,
    PROP_COMMAND,
    PROP_INTERVAL,
    PROP_MODULES,
    PROP_NET_ERROR,
    PROP_ICON_COLOUR
};

static gboolean
mailtc_settings_initable_init (GInitable*    initable,
                               GCancellable* cancellable,
                               GError**      error)
{
    MailtcSettings* settings;
    MailtcSettingsPrivate* priv;

    (void) cancellable;

    g_assert (MAILTC_IS_SETTINGS (initable));

    settings = MAILTC_SETTINGS (initable);
    priv = settings->priv;

    if (priv->error)
    {
        if (error)
            *error = g_error_copy (priv->error);

        return FALSE;
    }
    return TRUE;
}

static void
mailtc_settings_initable_iface_init (GInitableIface* iface)
{
    iface->init = mailtc_settings_initable_init;
}

G_DEFINE_TYPE_WITH_CODE (MailtcSettings, mailtc_settings, G_TYPE_OBJECT,
        G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, mailtc_settings_initable_iface_init))

static void
mailtc_settings_string_to_colour (const gchar* str,
                                  GdkColor*    colour)
{
    guint64 cvalue;

    g_assert (colour);

    cvalue = g_ascii_strtoull (str, NULL, 16);
    colour->red = (guint16) ((cvalue >> 32) & 0xFFFF);
    colour->green = (guint16) ((cvalue >> 16) & 0xFFFF);
    colour->blue = (guint16) (cvalue & 0xFFFF);
}

static void
mailtc_settings_keyfile_write_uint (MailtcSettings* settings,
                                    const gchar*    name)
{
    GKeyFile* key_file;
    guint value;

    g_assert (MAILTC_IS_SETTINGS (settings));

    key_file = settings->priv->key_file;

    g_object_get (G_OBJECT (settings), name, &value, NULL);

    g_key_file_set_integer (key_file, MAILTC_SETTINGS_GROUP_GLOBAL, name, (gint) value);
}

static gboolean
mailtc_settings_keyfile_read_uint (MailtcSettings* settings,
                                   const gchar*    name,
                                   GError**        error)
{
    GKeyFile* key_file;
    guint value;

    g_assert (MAILTC_IS_SETTINGS (settings));

    key_file = settings->priv->key_file;

    value = (guint) g_key_file_get_integer (key_file, MAILTC_SETTINGS_GROUP_GLOBAL, name, error);
    if (error && *error)
        return FALSE;

    g_object_set (G_OBJECT (settings), name, value, NULL);

    return TRUE;
}

static void
mailtc_settings_keyfile_write_string (MailtcSettings* settings,
                                      const gchar*    name)
{
    GKeyFile* key_file;
    gchar* value = NULL;

    g_assert (MAILTC_IS_SETTINGS (settings));

    key_file = settings->priv->key_file;

    g_object_get (G_OBJECT (settings), name, &value, NULL);

    g_key_file_set_string (key_file, MAILTC_SETTINGS_GROUP_GLOBAL, name, value);
    g_free (value);
}

static gboolean
mailtc_settings_keyfile_read_string (MailtcSettings* settings,
                                     const gchar*    name,
                                     GError**        error)
{
    GKeyFile* key_file;
    gchar* value;

    g_assert (MAILTC_IS_SETTINGS (settings));

    key_file = settings->priv->key_file;

    value = g_key_file_get_string (key_file, MAILTC_SETTINGS_GROUP_GLOBAL, name, error);
    if (error && *error)
        return FALSE;

    g_object_set (G_OBJECT (settings), name, value, NULL);
    g_free (value);

    return TRUE;
}

static void
mailtc_settings_keyfile_write_colour (MailtcSettings* settings,
                                      const gchar*    name)
{
    GKeyFile* key_file;
    gchar* value;
    GdkColor* colour = NULL;

    g_assert (MAILTC_IS_SETTINGS (settings));

    key_file = settings->priv->key_file;

    g_object_get (G_OBJECT (settings), name, &colour, NULL);
    value = gdk_color_to_string (colour);

    g_key_file_set_string (key_file, MAILTC_SETTINGS_GROUP_GLOBAL, name, value);

    g_free (value);
    gdk_color_free (colour);
}

static gboolean
mailtc_settings_keyfile_read_colour (MailtcSettings* settings,
                                     const gchar*    name,
                                     GError**        error)
{
    GKeyFile* key_file;
    gchar* value;
    GdkColor colour;

    g_assert (MAILTC_IS_SETTINGS (settings));

    key_file = settings->priv->key_file;

    value = g_key_file_get_string (key_file, MAILTC_SETTINGS_GROUP_GLOBAL, name, error);
    if (error && *error)
        return FALSE;

    mailtc_settings_string_to_colour (value, &colour);
    g_free (value);

    g_object_set (G_OBJECT (settings), name, &colour, NULL);

    return TRUE;
}

static void
mailtc_settings_keyfile_write_accounts (MailtcSettings* settings)
{
    GKeyFile* key_file;
    mtc_account* account;
    GPtrArray* accounts;
    gchar* colour;
    gchar* key_group;
    gchar** groups;
    gchar* password;
    const gchar* module_name;
    guint i;

    g_assert (MAILTC_IS_SETTINGS (settings));
    g_assert (settings->modules);
    g_assert (settings->accounts);

    key_file = settings->priv->key_file;
    accounts = settings->accounts;

    groups = g_new0 (gchar *, accounts->len);
    for (i = 0; i < accounts->len; i++)
    {
        account = g_ptr_array_index (accounts, i);
        groups[i] = key_group = g_strdup_printf ("account%u", i);

        g_key_file_set_string (key_file, key_group, MAILTC_ACCOUNT_PROPERTY_NAME, account->name);
        g_key_file_set_string (key_file, key_group, MAILTC_ACCOUNT_PROPERTY_SERVER, account->server);
        g_key_file_set_integer (key_file, key_group, MAILTC_ACCOUNT_PROPERTY_PORT, account->port);
        g_key_file_set_string (key_file, key_group, MAILTC_ACCOUNT_PROPERTY_USER, account->user);
        g_key_file_set_integer (key_file, key_group, MAILTC_ACCOUNT_PROPERTY_PROTOCOL, account->protocol);

        module_name = mailtc_module_get_name (account->plugin->module);
        g_key_file_set_string (key_file, key_group, MAILTC_ACCOUNT_PROPERTY_MODULE, module_name);

        colour = gdk_color_to_string (account->icon_colour);
        g_key_file_set_string (key_file, key_group, MAILTC_ACCOUNT_PROPERTY_ICON_COLOUR, colour + 1);
        g_free (colour);

        password = g_base64_encode ((const guchar*) account->password, strlen (account->password));
        g_key_file_set_string (key_file, key_group, MAILTC_ACCOUNT_PROPERTY_PASSWORD, password);
        g_free (password);
        g_free (key_group);
    }
    g_key_file_set_string_list (key_file, key_group, MAILTC_SETTINGS_PROPERTY_ACCOUNTS, (const gchar**) groups, accounts->len);
    g_strfreev (groups);
}

static gboolean
mailtc_settings_keyfile_read_accounts (MailtcSettings* settings,
                                       GError**        error)
{
    GKeyFile* key_file;
    gchar** account_groups;
    gsize naccounts;

    g_assert (MAILTC_IS_SETTINGS (settings));
    g_assert (settings->modules);

    key_file = settings->priv->key_file;

    account_groups = g_key_file_get_string_list (key_file, MAILTC_SETTINGS_GROUP_GLOBAL,
                                                 MAILTC_SETTINGS_PROPERTY_ACCOUNTS, &naccounts, error);
    if (!account_groups)
        return FALSE;

    if (naccounts > 0)
    {
        GPtrArray* accounts;
        GPtrArray* modules;
        GdkColor colour;
        gchar* str;
        const gchar* key_group;
        const gchar* module_name;
        gsize i;
        gsize j;
        gsize len;
        gboolean success;
        mtc_account* account;
        mtc_plugin* plugin = NULL;

        accounts = g_ptr_array_new ();
        modules = settings->modules;

        for (i = 0; i < naccounts; i++)
        {
            success = FALSE;

            key_group = account_groups[i];
            if (!g_key_file_has_group (key_file, key_group))
                continue;

            account = g_new0 (mtc_account, 1);

            account->name = g_key_file_get_string (key_file, key_group, MAILTC_ACCOUNT_PROPERTY_NAME, error);
            if (!*error)
                account->server = g_key_file_get_string (key_file, key_group, MAILTC_ACCOUNT_PROPERTY_SERVER, error);
            if (!*error)
                account->port = g_key_file_get_integer (key_file, key_group, MAILTC_ACCOUNT_PROPERTY_PORT, error);
            if (!*error)
                account->user = g_key_file_get_string (key_file, key_group, MAILTC_ACCOUNT_PROPERTY_USER, error);
            if (!*error)
                account->protocol = g_key_file_get_integer (key_file, key_group, MAILTC_ACCOUNT_PROPERTY_PROTOCOL, error);
            if (!*error)
            {
                str = g_key_file_get_string (key_file, key_group, MAILTC_ACCOUNT_PROPERTY_MODULE, error);

                for (j = 0; j < modules->len; j++)
                {
                    plugin = g_ptr_array_index (modules, j);

                    module_name = mailtc_module_get_name (plugin->module);
                    if (!g_strcmp0 (str, module_name))
                    {
                        account->plugin = plugin;
                        break;
                    }
                }
                g_free (str);
            }
            if (!*error)
            {
                str = g_key_file_get_string (key_file, key_group, MAILTC_ACCOUNT_PROPERTY_ICON_COLOUR, error);
                mailtc_settings_string_to_colour (str, &colour);
                account->icon_colour = gdk_color_copy (&colour);
                g_free (str);
            }
            if (!*error)
            {
                str = g_key_file_get_string (key_file, key_group, MAILTC_ACCOUNT_PROPERTY_PASSWORD, error);
                account->password = (gchar*) g_base64_decode (str, &len);
                g_free (str);

                if (*error)
                {
                    g_free (account->password);
                    account->password = NULL;
                }
            }
            if (!*error)
            {
                g_assert (account->name &&
                          account->server &&
                          account->port &&
                          account->user &&
                          account->password &&
                          account->plugin &&
                          account->icon_colour);

                if (plugin->add_account)
                    success = (*plugin->add_account) (account, error);
                else
                    success = TRUE;
            }
            if (success)
                g_ptr_array_add (accounts, account);
            else
                mailtc_free_account (account, error);
        }
        if (accounts->len > 0)
            g_object_set (G_OBJECT (settings), MAILTC_SETTINGS_PROPERTY_ACCOUNTS, accounts, NULL);

        g_ptr_array_unref (accounts);
    }
    g_strfreev (account_groups);

    return TRUE;
}

static gboolean
mailtc_settings_keyfile_read (MailtcSettings* settings,
                              GError**        error)
{

    if (!mailtc_settings_keyfile_read_uint (settings, MAILTC_SETTINGS_PROPERTY_INTERVAL, error))
        return FALSE;
    if (!mailtc_settings_keyfile_read_uint (settings, MAILTC_SETTINGS_PROPERTY_NET_ERROR, error))
        return FALSE;
    if (!mailtc_settings_keyfile_read_string (settings, MAILTC_SETTINGS_PROPERTY_COMMAND, error))
        return FALSE;
    if (!mailtc_settings_keyfile_read_colour (settings, MAILTC_SETTINGS_PROPERTY_ICON_COLOUR, error))
        return FALSE;

    return mailtc_settings_keyfile_read_accounts (settings, error);
}

gboolean
mailtc_settings_write (MailtcSettings* settings,
                       GError**        error)
{
    gchar* data;
    const gchar* filename;
    gboolean success;

    g_assert (MAILTC_IS_SETTINGS (settings));

    filename = settings->filename;

    if (g_file_test (filename, G_FILE_TEST_EXISTS))
        g_chmod (filename, S_IRUSR | S_IWUSR);

    mailtc_settings_keyfile_write_uint (settings, MAILTC_SETTINGS_PROPERTY_INTERVAL);
    mailtc_settings_keyfile_write_uint (settings, MAILTC_SETTINGS_PROPERTY_NET_ERROR);
    mailtc_settings_keyfile_write_string (settings, MAILTC_SETTINGS_PROPERTY_COMMAND);
    mailtc_settings_keyfile_write_colour (settings, MAILTC_SETTINGS_PROPERTY_ICON_COLOUR);

    mailtc_settings_keyfile_write_accounts (settings);

    data = g_key_file_to_data (settings->priv->key_file, NULL, error);
    if (data)
    {
        success = g_file_set_contents (filename, data, -1, error);
        g_free (data);
    }
    else
        success = FALSE;

    if (g_file_test (filename, G_FILE_TEST_EXISTS))
        g_chmod (filename, S_IRUSR);

    return success;
}


static void
mailtc_settings_set_filename (MailtcSettings* settings,
                              const gchar*    filename)
{
    g_assert (MAILTC_IS_SETTINGS (settings));

    if (g_strcmp0 (filename, settings->filename) != 0)
    {
        g_free (settings->filename);
        settings->filename = g_strdup (filename);

        g_object_notify (G_OBJECT (settings), MAILTC_SETTINGS_PROPERTY_FILENAME);
    }
}

void
mailtc_settings_set_command (MailtcSettings* settings,
                             const gchar*    command)
{
    g_assert (MAILTC_IS_SETTINGS (settings));

    if (g_strcmp0 (command, settings->command) != 0)
    {
        g_free (settings->command);
        settings->command = g_strdup (command);

        g_object_notify (G_OBJECT (settings), MAILTC_SETTINGS_PROPERTY_COMMAND);
    }
}

const gchar*
mailtc_settings_get_command (MailtcSettings* settings)
{
    g_assert (MAILTC_IS_SETTINGS (settings));

    return settings->command;
}

void
mailtc_settings_set_interval (MailtcSettings* settings,
                              guint           interval)
{
    g_assert (MAILTC_IS_SETTINGS (settings));

    if (interval != settings->interval)
    {
        settings->interval = interval;
        g_object_notify (G_OBJECT (settings), MAILTC_SETTINGS_PROPERTY_INTERVAL);
    }
}

guint
mailtc_settings_get_interval (MailtcSettings* settings)
{
    g_assert (MAILTC_IS_SETTINGS (settings));

    return settings->interval;
}

void
mailtc_settings_set_neterror (MailtcSettings* settings,
                              guint           neterror)
{
    g_assert (MAILTC_IS_SETTINGS (settings));

    if (neterror != settings->neterror)
    {
        settings->neterror = neterror;
        g_object_notify (G_OBJECT (settings), MAILTC_SETTINGS_PROPERTY_NET_ERROR);
    }
}

guint
mailtc_settings_get_neterror (MailtcSettings* settings)
{
    g_assert (MAILTC_IS_SETTINGS (settings));

    return settings->neterror;
}

void
mailtc_settings_set_iconcolour (MailtcSettings* settings,
                                const GdkColor* colour)
{
    GdkColor defaultcolour;
    GdkColor* iconcolour;

    g_assert (MAILTC_IS_SETTINGS (settings));

    iconcolour = &settings->iconcolour;

    if (!colour)
    {
        defaultcolour.red = defaultcolour.green = defaultcolour.blue = 0xFFFF;
        colour = &defaultcolour;
    }
    if (!gdk_color_equal (colour, iconcolour))
    {
        iconcolour->red = colour->red;
        iconcolour->green = colour->green;
        iconcolour->blue = colour->blue;

        g_object_notify (G_OBJECT (settings), MAILTC_SETTINGS_PROPERTY_ICON_COLOUR);
    }
}

void
mailtc_settings_get_iconcolour (MailtcSettings* settings,
                                GdkColor*       colour)
{
    GdkColor* iconcolour;

    g_assert (MAILTC_IS_SETTINGS (settings));

    iconcolour = &settings->iconcolour;

    colour->red = iconcolour->red;
    colour->green = iconcolour->green;
    colour->blue = iconcolour->blue;
}

static void
mailtc_settings_set_accounts (MailtcSettings* settings,
                              GPtrArray*      accounts)
{
    g_assert (MAILTC_IS_SETTINGS (settings));

    if (accounts != settings->accounts)
    {
        if (settings->accounts)
            g_ptr_array_unref (settings->accounts);

        settings->accounts = accounts ? g_ptr_array_ref (accounts) : NULL;
        g_object_notify (G_OBJECT (settings), MAILTC_SETTINGS_PROPERTY_ACCOUNTS);
    }
}

GPtrArray*
mailtc_settings_get_accounts (MailtcSettings* settings)
{
    g_assert (MAILTC_IS_SETTINGS (settings));

    return settings->accounts ? g_ptr_array_ref (settings->accounts) : NULL;
}

static void
mailtc_settings_set_modules (MailtcSettings* settings,
                             GPtrArray*      modules)
{
    g_assert (MAILTC_IS_SETTINGS (settings));

    if (modules != settings->modules)
    {
        if (settings->modules)
            g_ptr_array_unref (settings->modules);

        settings->modules = modules ? g_ptr_array_ref (modules) : NULL;
        g_object_notify (G_OBJECT (settings), MAILTC_SETTINGS_PROPERTY_MODULES);
    }
}

GPtrArray*
mailtc_settings_get_modules (MailtcSettings* settings)
{
    g_assert (MAILTC_IS_SETTINGS (settings));

    return settings->modules;
}

static gboolean
mailtc_settings_free_accounts (MailtcSettings* settings,
                               GError**        error)
{
    g_assert (MAILTC_IS_SETTINGS (settings));

    if (settings->accounts)
    {
        g_ptr_array_foreach (settings->accounts, (GFunc) mailtc_free_account, error);
        g_ptr_array_unref (settings->accounts);
        settings->accounts = NULL;
    }
    return ((error && *error) ? FALSE : TRUE);
}

static void
mailtc_settings_set_property (GObject*      object,
                              guint         prop_id,
                              const GValue* value,
                              GParamSpec*   pspec)
{
    MailtcSettings* settings = MAILTC_SETTINGS (object);

    switch (prop_id)
    {
        case PROP_FILENAME:
            mailtc_settings_set_filename (settings, g_value_get_string (value));
            break;

        case PROP_COMMAND:
            mailtc_settings_set_command (settings, g_value_get_string (value));
            break;

        case PROP_INTERVAL:
            mailtc_settings_set_interval (settings, g_value_get_uint (value));
            break;

        case PROP_NET_ERROR:
            mailtc_settings_set_neterror (settings, g_value_get_uint (value));
            break;

        case PROP_ICON_COLOUR:
            mailtc_settings_set_iconcolour (settings, g_value_get_boxed (value));
            break;

        case PROP_ACCOUNTS:
            mailtc_settings_set_accounts (settings, g_value_get_boxed (value));
            break;

        case PROP_MODULES:
            mailtc_settings_set_modules (settings, g_value_get_boxed (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_settings_get_property (GObject*    object,
                              guint       prop_id,
                              GValue*     value,
                              GParamSpec* pspec)
{
    MailtcSettings* settings;
    GdkColor colour;

    settings = MAILTC_SETTINGS (object);

    switch (prop_id)
    {
        case PROP_FILENAME:
            g_value_set_string (value, settings->filename);
            break;

        case PROP_COMMAND:
            g_value_set_string (value, mailtc_settings_get_command (settings));
            break;

        case PROP_INTERVAL:
            g_value_set_uint (value, mailtc_settings_get_interval (settings));
            break;

        case PROP_NET_ERROR:
            g_value_set_uint (value, mailtc_settings_get_neterror (settings));
            break;

        case PROP_ICON_COLOUR:
            mailtc_settings_get_iconcolour (settings, &colour);
            g_value_set_boxed (value, &colour);
            break;

        case PROP_ACCOUNTS:
            g_value_set_boxed (value, settings->accounts);
            break;

        case PROP_MODULES:
            g_value_set_boxed (value, settings->modules);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_settings_finalize (GObject* object)
{
    MailtcSettings* settings;
    MailtcSettingsPrivate* priv;

    settings = MAILTC_SETTINGS (object);
    priv = settings->priv;

    /* FIXME error is ignored */
    mailtc_settings_free_accounts (settings, NULL);

    if (settings->modules)
        g_ptr_array_unref (settings->modules);

    g_free (settings->command);
    g_free (settings->filename);

    g_clear_error (&priv->error);

    if (priv->key_file)
        g_key_file_free (priv->key_file);

    G_OBJECT_CLASS (mailtc_settings_parent_class)->finalize (object);
}

static void
mailtc_settings_constructed (GObject* object)
{
    MailtcSettings* settings;
    MailtcSettingsPrivate* priv;

    settings = MAILTC_SETTINGS (object);
    priv = settings->priv;

    priv->key_file = g_key_file_new ();

    if (g_key_file_load_from_file (priv->key_file, settings->filename, G_KEY_FILE_NONE, &priv->error))
        mailtc_settings_keyfile_read (settings, &priv->error);

    G_OBJECT_CLASS (mailtc_settings_parent_class)->constructed (object);
}

static void
mailtc_settings_class_init (MailtcSettingsClass* class)
{
    GObjectClass* gobject_class;
    GParamFlags flags;

    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->finalize = mailtc_settings_finalize;
    gobject_class->constructed = mailtc_settings_constructed;
    gobject_class->set_property = mailtc_settings_set_property;
    gobject_class->get_property = mailtc_settings_get_property;

    flags = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT;

    g_object_class_install_property (gobject_class,
                                     PROP_FILENAME,
                                     g_param_spec_string (
                                     MAILTC_SETTINGS_PROPERTY_FILENAME,
                                     "Filename",
                                     "The settings filename",
                                     NULL,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_INTERVAL,
                                     g_param_spec_uint (
                                     MAILTC_SETTINGS_PROPERTY_INTERVAL,
                                     "Interval",
                                     "The mail checking interval in minutes",
                                     1,
                                     60,
                                     3,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_NET_ERROR,
                                     g_param_spec_uint (
                                     MAILTC_SETTINGS_PROPERTY_NET_ERROR,
                                     "Neterror",
                                     "The number of network errors before reporting",
                                     0,
                                     5,
                                     1,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_ICON_COLOUR,
                                     g_param_spec_boxed (
                                     MAILTC_SETTINGS_PROPERTY_ICON_COLOUR,
                                     "Iconcolour",
                                     "The icon colour",
                                     GDK_TYPE_COLOR,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_COMMAND,
                                     g_param_spec_string (
                                     MAILTC_SETTINGS_PROPERTY_COMMAND,
                                     "Command",
                                     "The mail command to execute",
                                     NULL,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_ACCOUNTS,
                                     g_param_spec_boxed (
                                     MAILTC_SETTINGS_PROPERTY_ACCOUNTS,
                                     "Accounts",
                                     "The mail accounts",
                                     G_TYPE_PTR_ARRAY,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_MODULES,
                                     g_param_spec_boxed (
                                     MAILTC_SETTINGS_PROPERTY_MODULES,
                                     "Modules",
                                     "The mail plugin modules",
                                     G_TYPE_PTR_ARRAY,
                                     flags));

    g_type_class_add_private (class, sizeof (MailtcSettingsPrivate));
}

static void
mailtc_settings_init (MailtcSettings* settings)
{
    MailtcSettingsPrivate* priv;

    priv = settings->priv = G_TYPE_INSTANCE_GET_PRIVATE (settings,
                        MAILTC_TYPE_SETTINGS, MailtcSettingsPrivate);

    priv->key_file = NULL;
    priv->error = NULL;
}

MailtcSettings*
mailtc_settings_new (gchar*     filename,
                     GPtrArray* modules,
                     GError**   error)
{
    return g_initable_new (MAILTC_TYPE_SETTINGS, NULL, error,
                           MAILTC_SETTINGS_PROPERTY_FILENAME, filename,
                           MAILTC_SETTINGS_PROPERTY_MODULES, modules,
                           NULL);
}

