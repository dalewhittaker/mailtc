/* mtc-net.h
 * Copyright (C) 2009-2022 Dale Whittaker
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

#ifndef __MAILTC_NET_H__
#define __MAILTC_NET_H__

#include "mailtc.h"
#include "mtc-uid.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_NET (mailtc_net_get_type  ())

#define MAILTC_NET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAILTC_TYPE_NET, MailtcNet))
#define MAILTC_NET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  MAILTC_TYPE_NET, MailtcNetClass))
#define MAILTC_IS_NET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAILTC_TYPE_NET))
#define MAILTC_IS_NET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  MAILTC_TYPE_NET))
#define MAILTC_NET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  MAILTC_TYPE_NET, MailtcNetClass))

typedef struct _MailtcNet        MailtcNet;
typedef struct _MailtcNetClass   MailtcNetClass;
typedef struct _MailtcNetPrivate MailtcNetPrivate;

struct _MailtcNet
{
    MailtcExtension parent_instance;

    MailtcNetPrivate* priv;
};

struct _MailtcNetClass
{
    MailtcExtensionClass parent_class;
};

GType
mailtc_net_get_type     (void);

MailtcNet*
mailtc_net_new          (void);

gssize
mailtc_net_read         (MailtcNet*   net,
                         GString*     msg,
                         gboolean     debug,
                         const gchar* endchars,
                         guint        endlen,
                         GError**     error);

gssize
mailtc_net_write        (MailtcNet*   net,
                         GString*     msg,
                         gboolean     debug,
                         GError**     error);

void
mailtc_net_disconnect   (MailtcNet*   net);

gboolean
mailtc_net_connect      (MailtcNet*   net,
                         const gchar* server,
                         guint        port,
                         gboolean     tls,
                         GError**     error);

gboolean
mailtc_net_supports_tls (void);

G_END_DECLS

#endif /* __MAILTC_NET_H__ */

