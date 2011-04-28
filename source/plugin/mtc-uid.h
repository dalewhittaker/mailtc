/* mtc-uid.h
 * Copyright (C) 2009-2011 Dale Whittaker <dayul@users.sf.net>
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

#ifndef __MAILTC_UID_H__
#define __MAILTC_UID_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_UID_TABLE            (mailtc_uid_table_get_type  ())
#define MAILTC_UID_TABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAILTC_TYPE_UID_TABLE, MailtcUidTable))
#define MAILTC_UID_TABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  MAILTC_TYPE_UID_TABLE, MailtcUidTableClass))
#define MAILTC_IS_UID_TABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAILTC_TYPE_UID_TABLE))
#define MAILTC_IS_UID_TABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  MAILTC_TYPE_UID_TABLE))
#define MAILTC_UID_TABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  MAILTC_TYPE_UID_TABLE, MailtcUidTableClass))

typedef struct _MailtcUidTable        MailtcUidTable;
typedef struct _MailtcUidTableClass   MailtcUidTableClass;
typedef struct _MailtcUidTablePrivate MailtcUidTablePrivate;

gboolean
mailtc_uid_table_load       (MailtcUidTable* uid_table,
                             GError**        error);

void
mailtc_uid_table_age        (MailtcUidTable* uid_table);

gint64
mailtc_uid_table_remove_old (MailtcUidTable* uid_table);

void
mailtc_uid_table_add        (MailtcUidTable* uid_table,
                             gchar*          uid);

gboolean
mailtc_uid_table_mark_read  (MailtcUidTable* uid_table,
                             GError**        error);

GType
mailtc_uid_table_get_type   (void);

MailtcUidTable*
mailtc_uid_table_new        (gchar* filename);

G_END_DECLS

#endif /* __MAILTC_UID_H__ */

