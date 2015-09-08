/* mtc-envelope.c
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

#include "mtc-colour.h"
#include "mtc-envelope.h"
#include "mtc-pixbuf.h"
#include "mtc-util.h"

#define MAILTC_ENVELOPE_SET_COLOUR(envelope,property) \
    mailtc_object_set_colour (G_OBJECT (envelope), MAILTC_TYPE_ENVELOPE, \
                              #property, &envelope->property, property)

struct _MailtcEnvelopePrivate
{
    MailtcPixbuf* pixbuf;
};

struct _MailtcEnvelope
{
    GtkImage parent_instance;

    MailtcEnvelopePrivate* priv;
    MailtcColour colour;
};

G_DEFINE_TYPE_WITH_CODE (MailtcEnvelope, mailtc_envelope, GTK_TYPE_IMAGE, G_ADD_PRIVATE (MailtcEnvelope))

enum
{
    PROP_0,
    PROP_COLOUR
};

static void
mailtc_envelope_notify_colour_cb (GObject*    object,
                                  GParamSpec* pspec)
{
    MailtcEnvelope* envelope;
    MailtcPixbuf* pixbuf;

    (void) pspec;
    g_assert (MAILTC_IS_ENVELOPE (object));

    envelope = MAILTC_ENVELOPE (object);
    pixbuf = envelope->priv->pixbuf;

    mailtc_pixbuf_set_colour (pixbuf, &envelope->colour);
    gtk_image_set_from_gicon (GTK_IMAGE (envelope), mailtc_pixbuf_get_icon (pixbuf), GTK_ICON_SIZE_LARGE_TOOLBAR);
}

void
mailtc_envelope_set_colour (MailtcEnvelope*     envelope,
                            const MailtcColour* colour)
{
    MAILTC_ENVELOPE_SET_COLOUR (envelope, colour);
}

void
mailtc_envelope_get_colour (MailtcEnvelope* envelope,
                            MailtcColour*   colour)
{
    g_assert (MAILTC_IS_ENVELOPE (envelope));
    g_assert (colour);

    *colour = envelope->colour;
}

static void
mailtc_envelope_set_property (GObject*      object,
                              guint         prop_id,
                              const GValue* value,
                              GParamSpec*   pspec)
{
    MailtcEnvelope* envelope = MAILTC_ENVELOPE (object);

    switch (prop_id)
    {
        case PROP_COLOUR:
            mailtc_envelope_set_colour (envelope, g_value_get_boxed (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_envelope_get_property (GObject*    object,
                              guint       prop_id,
                              GValue*     value,
                              GParamSpec* pspec)
{
    MailtcColour colour;
    MailtcEnvelope* envelope = MAILTC_ENVELOPE (object);

    switch (prop_id)
    {
        case PROP_COLOUR:
            mailtc_envelope_get_colour (envelope, &colour);
            g_value_set_boxed (value, &colour);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_envelope_finalize (GObject* object)
{
    MailtcEnvelope* envelope = MAILTC_ENVELOPE (object);

    g_object_unref (envelope->priv->pixbuf);

    G_OBJECT_CLASS (mailtc_envelope_parent_class)->finalize (object);
}

static void
mailtc_envelope_constructed (GObject* object)
{
    MailtcEnvelope* envelope = MAILTC_ENVELOPE (object);

    envelope->priv->pixbuf = mailtc_pixbuf_new ();

    G_OBJECT_CLASS (mailtc_envelope_parent_class)->constructed (object);
}

static void
mailtc_envelope_class_init (MailtcEnvelopeClass* klass)
{
    GObjectClass* gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->constructed = mailtc_envelope_constructed;
    gobject_class->finalize = mailtc_envelope_finalize;
    gobject_class->set_property = mailtc_envelope_set_property;
    gobject_class->get_property = mailtc_envelope_get_property;

    g_object_class_install_property (gobject_class,
                                     PROP_COLOUR,
                                     g_param_spec_boxed (
                                     "colour",
                                     "Colour",
                                     "The envelope colour",
                                     MAILTC_TYPE_COLOUR,
                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
}

static void
mailtc_envelope_init (MailtcEnvelope* envelope)
{
    envelope->priv = G_TYPE_INSTANCE_GET_PRIVATE (envelope, MAILTC_TYPE_ENVELOPE, MailtcEnvelopePrivate);
    envelope->priv->pixbuf = NULL;

    g_signal_connect (envelope, "notify::colour", G_CALLBACK (mailtc_envelope_notify_colour_cb), NULL);
}

MailtcEnvelope*
mailtc_envelope_new (void)
{
    return g_object_new (MAILTC_TYPE_ENVELOPE, NULL);
}

