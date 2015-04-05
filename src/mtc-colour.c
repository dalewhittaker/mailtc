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

#include <string.h>

#include "mtc-colour.h"

#define MAILTC_COLOUR_VALUE(c) ((gdouble) CLAMP ((float) (c) / 255.0, 0.0, 1.0))
#define MAILTC_HEX_COLOUR_VALUE(c) ((int) (0.5 + CLAMP ((c), 0.0, 1.0) * 255.0))

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

static gboolean
mailtc_colour_parse_value (const gchar* i,
                           gdouble*     o)
{
    gint c;

    g_assert (i);
    g_assert (o);
    g_assert (strlen (i) >= 2);

    g_return_val_if_fail (g_ascii_isxdigit (*i) && g_ascii_isxdigit (*(i + 1)), FALSE);

    c = g_ascii_xdigit_value (*i) << 4;
    c |= g_ascii_xdigit_value (*(i + 1));

    *o = MAILTC_COLOUR_VALUE (c);

    return TRUE;
}

gboolean
mailtc_colour_parse (MailtcColour* colour,
                     const gchar*  str)
{
    g_assert (colour);
    g_return_val_if_fail (str && strlen (str) == 6, FALSE);
    g_return_val_if_fail (mailtc_colour_parse_value (str, &colour->red), FALSE);
    g_return_val_if_fail (mailtc_colour_parse_value (str + 2, &colour->green), FALSE);
    g_return_val_if_fail (mailtc_colour_parse_value (str + 4, &colour->blue), FALSE);

    return TRUE;
}

gchar*
mailtc_colour_to_string (const MailtcColour* colour)
{
    g_assert (colour);
    return g_strdup_printf ("%02X%02X%02X",
                             MAILTC_HEX_COLOUR_VALUE (colour->red),
                             MAILTC_HEX_COLOUR_VALUE (colour->green),
                             MAILTC_HEX_COLOUR_VALUE (colour->blue));
}

gboolean
mailtc_colour_equal (const MailtcColour* a,
                     const MailtcColour* b)
{
    return (a->red == b->red && a->green == b->green && a->blue == b->blue) ? TRUE : FALSE;
}

