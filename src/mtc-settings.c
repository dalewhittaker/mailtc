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
#include "mtc-account.h"
#include "mtc-extension.h"
#include "mtc-module.h"
#include "mtc-util.h"

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

#define MAILTC_SETTINGS_SET_STRING(settings,property) \
    mailtc_object_set_string (G_OBJECT (settings), MAILTC_TYPE_SETTINGS, \
                              #property, &settings->property, property)

#define MAILTC_SETTINGS_SET_UINT(settings,property) \
    mailtc_object_set_uint (G_OBJECT (settings), MAILTC_TYPE_SETTINGS, \
                            #property, &settings->property, property)

#define MAILTC_SETTINGS_SET_COLOUR(settings,property) \
    mailtc_object_set_colour (G_OBJECT (settings), MAILTC_TYPE_SETTINGS, \
                              #property, &settings->property, property)

#define MAILTC_SETTINGS_SET_PTR_ARRAY(settings,property) \
    mailtc_object_set_ptr_array (G_OBJECT (settings), MAILTC_TYPE_SETTINGS, \
                                 #property, &settings->property, property)


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
        if (g_error_matches (priv->error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_NOT_FOUND) ||
            g_error_matches (priv->error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND) ||
            g_error_matches (priv->error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_GROUP_NOT_FOUND) ||
            g_error_matches (priv->error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
        {
            g_clear_error (&priv->error);
        }
        else
        {
            if (error)
                *error = g_error_copy (priv->error);

            return FALSE;
        }
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
                                    GObject*        obj,
                                    const gchar*    key_group,
                                    const gchar*    name)
{
    GKeyFile* key_file;
    guint value;

    g_assert (MAILTC_IS_SETTINGS (settings));

    key_file = settings->priv->key_file;

    g_object_get (obj, name, &value, NULL);

    g_key_file_set_integer (key_file, key_group, name, (gint) value);
}

static gboolean
mailtc_settings_keyfile_read_uint (MailtcSettings* settings,
                                   GObject*        obj,
                                   const gchar*    key_group,
                                   const gchar*    name,
                                   GError**        error)
{
    GKeyFile* key_file;
    guint value;

    g_assert (MAILTC_IS_SETTINGS (settings));

    key_file = settings->priv->key_file;

    value = (guint) g_key_file_get_integer (key_file, key_group, name, error);
    if (error && *error)
        return FALSE;

    g_object_set (obj, name, value, NULL);

    return TRUE;
}

static void
mailtc_settings_keyfile_write_string (MailtcSettings* settings,
                                      GObject*        obj,
                                      const gchar*    key_group,
                                      const gchar*    name)
{
    GKeyFile* key_file;
    gchar* value = NULL;

    g_assert (MAILTC_IS_SETTINGS (settings));

    key_file = settings->priv->key_file;

    g_object_get (obj, name, &value, NULL);

    g_key_file_set_string (key_file, key_group, name, value);
    g_free (value);
}

static gboolean
mailtc_settings_keyfile_read_string (MailtcSettings* settings,
                                     GObject*        obj,
                                     const gchar*    key_group,
                                     const gchar*    name,
                                     GError**        error)
{
    GKeyFile* key_file;
    gchar* value;

    g_assert (MAILTC_IS_SETTINGS (settings));

    key_file = settings->priv->key_file;

    value = g_key_file_get_string (key_file, key_group, name, error);
    if (error && *error)
        return FALSE;

    g_object_set (obj, name, value, NULL);
    g_free (value);

    return TRUE;
}

static void
mailtc_settings_keyfile_write_base64 (MailtcSettings* settings,
                                      GObject*        obj,
                                      const gchar*    key_group,
                                      const gchar*    name)
{
    GKeyFile* key_file;
    gchar* str;
    gchar* value = NULL;

    g_assert (MAILTC_IS_SETTINGS (settings));

    key_file = settings->priv->key_file;

    g_object_get (obj, name, &value, NULL);

    str = g_base64_encode ((const guchar*) value, strlen (value));
    g_key_file_set_string (key_file, key_group, name, value);
    g_free (str);
    g_free (value);
}

static gboolean
mailtc_settings_keyfile_read_base64 (MailtcSettings* settings,
                                     GObject*        obj,
                                     const gchar*    key_group,
                                     const gchar*    name,
                                     GError**        error)
{
    GKeyFile* key_file;
    gchar* str;
    gchar* value;
    gsize len;

    g_assert (MAILTC_IS_SETTINGS (settings));

    key_file = settings->priv->key_file;

    str = g_key_file_get_string (key_file, key_group, name, error);
    if (error && *error)
        return FALSE;

    value = (gchar*) g_base64_decode (str, &len);
    g_object_set (obj, name, value, NULL);
    g_free (value);
    g_free (str);

    return TRUE;
}

static void
mailtc_settings_keyfile_write_colour (MailtcSettings* settings,
                                      GObject*        obj,
                                      const gchar*    key_group,
                                      const gchar*    name)
{
    GKeyFile* key_file;
    gchar* value;
    GdkColor* colour = NULL;

    g_assert (MAILTC_IS_SETTINGS (settings));

    key_file = settings->priv->key_file;

    g_object_get (obj, name, &colour, NULL);
    value = gdk_color_to_string (colour);

    g_key_file_set_string (key_file, key_group, name, value);

    g_free (value);
    gdk_color_free (colour);
}

static gboolean
mailtc_settings_keyfile_read_colour (MailtcSettings* settings,
                                     GObject*        obj,
                                     const gchar*    key_group,
                                     const gchar*    name,
                                     GError**        error)
{
    GKeyFile* key_file;
    gchar* value;
    GdkColor colour;

    g_assert (MAILTC_IS_SETTINGS (settings));

    key_file = settings->priv->key_file;

    value = g_key_file_get_string (key_file, key_group, name, error);
    if (error && *error)
        return FALSE;

    mailtc_settings_string_to_colour (value, &colour);
    g_free (value);

    g_object_set (obj, name, &colour, NULL);

    return TRUE;
}

static void
mailtc_settings_keyfile_write_accounts (MailtcSettings* settings)
{
    MailtcAccount* account;
    MailtcModule* module;
    MailtcExtension* extension;
    GKeyFile* key_file;
    GPtrArray* accounts;
    GObject* obj;
    gchar* key_group;
    gchar** groups;
    guint i;
    const gchar* module_name;

    g_assert (MAILTC_IS_SETTINGS (settings));
    g_assert (settings->modules);
    g_assert (settings->accounts);

    key_file = settings->priv->key_file;
    accounts = settings->accounts;

    groups = g_new0 (gchar *, accounts->len);
    for (i = 0; i < accounts->len; i++)
    {
        account = g_ptr_array_index (accounts, i);
        obj = G_OBJECT (account);
        groups[i] = key_group = g_strdup_printf ("account%u", i);

        mailtc_settings_keyfile_write_string (settings, obj, key_group, MAILTC_ACCOUNT_PROPERTY_NAME);
        mailtc_settings_keyfile_write_string (settings, obj, key_group, MAILTC_ACCOUNT_PROPERTY_SERVER);
        mailtc_settings_keyfile_write_uint (settings, obj, key_group, MAILTC_ACCOUNT_PROPERTY_PORT);
        mailtc_settings_keyfile_write_string (settings, obj, key_group, MAILTC_ACCOUNT_PROPERTY_USER);
        mailtc_settings_keyfile_write_uint (settings, obj, key_group, MAILTC_ACCOUNT_PROPERTY_PROTOCOL);
        mailtc_settings_keyfile_write_colour (settings, obj, key_group, MAILTC_ACCOUNT_PROPERTY_ICON_COLOUR);
        mailtc_settings_keyfile_write_base64 (settings, obj, key_group, MAILTC_ACCOUNT_PROPERTY_PASSWORD);

        extension = mailtc_account_get_extension (account);
        module = MAILTC_MODULE (mailtc_extension_get_module (extension));
        module_name = mailtc_module_get_name (module);
        g_key_file_set_string (key_file, key_group, MAILTC_ACCOUNT_PROPERTY_EXTENSION, module_name);
        g_object_unref (module);
        g_object_unref (extension);
        g_free (key_group);
    }
    g_key_file_set_string_list (key_file, MAILTC_SETTINGS_GROUP_GLOBAL,
                                MAILTC_SETTINGS_PROPERTY_ACCOUNTS, (const gchar**) groups, accounts->len);
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
        MailtcAccount* account;
        MailtcModule* module;
        MailtcExtension* extension;
        GPtrArray* accounts;
        GPtrArray* modules;
        GObject* obj;
        gchar* str;
        const gchar* key_group;
        const gchar* module_name;
        gsize i;
        gsize j;
        gboolean success;

        accounts = g_ptr_array_new ();
        modules = settings->modules;

        for (i = 0; i < naccounts; i++)
        {
            success = FALSE;

            key_group = account_groups[i];
            if (!g_key_file_has_group (key_file, key_group))
                continue;

            account = mailtc_account_new ();
            obj = G_OBJECT (account);

            success = mailtc_settings_keyfile_read_string (settings, obj, key_group, MAILTC_ACCOUNT_PROPERTY_NAME, error);
            if (success)
                success = mailtc_settings_keyfile_read_string (settings, obj, key_group, MAILTC_ACCOUNT_PROPERTY_SERVER, error);
            if (success)
                success = mailtc_settings_keyfile_read_uint (settings, obj, key_group, MAILTC_ACCOUNT_PROPERTY_PORT, error);
            if (success)
                success = mailtc_settings_keyfile_read_string (settings, obj, key_group, MAILTC_ACCOUNT_PROPERTY_USER, error);
            if (success)
                success = mailtc_settings_keyfile_read_uint (settings, obj, key_group, MAILTC_ACCOUNT_PROPERTY_PROTOCOL, error);
            if (success)
                success = mailtc_settings_keyfile_read_colour (settings, obj, key_group, MAILTC_ACCOUNT_PROPERTY_ICON_COLOUR, error);
            if (success)
                success = mailtc_settings_keyfile_read_base64 (settings, obj, key_group, MAILTC_ACCOUNT_PROPERTY_PASSWORD, error);
            if (success)
            {
                str = g_key_file_get_string (key_file, key_group, MAILTC_ACCOUNT_PROPERTY_EXTENSION, error);

                for (j = 0; j < modules->len; j++)
                {
                    extension = g_ptr_array_index (modules, j);

                    module = MAILTC_MODULE (mailtc_extension_get_module (extension));
                    module_name = mailtc_module_get_name (module);
                    g_object_unref (module);

                    if (!g_strcmp0 (str, module_name))
                    {
                        mailtc_account_set_extension (account, extension);
                        break;
                    }
                }
                /* FIXME GError */
                g_assert (j < modules->len);
                g_free (str);
            }
            if (success)
                g_ptr_array_add (accounts, account);
            else
                g_object_unref (account);
        }
        if (accounts->len > 0)
            mailtc_settings_set_accounts (settings, accounts);

        g_ptr_array_unref (accounts);
    }
    g_strfreev (account_groups);

    return TRUE;
}

static gboolean
mailtc_settings_keyfile_read (MailtcSettings* settings,
                              GError**        error)
{
    GObject* obj = G_OBJECT (settings);
    const gchar* key_group = MAILTC_SETTINGS_GROUP_GLOBAL;

    if (!mailtc_settings_keyfile_read_uint (settings, obj, key_group, MAILTC_SETTINGS_PROPERTY_INTERVAL, error))
        return FALSE;
    if (!mailtc_settings_keyfile_read_uint (settings, obj, key_group, MAILTC_SETTINGS_PROPERTY_NET_ERROR, error))
        return FALSE;
    if (!mailtc_settings_keyfile_read_string (settings, obj, key_group, MAILTC_SETTINGS_PROPERTY_COMMAND, error))
        return FALSE;
    if (!mailtc_settings_keyfile_read_colour (settings, obj, key_group, MAILTC_SETTINGS_PROPERTY_ICON_COLOUR, error))
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
    GObject* obj = G_OBJECT (settings);
    const gchar* key_group = MAILTC_SETTINGS_GROUP_GLOBAL;

    g_assert (MAILTC_IS_SETTINGS (settings));

    filename = settings->filename;

    if (g_file_test (filename, G_FILE_TEST_EXISTS))
        g_chmod (filename, S_IRUSR | S_IWUSR);

    mailtc_settings_keyfile_write_uint (settings, obj, key_group, MAILTC_SETTINGS_PROPERTY_INTERVAL);
    mailtc_settings_keyfile_write_uint (settings, obj, key_group, MAILTC_SETTINGS_PROPERTY_NET_ERROR);
    mailtc_settings_keyfile_write_string (settings, obj, key_group, MAILTC_SETTINGS_PROPERTY_COMMAND);
    mailtc_settings_keyfile_write_colour (settings, obj, key_group, MAILTC_SETTINGS_PROPERTY_ICON_COLOUR);
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
    MAILTC_SETTINGS_SET_STRING (settings, filename);
}

void
mailtc_settings_set_command (MailtcSettings* settings,
                             const gchar*    command)
{
    MAILTC_SETTINGS_SET_STRING (settings, command);
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
    MAILTC_SETTINGS_SET_UINT (settings, interval);
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
    MAILTC_SETTINGS_SET_UINT (settings, neterror);
}

guint
mailtc_settings_get_neterror (MailtcSettings* settings)
{
    g_assert (MAILTC_IS_SETTINGS (settings));

    return settings->neterror;
}

void
mailtc_settings_set_iconcolour (MailtcSettings* settings,
                                const GdkColor* iconcolour)
{
    MAILTC_SETTINGS_SET_COLOUR (settings, iconcolour);
}

void
mailtc_settings_get_iconcolour (MailtcSettings* settings,
                                GdkColor*      iconcolour)
{
    g_assert (MAILTC_IS_SETTINGS (settings));

    *iconcolour = settings->iconcolour;
}

void
mailtc_settings_set_accounts (MailtcSettings* settings,
                              GPtrArray*      accounts)
{
    MAILTC_SETTINGS_SET_PTR_ARRAY (settings, accounts);
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
    MAILTC_SETTINGS_SET_PTR_ARRAY (settings, modules);
}

GPtrArray*
mailtc_settings_get_modules (MailtcSettings* settings)
{
    g_assert (MAILTC_IS_SETTINGS (settings));

    return settings->modules;
}

static void
mailtc_settings_free_accounts (MailtcSettings* settings)
{
    g_assert (MAILTC_IS_SETTINGS (settings));

    if (settings->accounts)
    {
        g_ptr_array_foreach (settings->accounts, (GFunc) g_object_unref, NULL);
        g_ptr_array_unref (settings->accounts);
        settings->accounts = NULL;
    }
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

    mailtc_settings_free_accounts (settings);

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
                                     "The mail extension modules",
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

