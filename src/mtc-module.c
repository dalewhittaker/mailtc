/* mtc-module.c
 * Copyright (C) 2009-2015 Dale Whittaker <dayul@users.sf.net>
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

#include "mtc-module.h"
#include "mtc-util.h"
#include <gmodule.h>

#define MAILTC_MODULE_SET_STRING(module,property) \
    mailtc_object_set_string (G_OBJECT (module), MAILTC_TYPE_MODULE, \
                              #property, &module->property, property)

#define MAILTC_MODULE_ERROR g_quark_from_string ("MAILTC_MODULE_ERROR")

typedef enum
{
    MAILTC_MODULE_ERROR_SUPPORTED = 0,
    MAILTC_MODULE_ERROR_OPEN,
    MAILTC_MODULE_ERROR_SYMBOL,
    MAILTC_MODULE_ERROR_CLOSE

} MailtcModuleError;

typedef struct
{
    GModule* module;
} MailtcModulePrivate;

struct _MailtcModule
{
    GObject parent_instance;

    MailtcModulePrivate* priv;
    gchar* name;
};

G_DEFINE_TYPE_WITH_PRIVATE (MailtcModule, mailtc_module, G_TYPE_OBJECT)

enum
{
    PROP_0,
    PROP_NAME
};

static void
mailtc_module_set_name (MailtcModule* module,
                        const gchar*  name)
{
    MAILTC_MODULE_SET_STRING (module, name);
}

const gchar*
mailtc_module_get_name (MailtcModule* module)
{
    g_assert (MAILTC_IS_MODULE (module));

    return module->name;
}

gboolean
mailtc_module_unload (MailtcModule* module,
                      GError**      error)
{
    MailtcModulePrivate* priv;
    gboolean retval = TRUE;

    g_assert (MAILTC_IS_MODULE (module));

    priv = module->priv;

    if (priv->module)
    {
        retval = g_module_close (priv->module);

        if (!retval && error && !*error)
        {
            *error = g_error_new (MAILTC_MODULE_ERROR,
                                  MAILTC_MODULE_ERROR_CLOSE,
                                  "Error unloading module: %s: %s",
                                  g_module_name (priv->module),
                                  g_module_error ());
        }

        priv->module = NULL;
    }
    return retval;
}

gboolean
mailtc_module_load (MailtcModule* module,
                    gchar*        filename,
                    GError**      error)
{
    GModule* pmodule;

    g_assert (MAILTC_IS_MODULE (module));

    pmodule = g_module_open (filename, G_MODULE_BIND_LOCAL);
    if (!pmodule)
    {
        if (error && !*error)
        {
            *error = g_error_new (MAILTC_MODULE_ERROR,
                                  MAILTC_MODULE_ERROR_OPEN,
                                  "Error loading module: %s: %s",
                                  filename,
                                  g_module_error ());
        }
        return FALSE;
    }

    module->priv->module = pmodule;

    filename = g_path_get_basename (filename);
    mailtc_module_set_name (module, filename);
    g_free (filename);

    return TRUE;
}

gboolean
mailtc_module_supported (GError** error)
{
    if (!g_module_supported ())
    {
        if (error && !*error)
        {
            *error = g_error_new (MAILTC_MODULE_ERROR,
                                  MAILTC_MODULE_ERROR_SUPPORTED,
                                  "Error checking module support: %s",
                                  g_module_error ());
        }
        return FALSE;
    }
    return TRUE;
}

gboolean
mailtc_module_filename (const gchar* filename)
{
    return g_str_has_suffix (filename, G_MODULE_SUFFIX);
}

gboolean
mailtc_module_symbol (MailtcModule* module,
                      const gchar*  symbol_name,
                      gpointer*     symbol,
                      GError**      error)
{
    g_assert (MAILTC_IS_MODULE (module));

    if (!g_module_symbol (module->priv->module, symbol_name, symbol))
    {
        if (error && !*error)
        {
            *error = g_error_new (MAILTC_MODULE_ERROR,
                                  MAILTC_MODULE_ERROR_SYMBOL,
                                  "Error getting pointer to symbol: %s",
                                  g_module_error ());
        }
        return FALSE;
    }
    return TRUE;
}

static void
mailtc_module_set_property (GObject*      object,
                            guint         prop_id,
                            const GValue* value,
                            GParamSpec*   pspec)
{
    MailtcModule* module = MAILTC_MODULE (object);

    switch (prop_id)
    {
        case PROP_NAME:
            mailtc_module_set_name (module, g_value_get_string (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_module_get_property (GObject*    object,
                            guint       prop_id,
                            GValue*     value,
                            GParamSpec* pspec)
{
    MailtcModule* module = MAILTC_MODULE (object);

    switch (prop_id)
    {
        case PROP_NAME:
            g_value_set_string (value, module->name);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_module_finalize (GObject* object)
{
    MailtcModule* module = MAILTC_MODULE (object);
    GError* error = NULL;

    g_free (module->name);
    if (!mailtc_module_unload (module, &error))
        mailtc_gerror (&error);

    G_OBJECT_CLASS (mailtc_module_parent_class)->finalize (object);
}

static void
mailtc_module_class_init (MailtcModuleClass* klass)
{
    GObjectClass* gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = mailtc_module_finalize;
    gobject_class->set_property = mailtc_module_set_property;
    gobject_class->get_property = mailtc_module_get_property;

    g_object_class_install_property (gobject_class,
                                     PROP_NAME,
                                     g_param_spec_string (
                                     "name",
                                     "Name",
                                     "The module name",
                                     NULL,
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT));
}

static void
mailtc_module_init (MailtcModule* module)
{
    module->priv = G_TYPE_INSTANCE_GET_PRIVATE (module,
                     MAILTC_TYPE_MODULE, MailtcModulePrivate);

    module->priv->module = NULL;
}

MailtcModule*
mailtc_module_new (void)
{
    return g_object_new (MAILTC_TYPE_MODULE, NULL);
}

