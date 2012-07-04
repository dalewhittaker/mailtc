/* mtc-envelope.h
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

#ifndef __MAILTC_ENVELOPE_H__
#define __MAILTC_ENVELOPE_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_ENVELOPE            (mailtc_envelope_get_type  ())
#define MAILTC_ENVELOPE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAILTC_TYPE_ENVELOPE, MailtcEnvelope))
#define MAILTC_ENVELOPE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  MAILTC_TYPE_ENVELOPE, MailtcEnvelopeClass))
#define MAILTC_IS_ENVELOPE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAILTC_TYPE_ENVELOPE))
#define MAILTC_IS_ENVELOPE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  MAILTC_TYPE_ENVELOPE))
#define MAILTC_ENVELOPE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  MAILTC_TYPE_ENVELOPE, MailtcEnvelopeClass))

typedef struct _MailtcEnvelope        MailtcEnvelope;
typedef struct _MailtcEnvelopeClass   MailtcEnvelopeClass;
typedef struct _MailtcEnvelopePrivate MailtcEnvelopePrivate;

GType
mailtc_envelope_get_type   (void);

GtkWidget*
mailtc_envelope_new        (void);

void
mailtc_envelope_set_colour (MailtcEnvelope* envelope,
                            const GdkColor* colour);

void
mailtc_envelope_get_colour (MailtcEnvelope* envelope,
                            GdkColor*       colour);

GdkPixbuf*
mailtc_envelope_get_pixbuf (MailtcEnvelope* envelope);

G_END_DECLS

#endif /* __MAILTC_ENVELOPE_H__ */

