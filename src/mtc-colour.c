/* mtc-colour.c
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

#include <gdk/gdk.h>

#include "mtc-colour.h"

G_DEFINE_BOXED_TYPE (MailtcColour, mailtc_colour, mailtc_colour_copy, mailtc_colour_free)

MailtcColour*
mailtc_colour_copy (const MailtcColour* colour)
{
    return g_slice_dup (MailtcColour, colour);
}

void
mailtc_colour_free (MailtcColour* colour)
{
    g_slice_free (MailtcColour, colour);
}

gboolean
mailtc_colour_parse (MailtcColour* colour,
                     const gchar*  str)
{
    GdkRGBA rgb;

    g_assert (colour);
    g_assert (str);

    if (!gdk_rgba_parse (&rgb, str))
        return FALSE;

    colour->red = rgb.red;
    colour->green = rgb.green;
    colour->blue = rgb.blue;

    return TRUE;
}

gchar*
mailtc_colour_to_string (const MailtcColour* colour)
{
    GdkRGBA rgb;

    g_assert (colour);

    rgb.red = colour->red;
    rgb.green = colour->green;
    rgb.blue = colour->blue;
    rgb.alpha = 1.0;

    return gdk_rgba_to_string (&rgb);
}

gboolean
mailtc_colour_equal (const MailtcColour* a,
                     const MailtcColour* b)
{
    return (a->red == b->red && a->green == b->green && a->blue == b->blue) ? TRUE : FALSE;
}

