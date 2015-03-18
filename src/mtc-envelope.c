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

#include "mtc-envelope.h"
#include "mtc-util.h"

#define MAILTC_ENVELOPE_SET_COLOUR(envelope,property) \
    mailtc_object_set_colour (G_OBJECT (envelope), MAILTC_TYPE_ENVELOPE, \
                              #property, &envelope->property, property)

struct _MailtcEnvelopePrivate
{
    GdkPixbuf* pixbuf;
    gboolean pixbuf_is_envelope;
};

struct _MailtcEnvelope
{
    GtkImage parent_instance;

    MailtcEnvelopePrivate* priv;
    GdkRGBA colour;
};

struct _MailtcEnvelopeClass
{
    GtkImageClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (MailtcEnvelope, mailtc_envelope, GTK_TYPE_IMAGE, G_ADD_PRIVATE (MailtcEnvelope))

enum
{
    PROP_0,
    PROP_COLOUR
};

static void
mailtc_envelope_set_pixbuf_colour (GdkPixbuf* pixbuf,
                                   GdkRGBA*   colour)
{
    gint width;
    gint height;
    gint rowstride;
    gint n_channels;
    guchar* pixels;
    guchar* p;
    gint i;
    gint j;
    guint8 r;
    guint8 g;
    guint8 b;

    if (colour)
    {
        r = (guint8) (colour->red * 255);
        g = (guint8) (colour->green * 255);
        b = (guint8) (colour->blue * 255);
    }
    else
        r = g = b = 255;

    n_channels = gdk_pixbuf_get_n_channels (pixbuf);
    g_assert (gdk_pixbuf_get_colorspace (pixbuf) == GDK_COLORSPACE_RGB);
    g_assert (gdk_pixbuf_get_bits_per_sample (pixbuf) == 8);
    g_assert (gdk_pixbuf_get_has_alpha (pixbuf));
    g_assert (n_channels == 4);

    width = gdk_pixbuf_get_width (pixbuf);
    height = gdk_pixbuf_get_height (pixbuf);
    rowstride = gdk_pixbuf_get_rowstride (pixbuf);
    pixels = gdk_pixbuf_get_pixels (pixbuf);

    for (i = 0; i < width; i++)
    {
        for (j = 0; j < height; j++)
        {
            p = pixels + i * rowstride + j * n_channels;
            if (p[0] == 0xFF && p[1] == 0xFF && p[2] == 0xFF && p[3] == 0xFF)
            {
                p[0] = r;
                p[1] = g;
                p[2] = b;
                p[3] = 0xFF;
            }
        }
    }
}

static GdkPixbuf*
mailtc_envelope_create_pixbuf (MailtcEnvelope* envelope)
{
    GInputStream *stream;
    GdkPixbuf* pixbuf;

    g_assert (MAILTC_IS_ENVELOPE (envelope));

    stream = g_resources_open_stream ("/org/mailtc/icon/envelope.png", 0, NULL);
    pixbuf = gdk_pixbuf_new_from_stream (stream, NULL, NULL);
    g_object_unref (stream);

    return pixbuf;
}

static void
mailtc_envelope_notify_pixbuf_cb (GObject*    object,
                                  GParamSpec* pspec)
{
    MailtcEnvelope* envelope;
    GdkPixbuf* pixbuf;
    gboolean is_envelope = FALSE;

    (void) pspec;
    envelope = MAILTC_ENVELOPE (object);

    pixbuf = mailtc_envelope_get_pixbuf (envelope);
    if (pixbuf)
        is_envelope = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (pixbuf), "mailtc_envelope"));

    envelope->priv->pixbuf_is_envelope = is_envelope;
}

static void
mailtc_envelope_notify_colour_cb (GObject*    object,
                                  GParamSpec* pspec)
{
    MailtcEnvelopePrivate* priv;
    MailtcEnvelope* envelope;
    GdkPixbuf* pixbuf;

    (void) pspec;
    envelope = MAILTC_ENVELOPE (object);

    g_assert (MAILTC_IS_ENVELOPE (envelope));

    priv = envelope->priv;

    pixbuf = mailtc_envelope_create_pixbuf (envelope);
    g_assert (pixbuf);

    mailtc_envelope_set_pixbuf_colour (pixbuf, &envelope->colour);

    g_object_set_data (G_OBJECT (pixbuf),
                       "mailtc_envelope", GUINT_TO_POINTER (1));
    gtk_image_set_from_pixbuf (GTK_IMAGE (envelope), pixbuf);

    if (priv->pixbuf)
        g_object_unref (priv->pixbuf);

    priv->pixbuf = pixbuf;
}

GdkPixbuf*
mailtc_envelope_get_pixbuf (MailtcEnvelope* envelope)
{
    GdkPixbuf* pixbuf = NULL;

    g_assert (MAILTC_IS_ENVELOPE (envelope));

    if (gtk_image_get_storage_type (GTK_IMAGE (envelope)) == GTK_IMAGE_PIXBUF)
        pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (envelope));

    return (pixbuf);
}

void
mailtc_envelope_set_colour (MailtcEnvelope* envelope,
                            const GdkRGBA*  colour)
{
    MAILTC_ENVELOPE_SET_COLOUR (envelope, colour);
}

void
mailtc_envelope_get_colour (MailtcEnvelope* envelope,
                            GdkRGBA*        colour)
{
    g_assert (MAILTC_IS_ENVELOPE (envelope));

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
    MailtcEnvelope* envelope = MAILTC_ENVELOPE (object);
    GdkRGBA colour;

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
mailtc_envelope_class_init (MailtcEnvelopeClass* klass)
{
    GObjectClass* gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = mailtc_envelope_finalize;
    gobject_class->set_property = mailtc_envelope_set_property;
    gobject_class->get_property = mailtc_envelope_get_property;

    g_object_class_install_property (gobject_class,
                                     PROP_COLOUR,
                                     g_param_spec_boxed (
                                     "colour",
                                     "Colour",
                                     "The envelope colour",
                                     GDK_TYPE_RGBA,
                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
}

static void
mailtc_envelope_init (MailtcEnvelope* envelope)
{
    envelope->priv = G_TYPE_INSTANCE_GET_PRIVATE (envelope,
                     MAILTC_TYPE_ENVELOPE, MailtcEnvelopePrivate);

    envelope->priv->pixbuf = NULL;
    envelope->priv->pixbuf_is_envelope = FALSE;

    g_signal_connect (envelope, "notify::pixbuf",
            G_CALLBACK (mailtc_envelope_notify_pixbuf_cb), NULL);
    g_signal_connect (envelope, "notify::colour",
            G_CALLBACK (mailtc_envelope_notify_colour_cb), NULL);
}

GtkWidget*
mailtc_envelope_new (void)
{
    return g_object_new (MAILTC_TYPE_ENVELOPE, NULL);
}

