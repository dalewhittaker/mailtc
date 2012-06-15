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

#include <glib/gstdio.h>
#include <gdk/gdk.h>

#define MAILTC_SETTINGS_GROUP_GLOBAL      "settings"
#define MAILTC_SETTINGS_PROPERTY_FILENAME "filename"

struct _MailtcSettingsPrivate
{
    GHashTable* hash_table;
    GKeyFile* key_file;
    GError* error;
};

struct _MailtcSettings
{
    GObject parent_instance;

    MailtcSettingsPrivate* priv;
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
    PROP_FILENAME,
    PROP_COMMAND,
    PROP_INTERVAL,
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
mailtc_settings_set_colour (MailtcSettings* settings,
                            const GdkColor* colour)
{
    GdkColor* iconcolour;

    g_assert (MAILTC_IS_SETTINGS (settings));

    iconcolour = &settings->iconcolour;

    if (colour)
    {
        iconcolour->red = colour->red;
        iconcolour->green = colour->green;
        iconcolour->blue = colour->blue;
    }
    else
        iconcolour->red = iconcolour->green = iconcolour->blue = 0xFFFF;
}

static void
mailtc_settings_set_property (GObject*      object,
                              guint         prop_id,
                              const GValue* value,
                              GParamSpec*   pspec)
{
    MailtcSettings* settings = MAILTC_SETTINGS (object);

    /* FIXME any reason we aren't freeing these strings? */
    /* FIXME can also check if they are equal, if they are
     * no point in freeing...
     */
    switch (prop_id)
    {
        case PROP_COMMAND:
            settings->command = g_value_dup_string (value);
            break;

        case PROP_FILENAME:
            settings->filename = g_value_dup_string (value);
            break;

        case PROP_INTERVAL:
            settings->interval = g_value_get_uint (value);
            break;

        case PROP_NET_ERROR:
            settings->neterror = g_value_get_uint (value);
            break;

        case PROP_ICON_COLOUR:
            mailtc_settings_set_colour (settings, g_value_get_boxed (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static gpointer
mailtc_settings_hash_table_lookup (MailtcSettings* settings,
                                   const gchar*    name)
{
    MailtcSettingsPrivate* priv;

    g_assert (MAILTC_IS_SETTINGS (settings));

    priv = settings->priv;
    g_assert (priv->hash_table);

    return  g_hash_table_lookup (priv->hash_table, name);
}

static void
mailtc_settings_hash_table_insert (MailtcSettings* settings,
                                   const gchar*    key,
                                   GValue*         value)
{
    MailtcSettingsPrivate* priv;
    gchar* hkey;
    GValue* hvalue;

    g_assert (MAILTC_IS_SETTINGS (settings));

    priv = settings->priv;
    g_assert (priv->hash_table);

    hkey = g_strdup (key);
    hvalue = g_new0 (GValue, 1);

    g_value_copy (value, g_value_init (hvalue, G_VALUE_TYPE (value)));
    g_hash_table_insert (priv->hash_table, hkey, hvalue);
}

static gboolean
mailtc_settings_set_from_hash_table (MailtcSettings* settings,
                                     const gchar*    name,
                                     GValue*         value)
{
    GValue* hvalue;

    hvalue = mailtc_settings_hash_table_lookup (settings, name);
    if (!hvalue)
        return FALSE;

    g_assert (g_value_type_compatible (G_VALUE_TYPE (hvalue), G_VALUE_TYPE (value)));
    g_value_copy (hvalue, value);

    return TRUE;
}

static gboolean
mailtc_settings_uint_from_keyfile (MailtcSettings* settings,
                                   const gchar*    group,
                                   const gchar*    name,
                                   guint*          v)
{
    GKeyFile* key_file;
    GError* error = NULL;

    g_assert (MAILTC_IS_SETTINGS (settings));
    g_assert (v);

    key_file = settings->priv->key_file;
    g_assert (key_file);

    *v = (guint) g_key_file_get_integer (key_file, group, name, &error);
    if (error)
    {
        g_clear_error (&error);
        return FALSE;
    }
    return TRUE;
}

static gboolean
mailtc_settings_string_from_keyfile (MailtcSettings* settings,
                                     const gchar*    group,
                                     const gchar*    name,
                                     gchar**         v)
{
    GKeyFile* key_file;
    GError* error = NULL;

    g_assert (MAILTC_IS_SETTINGS (settings));
    g_assert (v);

    key_file = settings->priv->key_file;
    g_assert (key_file);

    *v = g_key_file_get_string (key_file, group, name, &error);
    if (error)
    {
        g_clear_error (&error);
        return FALSE;
    }
    return TRUE;
}

/* FIXME we need to test for memory leaks, and also
 * ensure the value retrieved by external classes doesn't need
 * to be copied.
 */
static void
mailtc_settings_set_uint (MailtcSettings* settings,
                          const gchar*    name,
                          GValue*         value,
                          guint           vdefault)
{
    if (!mailtc_settings_set_from_hash_table (settings, name, value))
    {
        guint v;

        if (!mailtc_settings_uint_from_keyfile (settings, MAILTC_SETTINGS_GROUP_GLOBAL, name, &v))
            v = vdefault;

        g_value_set_uint (value, v);
        mailtc_settings_hash_table_insert (settings, name, value);
    }
}

static void
mailtc_settings_set_string (MailtcSettings* settings,
                            const gchar*    name,
                            GValue*         value,
                            gchar*          vdefault)
{
    if (!mailtc_settings_set_from_hash_table (settings, name, value))
    {
        gchar* v;

        if (!mailtc_settings_string_from_keyfile (settings, MAILTC_SETTINGS_GROUP_GLOBAL, name, &v))
            v = vdefault;

        g_value_set_string (value, v);
        mailtc_settings_hash_table_insert (settings, name, value);
    }
}

static void
mailtc_settings_set_gdkcolor (MailtcSettings* settings,
                              const gchar*    name,
                              GValue*         value,
                              GdkColor*       vdefault)
{
    if (!mailtc_settings_set_from_hash_table (settings, name, value))
    {
        GdkColor colour;
        GdkColor* v;
        gchar* s;

        if (mailtc_settings_string_from_keyfile (settings, MAILTC_SETTINGS_GROUP_GLOBAL, name, &s))
        {
            guint64 colourval;

            v = &colour;

            colourval = g_ascii_strtoull (s, NULL, 16);
            v->red = (guint16) ((colourval >> 32) & 0xFFFF);
            v->green = (guint16) ((colourval >> 16) & 0xFFFF);
            v->blue = (guint16) (colourval & 0xFFFF);
        }
        else
            v = vdefault;

        g_value_set_boxed (value, v);
        mailtc_settings_hash_table_insert (settings, name, value);
    }
}

static void
mailtc_settings_get_property (GObject*    object,
                              guint       prop_id,
                              GValue*     value,
                              GParamSpec* pspec)
{
    MailtcSettings* settings;
    const gchar* name;

    settings = MAILTC_SETTINGS (object);
    name = g_param_spec_get_name (pspec);

    switch (prop_id)
    {
        case PROP_FILENAME:
            g_value_set_string (value, settings->filename);
            break;

        case PROP_COMMAND:
            mailtc_settings_set_string (settings, name, value, settings->command);
            break;

        case PROP_INTERVAL:
            mailtc_settings_set_uint (settings, name, value, settings->interval);
            break;

        case PROP_NET_ERROR:
            mailtc_settings_set_uint (settings, name, value, settings->neterror);
            break;

        case PROP_ICON_COLOUR:
            mailtc_settings_set_gdkcolor (settings, name, value, &settings->iconcolour);
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

    g_free (settings->command);
    g_free (settings->filename);
    settings->command = NULL;
    settings->filename = NULL;

    g_clear_error (&priv->error);
    priv->error = NULL;

    if (priv->key_file)
    {
        g_key_file_free (priv->key_file);
        priv->key_file = NULL;

    }
    if (priv->hash_table)
    {
        g_hash_table_destroy (priv->hash_table);
        priv->hash_table = NULL;
    }

    G_OBJECT_CLASS (mailtc_settings_parent_class)->finalize (object);
}

static void
mailtc_settings_free_property (GValue* property)
{
    g_value_unset (property);
    g_free (property);
}

static void
mailtc_settings_constructed (GObject* object)
{
    MailtcSettings* settings;
    MailtcSettingsPrivate* priv;

    settings = MAILTC_SETTINGS (object);
    priv = settings->priv;

    priv->hash_table = g_hash_table_new_full (g_str_hash,
                                              g_str_equal,
                                              g_free,
                                              (GDestroyNotify) mailtc_settings_free_property);

    priv->key_file = g_key_file_new ();

    g_key_file_load_from_file (priv->key_file, settings->filename, G_KEY_FILE_NONE, &priv->error);

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
                                     MAILTC_SETTINGS_PROPERTY_FILENAME
                                     "Filename",
                                     "The settings filename",
                                     NULL,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_INTERVAL,
                                     g_param_spec_uint (
                                     "interval",
                                     "Interval",
                                     "The mail checking interval in minutes",
                                     1,
                                     60,
                                     3,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_NET_ERROR,
                                     g_param_spec_uint (
                                     "neterror",
                                     "Neterror",
                                     "The number of network errors before reporting",
                                     0,
                                     5,
                                     1,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_ICON_COLOUR,
                                     g_param_spec_boxed (
                                     "iconcolour",
                                     "Iconcolour",
                                     "The icon colour",
                                     GDK_TYPE_COLOR,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_COMMAND,
                                     g_param_spec_string (
                                     "command",
                                     "Command",
                                     "The mail command to execute",
                                     NULL,
                                     flags));

    g_type_class_add_private (class, sizeof (MailtcSettingsPrivate));
}

static void
mailtc_settings_init (MailtcSettings* settings)
{
    MailtcSettingsPrivate* priv;

    priv = settings->priv = G_TYPE_INSTANCE_GET_PRIVATE (settings,
                        MAILTC_TYPE_SETTINGS, MailtcSettingsPrivate);

    priv->hash_table = NULL;
    priv->key_file = NULL;
    priv->error = NULL;
}

MailtcSettings*
mailtc_settings_new (gchar*   filename,
                     GError** error)
{
    return g_initable_new (MAILTC_TYPE_SETTINGS, NULL, error,
                           MAILTC_SETTINGS_PROPERTY_FILENAME, filename, NULL);
}

