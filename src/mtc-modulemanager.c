/* mtc-modulemanager.c
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

#include "mtc-modulemanager.h"
#include "mtc-util.h"

#include <gio/gio.h>
#include <glib/gstdio.h>

#define MAILTC_MODULE_MANAGER_PROPERTY_DIRECTORY "directory"

#define MAILTC_MODULE_MANAGER_SET_STRING(manager, property) \
    mailtc_object_set_string (G_OBJECT (manager), MAILTC_TYPE_MODULE_MANAGER, \
                              #property, &manager->priv->property, property)

#define MAILTC_MODULE_MANAGER_ERROR g_quark_from_string ("MAILTC_MODULE_MANAGER_ERROR")

typedef enum
{
    MAILTC_MODULE_MANAGER_ERROR_DIRECTORY = 0,
    MAILTC_MODULE_MANAGER_ERROR_EMPTY
} MailtcModuleManagerError;

enum
{
    PROP_0,
    PROP_DIRECTORY
};

struct _MailtcModuleManagerPrivate
{
    GPtrArray* modules;
    gchar* directory;
    GError* error;
};

struct _MailtcModuleManager
{
    GObject parent_instance;

    MailtcModuleManagerPrivate* priv;
};

struct _MailtcModuleManagerClass
{
    GObjectClass parent_class;
};

static gboolean
mailtc_module_manager_initable_init (GInitable*    initable,
                                     GCancellable* cancellable,
                                     GError**      error)
{
    MailtcModuleManager* manager;

    (void) cancellable;

    g_assert (MAILTC_IS_MODULE_MANAGER (initable));

    manager = MAILTC_MODULE_MANAGER (initable);

    if (manager->priv->error && error)
    {
        *error = g_error_copy (manager->priv->error);
        return FALSE;
    }
    return TRUE;
}

static void
mailtc_module_manager_initable_iface_init (GInitableIface* iface)
{
    iface->init = mailtc_module_manager_initable_init;
}

G_DEFINE_TYPE_WITH_CODE (MailtcModuleManager, mailtc_module_manager, G_TYPE_OBJECT,
        G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, mailtc_module_manager_initable_iface_init))

static void
mailtc_module_manager_set_directory (MailtcModuleManager* manager,
                                     const gchar*         directory)
{
    MAILTC_MODULE_MANAGER_SET_STRING (manager, directory);
}

static void
mailtc_module_manager_unload_func (MailtcExtension* extension,
                                   GError**         error)
{
    MailtcModule* module;

    g_assert (extension);

    /* FIXME not sure this is really correct, it wouldn't work if e.g
     * we had more than one extension per module.
     */
    module = MAILTC_MODULE (mailtc_extension_get_module (extension));
    g_object_unref (extension);
    mailtc_module_unload (module, error);

    g_object_unref (module);
}

gboolean
mailtc_module_manager_unload (MailtcModuleManager* manager,
                              GError**             error)
{
    MailtcModuleManagerPrivate* priv;

    g_assert (MAILTC_IS_MODULE_MANAGER (manager));

    priv = manager->priv;

    if (priv->modules)
    {
        g_ptr_array_foreach (priv->modules, (GFunc) mailtc_module_manager_unload_func, error);
        g_ptr_array_unref (priv->modules);
        priv->modules = NULL;
    }
    return ((error && *error) ? FALSE : TRUE);
}

gboolean
mailtc_module_manager_load (MailtcModuleManager* manager,
                            GError**             error)
{
    GDir* dir;
    gchar* dirname = LIBDIR;

    g_assert (MAILTC_IS_MODULE_MANAGER (manager));

    if ((dir = g_dir_open (dirname, 0, error)))
    {
        MailtcModule* module;
        MailtcExtension* extension;
        MailtcExtensionInitFunc extension_init;
        GSList* extensions;
        GSList* l;
        GPtrArray* modules;
        const gchar* filename;
        gchar* directory;
        gchar* fullname;

        modules = g_ptr_array_new ();

        while ((filename = g_dir_read_name (dir)))
        {
            if (!mailtc_module_filename (filename))
                continue;

            fullname = g_build_filename (dirname, filename, NULL);

            module = mailtc_module_new ();
            if (!mailtc_module_load (module, fullname, error) ||
                !mailtc_module_symbol (module, MAILTC_EXTENSION_SYMBOL_INIT, (gpointer*) &extension_init, error))
            {
                mailtc_gerror (error);
            }
            else
            {

                g_type_class_ref (MAILTC_TYPE_EXTENSION);

                g_assert (manager->priv->directory);
                directory = g_build_filename (manager->priv->directory, filename, NULL);

                g_mkdir_with_parents (directory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

                l = extensions = extension_init (directory);
                g_free (directory);

                while (l)
                {
                    extension = l->data;

                    if (mailtc_extension_is_valid (extension, error))
                    {
                        mailtc_extension_set_module (extension, G_OBJECT (module));
                        g_ptr_array_add (modules, extension);
                    }
                    else
                        mailtc_gerror (error);

                    l = l->next;
                }
                g_slist_free (extensions);
            }
            g_object_unref (module);
            g_free (fullname);
        }
        if (modules->len > 0)
            manager->priv->modules = modules;
        else
            g_ptr_array_unref (modules);

        g_dir_close (dir);
    }
    else if (error)
    {
        *error = g_error_new (MAILTC_MODULE_MANAGER_ERROR,
                              MAILTC_MODULE_MANAGER_ERROR_DIRECTORY,
                              "Error opening module directory %s",
                              dirname);
        return FALSE;
    }
    if (error && !manager->priv->modules)
    {
        *error = g_error_new (MAILTC_MODULE_MANAGER_ERROR,
                              MAILTC_MODULE_MANAGER_ERROR_EMPTY,
                              "Error: no modules found!");
        return FALSE;
    }
    return TRUE;
}

static gboolean
mailtc_module_manager_find (MailtcModuleManager* manager,
                            const gchar*         name,
                            MailtcModule**       module,
                            MailtcExtension**    extension)
{
    MailtcModule* mod;
    MailtcExtension* ext;
    GPtrArray* modules;
    const gchar* module_name;
    gsize i;

    g_assert (MAILTC_IS_MODULE_MANAGER (manager));

    /* FIXME all this assumes that there is one extension per module */
    modules = manager->priv->modules;
    if (!modules)
        return FALSE;

    /* FIXME could eventually use a hash table */
    for (i = 0; i < modules->len; i++)
    {
        ext = g_ptr_array_index (modules, i);

        mod = MAILTC_MODULE (mailtc_extension_get_module (ext));
        module_name = mailtc_module_get_name (mod);

        if (!g_strcmp0 (name, module_name))
        {
            if (module)
                *module = mod;
            else
                g_object_unref (mod);

            if (extension)
                *extension = ext;
            return TRUE;
        }
        g_object_unref (mod);
    }

    return FALSE;
}

MailtcExtension*
mailtc_module_manager_find_extension (MailtcModuleManager* manager,
                                      const gchar*         module_name)
{
   MailtcExtension* extension = NULL;

   mailtc_module_manager_find (manager, module_name, NULL, &extension);

   return extension;
}

static MailtcModule*
mailtc_module_manager_find_module (MailtcModuleManager* manager,
                                   const gchar*         module_name)
{
   MailtcModule* module = NULL;

   mailtc_module_manager_find (manager, module_name, &module, NULL);

   return module;
}

void
mailtc_module_manager_foreach_extension (MailtcModuleManager* manager,
                                         GFunc                func,
                                         gpointer             user_data)
{
    MailtcExtension* extension;
    GPtrArray* modules;
    gsize i;

    g_assert (MAILTC_IS_MODULE_MANAGER (manager));

    /* FIXME all this assumes that there is one extension per module */
    modules = manager->priv->modules;
    if (modules && func)
    {
        /* FIXME could eventually use a hash table */
        for (i = 0; i < modules->len; i++)
        {
            extension = g_ptr_array_index (modules, i);

            if (MAILTC_IS_EXTENSION (extension))
                (*func) (extension, user_data);
        }
    }
}

static void
mailtc_module_manager_set_property (GObject*      object,
                                    guint         prop_id,
                                    const GValue* value,
                                    GParamSpec*   pspec)
{
    MailtcModuleManager* manager = MAILTC_MODULE_MANAGER (object);

    switch (prop_id)
    {
        case PROP_DIRECTORY:
            mailtc_module_manager_set_directory (manager, g_value_get_string (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_module_manager_get_property (GObject*      object,
                                    guint         prop_id,
                                    GValue*       value,
                                    GParamSpec*   pspec)
{
    MailtcModuleManager* manager = MAILTC_MODULE_MANAGER (object);

    switch (prop_id)
    {
        case PROP_DIRECTORY:
            g_value_set_string (value, manager->priv->directory);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_module_manager_finalize (GObject* object)
{
    MailtcModuleManager* manager = MAILTC_MODULE_MANAGER (object);

    mailtc_module_manager_unload (manager, NULL); /* FIXME GError */
    g_clear_error (&manager->priv->error);

    g_free (manager->priv->directory);

    G_OBJECT_CLASS (mailtc_module_manager_parent_class)->finalize (object);
}

static void
mailtc_module_manager_constructed (GObject* object)
{
    MailtcModuleManager* manager = MAILTC_MODULE_MANAGER (object);

    mailtc_module_supported (&manager->priv->error);

    G_OBJECT_CLASS (mailtc_module_manager_parent_class)->constructed (object);
}

static void
mailtc_module_manager_class_init (MailtcModuleManagerClass* klass)
{
    GObjectClass* gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->constructed = mailtc_module_manager_constructed;
    gobject_class->finalize = mailtc_module_manager_finalize;
    gobject_class->set_property = mailtc_module_manager_set_property;
    gobject_class->get_property = mailtc_module_manager_get_property;

    g_object_class_install_property (gobject_class,
                                     PROP_DIRECTORY,
                                     g_param_spec_string (
                                     MAILTC_MODULE_MANAGER_PROPERTY_DIRECTORY,
                                     "Directory",
                                     "The parent directory for modules",
                                     NULL,
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT));

    g_type_class_add_private (klass, sizeof (MailtcModuleManagerPrivate));
}

static void
mailtc_module_manager_init (MailtcModuleManager* module_manager)
{
    MailtcModuleManagerPrivate* priv;

    priv = module_manager->priv = G_TYPE_INSTANCE_GET_PRIVATE (module_manager, MAILTC_TYPE_MODULE_MANAGER, MailtcModuleManagerPrivate);
    priv->error = NULL;
    priv->modules = NULL;
    priv->directory = NULL;
}

MailtcModuleManager*
mailtc_module_manager_new (gchar*   directory,
                           GError** error)
{
    return g_initable_new (MAILTC_TYPE_MODULE_MANAGER, NULL, error,
                           MAILTC_MODULE_MANAGER_PROPERTY_DIRECTORY, directory, NULL);
}
