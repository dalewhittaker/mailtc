/* mtc-checker.c
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

#include "mtc-checker.h"
#include "mtc-util.h"

#define MAILTC_CHECKER_SET_UINT(checker,property) \
    mailtc_object_set_uint (G_OBJECT (checker), MAILTC_TYPE_CHECKER, \
                            #property, &checker->property, property)

#define MAILTC_CHECKER_SET_OBJECT(checker,property) \
    mailtc_object_set_object (G_OBJECT (checker), MAILTC_TYPE_CHECKER, \
                              #property, (GObject **) (&checker->property), G_OBJECT (property))

enum
{
    PROP_0,
    PROP_TIMEOUT,
    PROP_STATUS_ICON
};

enum
{
    SIGNAL_CHECK,
    LAST_SIGNAL
};

struct _MailtcCheckerPrivate
{
    guint idle_id;
    guint timeout_id;
    gboolean is_running;
    gboolean locked;
};

struct _MailtcChecker
{
    GObject parent_instance;

    MailtcCheckerPrivate* priv;
    guint timeout;
    MailtcStatusIcon* statusicon;
};

struct _MailtcCheckerClass
{
    GObjectClass parent_class;

    void (*check) (MailtcChecker* app);
};

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE_WITH_CODE (MailtcChecker, mailtc_checker, G_TYPE_OBJECT, G_ADD_PRIVATE (MailtcChecker))

static gboolean
mailtc_checker_timeout_func (MailtcChecker* checker)
{
    MailtcCheckerPrivate* priv;

    g_assert (MAILTC_IS_CHECKER (checker));

    priv = checker->priv;

    if (!priv->locked)
    {
        priv->locked = TRUE;
        g_signal_emit (checker, signals[SIGNAL_CHECK], 0);
        priv->locked = FALSE;
    }
    return TRUE;
}

static void
mailtc_checker_timeout_destroy (MailtcChecker* checker)
{
    g_assert (MAILTC_IS_CHECKER (checker));

    checker->priv->timeout_id = 0;
}

static gboolean
mailtc_checker_idle_func (MailtcChecker* checker)
{
    g_assert (MAILTC_IS_CHECKER (checker));

    g_signal_emit (checker, signals[SIGNAL_CHECK], 0);
    return FALSE;
}

static void
mailtc_checker_idle_destroy (MailtcChecker* checker)
{
    g_assert (MAILTC_IS_CHECKER (checker));

    checker->priv->idle_id = 0;
}

void
mailtc_checker_run (MailtcChecker* checker)
{
    MailtcCheckerPrivate* priv;

    g_assert (MAILTC_IS_CHECKER (checker));

    priv = checker->priv;

    if (!priv->is_running)
    {
        priv->is_running = TRUE;
        priv->idle_id = g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                                         (GSourceFunc) mailtc_checker_idle_func, checker,
                                         (GDestroyNotify) mailtc_checker_idle_destroy);

        priv->timeout_id = g_timeout_add_seconds_full (G_PRIORITY_DEFAULT, 60 * checker->timeout,
                                                           (GSourceFunc) mailtc_checker_timeout_func, checker,
                                                           (GDestroyNotify) mailtc_checker_timeout_destroy);
    }
}

static void
mailtc_checker_notify_timeout_cb (GObject*    object,
                                  GParamSpec* pspec)
{
    MailtcChecker* checker;
    MailtcCheckerPrivate* priv;

    checker = MAILTC_CHECKER (object);
    priv = checker->priv;

    (void) pspec;

    g_assert (MAILTC_IS_CHECKER (checker));

    if (priv->timeout_id > 0)
        g_source_remove (priv->timeout_id);

    if (priv->is_running && checker->timeout > 0)
    {
        priv->timeout_id = g_timeout_add_seconds_full (G_PRIORITY_DEFAULT, 60 * checker->timeout,
                                                       (GSourceFunc) mailtc_checker_timeout_func, checker,
                                                       (GDestroyNotify) mailtc_checker_timeout_destroy);
    }
    else
        priv->timeout_id = 0;
}

static void
mailtc_checker_set_timeout (MailtcChecker* checker,
                            guint          timeout)
{
    MAILTC_CHECKER_SET_UINT (checker, timeout);
}

void
mailtc_checker_set_status_icon (MailtcChecker*    checker,
                                MailtcStatusIcon* statusicon)
{
    MAILTC_CHECKER_SET_OBJECT (checker, statusicon);
}

MailtcStatusIcon*
mailtc_checker_get_status_icon (MailtcChecker* checker)
{
    g_assert (MAILTC_IS_CHECKER (checker));

    return checker->statusicon ? g_object_ref (checker->statusicon) : NULL;
}

static void
mailtc_checker_set_property (GObject*      object,
                             guint         prop_id,
                             const GValue* value,
                             GParamSpec*   pspec)
{
    MailtcChecker* checker = MAILTC_CHECKER (object);

    switch (prop_id)
    {
        case PROP_TIMEOUT:
            mailtc_checker_set_timeout (checker, g_value_get_uint (value));
            break;
        case PROP_STATUS_ICON:
            mailtc_checker_set_status_icon (checker, g_value_get_object (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_checker_get_property (GObject*      object,
                             guint         prop_id,
                             GValue*       value,
                             GParamSpec*   pspec)
{
    MailtcChecker* checker = MAILTC_CHECKER (object);

    switch (prop_id)
    {
        case PROP_TIMEOUT:
            g_value_set_uint (value, checker->timeout);
            break;
        case PROP_STATUS_ICON:
            g_value_set_object (value, checker->statusicon);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_checker_finalize (GObject* object)
{
    MailtcChecker* checker;
    MailtcCheckerPrivate* priv;

    checker = MAILTC_CHECKER (object);
    priv = checker->priv;

    if (checker->statusicon)
        g_object_unref (checker->statusicon);

    if (priv->timeout_id > 0)
    {
        g_source_remove (priv->timeout_id);
        priv->timeout_id = 0;
    }
    if (priv->idle_id > 0)
    {
        g_source_remove (priv->idle_id);
        priv->idle_id = 0;
    }

    G_OBJECT_CLASS (mailtc_checker_parent_class)->finalize (object);
}

static void
mailtc_checker_class_init (MailtcCheckerClass* klass)
{
    GObjectClass* gobject_class;
    GParamFlags flags;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->set_property = mailtc_checker_set_property;
    gobject_class->get_property = mailtc_checker_get_property;
    gobject_class->finalize = mailtc_checker_finalize;

    flags = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT;

    g_object_class_install_property (gobject_class,
                                     PROP_TIMEOUT,
                                     g_param_spec_uint (
                                     "timeout",
                                     "Timeout",
                                     "The checking timeout",
                                     0,
                                     G_MAXUINT,
                                     0,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_STATUS_ICON,
                                     g_param_spec_object (
                                     "statusicon",
                                     "Statusicon",
                                     "The status icon",
                                     MAILTC_TYPE_STATUS_ICON,
                                     flags));


    signals[SIGNAL_CHECK] = g_signal_new ("check",
                                          G_TYPE_FROM_CLASS (gobject_class),
                                          G_SIGNAL_RUN_LAST,
                                          G_STRUCT_OFFSET (MailtcCheckerClass, check),
                                          NULL,
                                          NULL,
                                          g_cclosure_marshal_VOID__VOID,
                                          G_TYPE_NONE,
                                          0);
}

static void
mailtc_checker_init (MailtcChecker* checker)
{
    MailtcCheckerPrivate* priv;

    priv = checker->priv = G_TYPE_INSTANCE_GET_PRIVATE (checker, MAILTC_TYPE_CHECKER, MailtcCheckerPrivate);

    priv->idle_id = 0;
    priv->timeout_id = 0;
    priv->is_running = FALSE;
    priv->locked = FALSE;

    g_signal_connect (checker, "notify::timeout",
            G_CALLBACK (mailtc_checker_notify_timeout_cb), NULL);
}

MailtcChecker*
mailtc_checker_new (guint timeout)
{
    return g_object_new (MAILTC_TYPE_CHECKER, "timeout", timeout, NULL);
}
