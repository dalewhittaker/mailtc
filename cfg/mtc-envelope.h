/* mtc-envelope.h
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

#ifndef __MAILTC_ENVELOPE_H__
#define __MAILTC_ENVELOPE_H__

#include "mtc-colour.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_ENVELOPE (mailtc_envelope_get_type ())

G_DECLARE_FINAL_TYPE         (MailtcEnvelope, mailtc_envelope, MAILTC, ENVELOPE, GtkImage)

MailtcEnvelope*
mailtc_envelope_new          (void);

void
mailtc_envelope_set_colour   (MailtcEnvelope*     envelope,
                              const MailtcColour* colour);

void
mailtc_envelope_get_colour   (MailtcEnvelope*     envelope,
                              MailtcColour*       colour);

G_END_DECLS

#endif /* __MAILTC_ENVELOPE_H__ */

