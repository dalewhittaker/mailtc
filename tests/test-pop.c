/* test-pop.c
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


/* NOTE - This test must be run with appropriate permissions. */

#include <string.h> /* strlen () */
#include <stdlib.h> /* atoi () */
#include <glib.h>
#include <glib/gstdio.h>
#include <config.h>

#include "mtc-account.h"
#include "mtc-extension.h"
#include "mtc-module.h"

#define CONFIGDIR ".config"
#define POP_PORT 110
#define POPS_PORT 995
#define TIMEOUT_NET 5
#define TIMEOUT_FREQ 100

#define POP_CONNECT_SERVER   0
#define POP_CONNECT_PORT     1
#define POP_COMMAND_USER     2
#define POP_COMMAND_PASSWORD 3
#define POP_COMMAND_STAT     4
#define POP_COMMAND_UIDL     5
#define POP_COMMAND_QUIT     6

typedef struct
{
    const gchar* name;
    const gchar* server;
    guint port;
    const gchar* user;
    const gchar* password;
} account_data;

typedef struct
{
    GArray* protocols;
    guint command;
    guint protocol;
    gint nuids;
    gboolean success;
    char buf[512];
    GString* msg;
    GIOStream* connection;
    GInputStream* istream;
    GOutputStream* ostream;
    GMainLoop* loop;
    GThread* thread;
    MailtcExtension* extension;
    account_data accounts[3];
    guint user;
} server_data;

static void
server_read (GInputStream* istream,
             server_data*  data);

static void
server_write (GOutputStream* ostream,
              gchar*         buf,
              server_data*   data);

static gpointer
run_plugin_thread (server_data* data)
{
    MailtcAccount* account;
    MailtcExtension* extension;
    MailtcProtocol* protocol;
    account_data* accdata;
    account_data* baddata;
    guint errval;
    const gchar* server;
    guint port;
    const gchar* user;
    const gchar* password;
    gint messages = 0;
    GError* error = NULL;

    g_assert (data);

    extension = data->extension;
    g_assert (MAILTC_IS_EXTENSION (extension));

    protocol = &g_array_index (data->protocols, MailtcProtocol, data->protocol);
    accdata = &data->accounts[data->protocol];
    baddata = &data->accounts[data->protocols->len];

    account = mailtc_account_new ();

    mailtc_account_set_protocol (account, data->protocol);
    mailtc_account_set_name (account, accdata->name);

    errval = POP_CONNECT_SERVER;

    while (errval <= POP_COMMAND_STAT)
    {
        data->success = TRUE;

        server = errval == POP_CONNECT_SERVER ? baddata->server : accdata->server;
        port = errval == POP_CONNECT_PORT ? baddata->port : accdata->port;
        user = errval == POP_COMMAND_USER ? baddata->user : accdata->user;
        password = errval == POP_COMMAND_PASSWORD ? baddata->password : accdata->password;

        mailtc_account_set_server (account, server);
        mailtc_account_set_port (account, port);
        mailtc_account_set_user (account, user);
        mailtc_account_set_password (account, password);
        g_assert (mailtc_account_update_extension (account, extension, &error));

        g_print ("\n");
        messages = mailtc_extension_get_messages (extension, G_OBJECT (account), TRUE, &error);
        if (error)
        {
            if (errval == POP_CONNECT_SERVER && error->domain == G_RESOLVER_ERROR && (error->code == G_RESOLVER_ERROR_NOT_FOUND || error->code == G_RESOLVER_ERROR_TEMPORARY_FAILURE))
            {
                g_print ("%s\n", error->message);
                g_clear_error (&error);
            }
            else if (errval == POP_CONNECT_PORT && error->domain == G_IO_ERROR && error->code == G_IO_ERROR_CONNECTION_REFUSED)
            {
                g_print ("%s\n", error->message);
                g_clear_error (&error);
            }
            else if (errval == data->command - 1 && !g_strcmp0 (g_quark_to_string (error->domain), "MAILTC_POP_ERROR") && error->code == 0)
            {
                g_print ("%s\n", error->message);
                g_clear_error (&error);
            }
        }
        g_assert_no_error (error);
        g_print ("messages = %d\n", messages);

        ++errval;
        /* FIXME we'll want to mark as read too. */
    }

    g_object_unref (account);

    return NULL;
}

static gboolean
run_plugin (server_data* data)
{
    data->thread = g_thread_create ((GThreadFunc) run_plugin_thread, data, TRUE, NULL);
    return FALSE;
}

static void
server_close_stream_finish (GObject*      object,
                            GAsyncResult* res,
                            server_data*  data)
{
    GError* error = NULL;

    if (G_IS_INPUT_STREAM (object))
    {
        g_input_stream_close_finish (G_INPUT_STREAM (object), res, &error);
        data->istream = NULL;
    }
    else
    {
        g_assert (G_IS_OUTPUT_STREAM (object));
        g_output_stream_close_finish (G_OUTPUT_STREAM (object), res, &error);
        data->ostream = NULL;
    }
    g_assert_no_error (error);

    if (!data->istream && !data->ostream)
    {
        if (++data->protocol < data->protocols->len)
        {
            g_object_unref (data->connection);
            g_idle_add ((GSourceFunc) run_plugin, data);
        }
        else
        {
            GIOStream* base_connection;

            g_object_get (data->connection, "base-io-stream", &base_connection, NULL);
            g_object_unref (data->connection);
            g_object_unref (base_connection);
            g_thread_join (data->thread);
            g_main_loop_quit (data->loop);
        }
    }
}

static void
server_read_write_finish (GObject*      stream,
                          GAsyncResult* res,
                          server_data*  data)
{
    gchar** sv;
    guint svlen;
    guint command;
    gint uid;
    gssize read;
    GString* s;
    GError* error = NULL;

    if (G_IS_OUTPUT_STREAM (stream))
    {
        g_output_stream_write_finish (G_OUTPUT_STREAM (stream), res, &error);
        g_assert_no_error (error);

        if (data->success)
            server_read (data->istream, data);

        if (data->command > POP_COMMAND_QUIT)
            g_output_stream_close_async (G_OUTPUT_STREAM (stream), G_PRIORITY_DEFAULT, NULL, (GAsyncReadyCallback) server_close_stream_finish, data);

        return;
    }

    g_assert (G_IS_INPUT_STREAM (stream));
    read = g_input_stream_read_finish (G_INPUT_STREAM (stream), res, &error);
    g_assert_no_error (error);
    data->buf[read] = '\0';

    s = data->msg;
    command = data->command;
    data->command++;

    /* FIXME want to simulate server blocking errors at each point. */
    /* FIXME tidy all this code at some point. */
    switch (command)
    {
        case POP_COMMAND_USER:
            sv = g_strsplit_set (data->buf, " \r\n", 0);
            svlen = g_strv_length (sv);
            g_assert (svlen == 4 && !g_strcmp0 (sv[0], "USER") && strlen (sv[2]) == 0 && strlen (sv[3]) == 0);

            if (!g_strcmp0 (sv[1], data->accounts[0].user))
            {
                data->user = 0;
                data->success = TRUE;
            }
            else if (!g_strcmp0 (sv[1], data->accounts[1].user))
            {
                data->user = 1;
                data->success = TRUE;
            }
            else
                data->success = FALSE;

            if (data->success)
                g_string_printf (s, "+OK Hello %s, can i have your password?\r\n", sv[1]);
            else
                g_string_printf (s, "-ERR I don't know any %s.\r\n", sv[1]);
            server_write (data->ostream, s->str, data);
            g_strfreev (sv);
            break;

        case POP_COMMAND_PASSWORD:
            sv = g_strsplit_set (data->buf, " \r\n", 0);
            svlen = g_strv_length (sv);
            g_assert (svlen == 4 && !g_strcmp0 (sv[0], "PASS") && strlen (sv[2]) == 0 && strlen (sv[3]) == 0);
            data->success = (!g_strcmp0 (sv[1], data->accounts[0].password) || !g_strcmp0 (sv[1], data->accounts[1].password)) ? TRUE : FALSE;
            if (data->success)
                server_write (data->ostream, "+OK We are good.\r\n", data);
            else
            {
                g_string_printf (s, "-ERR Hey you're not %s!\r\n", data->accounts[data->user].user);
                server_write (data->ostream, s->str, data);
            }
            g_strfreev (sv);
            break;

        case POP_COMMAND_STAT:
            sv = g_strsplit_set (data->buf, " \r\n", 0);
            svlen = g_strv_length (sv);
            g_assert (svlen == 3 && !g_strcmp0 (sv[0], "STAT") && strlen (sv[1]) == 0 && strlen (sv[2]) == 0);
            /* The plugin doesn't use the maildrop size param, so we can just use 1. */
            data->nuids = g_random_int_range (1, 5);
            g_string_printf (s, "+OK %d 1.\r\n", data->nuids);
            server_write (data->ostream, s->str, data);
            g_strfreev (sv);
            break;

        case POP_COMMAND_UIDL:
            sv = g_strsplit_set (data->buf, " \r\n", 0);
            svlen = g_strv_length (sv);
            g_assert (svlen == 4 && !g_strcmp0 (sv[0], "UIDL") && strlen (sv[2]) == 0 && strlen (sv[3]) == 0);
            uid = atoi (sv[1]);
            g_assert (uid <= data->nuids);
            if (uid < data->nuids)
                data->command = POP_COMMAND_UIDL;
            g_string_printf (s, "+OK MTC-%s\r\n", sv[1]);
            g_strfreev (sv);
            server_write (data->ostream, s->str, data);
            break;

        case POP_COMMAND_QUIT:
            server_write (data->ostream, "+OK See ya!.\r\n", data);
            break;

        default:
            g_input_stream_close_async (G_INPUT_STREAM (stream), G_PRIORITY_DEFAULT, NULL, (GAsyncReadyCallback) server_close_stream_finish, data);
    }
}

static void
server_read (GInputStream* istream,
             server_data*  data)
{
    g_input_stream_read_async (istream, data->buf, sizeof (data->buf), G_PRIORITY_DEFAULT, NULL, (GAsyncReadyCallback) server_read_write_finish, data);
}

static void
server_write (GOutputStream* ostream,
              gchar*         buf,
              server_data*   data)
{
    gsize bytes;

    bytes = strlen (buf);
    g_output_stream_write_async (ostream, buf, bytes, G_PRIORITY_DEFAULT, NULL, (GAsyncReadyCallback) server_read_write_finish, data);
}

static gboolean
service_incoming_cb (GSocketService*    service,
                     GSocketConnection* connection,
                     GObject*           source_object,
                     server_data*       data)
{
    GIOStream* conn;

    (void) service;
    (void) source_object;

    if (data->protocol == 1)
    {
        GTlsCertificate* cert;
        GError* error = NULL;

        /* server.pem was a file generated as follows:
         * openssl genrsa -out key.pem 1024
         * openssl req -new -x509 -key.pem -out server.pem -days 1095
         * cat key.pem >> server.pem
         * FIXME autogenerate server.pem from either here or Makefile.am
         */
        cert = g_tls_certificate_new_from_file (SRCDIR "/server.pem", &error);
        g_assert_no_error (error);
        conn = g_tls_server_connection_new (G_IO_STREAM (connection), cert, &error);
        g_assert_no_error (error);
        g_object_unref (cert);
        g_tls_connection_set_require_close_notify (G_TLS_CONNECTION (conn), FALSE);
        g_tls_connection_handshake (G_TLS_CONNECTION (conn), NULL, &error);
        g_assert_no_error (error);
    }
    else
        conn = G_IO_STREAM (connection);

    g_object_ref (conn);
    data->connection = conn;
    data->msg = g_string_new (NULL);
    data->command = POP_COMMAND_USER;
    data->istream = g_io_stream_get_input_stream (conn);
    data->ostream = g_io_stream_get_output_stream (conn);
    server_write (data->ostream, "+OK MTC test server\r\n", data);

    return TRUE;
}

static void
test_pop (void)
{
    MailtcModule* module;
    MailtcExtension* extension;
    MailtcProtocol* protocol;
    MailtcExtensionInitFunc extension_init;
    GMainLoop* loop = NULL;
    GInetAddress* address;
    GSocketAddress* sock_address;
    GSocketService* service;
    GSList* extensions;
    GArray* protocols;
    GDir* dir;
    GError* error = NULL;
    guint i;
    const gchar* filename;
    gchar* fullname;
    gchar* directory;
    gchar* edirectory;
    server_data data;

    dir = g_dir_open (PLUGINDIR, 0, &error);
    g_assert (dir);

    /* Find the pop plugin. */
    while ((filename = g_dir_read_name (dir)))
    {
        if (g_str_has_prefix (filename, "net") && g_str_has_suffix (filename, G_MODULE_SUFFIX))
            break;
    }
    g_assert (filename);

    /* Load it. */
    fullname = g_build_filename (PLUGINDIR, filename, NULL);
    g_assert (fullname);
    directory = g_build_filename (CONFIGDIR, PACKAGE, NULL);
    g_assert (directory);
    edirectory = g_build_filename (directory, filename, NULL);
    g_assert (edirectory);
    g_mkdir_with_parents (edirectory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    module = mailtc_module_new ();
    g_assert (module);
    g_assert (mailtc_module_load (module, fullname, &error));
    g_assert (mailtc_module_symbol (module, MAILTC_EXTENSION_SYMBOL_INIT, (gpointer*) &extension_init, &error));
    g_assert (extension_init);

    g_type_class_ref (MAILTC_TYPE_EXTENSION);

    extensions = extension_init (directory);

    /* FIXME assumes one extension. */
    extension = extensions->data;
    g_assert (extension);
    g_assert (mailtc_extension_is_valid (extension, NULL));
    protocols = mailtc_extension_get_protocols (extension);

    data.accounts[0].name = "POP account 1";
    data.accounts[0].user = "Fred";
    data.accounts[0].password = "abc123";

    data.accounts[1].name = "POPTLS account 2";
    data.accounts[1].user = "Bob";
    data.accounts[1].password = "def456";

    data.accounts[2].server = "localhost1";
    data.accounts[2].port = 123;
    data.accounts[2].name = "bad account";
    data.accounts[2].user = "Bill";
    data.accounts[2].password = "ghi789";

    service = g_socket_service_new ();
    address = g_inet_address_new_from_string ("127.0.0.1");
    for (i = 0; i < protocols->len; i++)
    {
        protocol = &g_array_index (protocols, MailtcProtocol, i);
        sock_address = g_inet_socket_address_new (address, protocol->port);
        g_socket_listener_add_address (G_SOCKET_LISTENER (service), sock_address, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP, NULL, NULL, &error);
        g_assert_no_error (error);
        g_object_unref (sock_address);

        data.accounts[i].server = "localhost";
        data.accounts[i].port = protocol->port;
    }
    g_object_unref (address);

    g_signal_connect (service, "incoming", G_CALLBACK (service_incoming_cb), &data);

    g_socket_service_start (service);

    mailtc_extension_set_module (extension, G_OBJECT (module));

    loop = g_main_loop_new (NULL, FALSE);
    g_assert (loop);
    data.loop = loop;
    data.protocols = protocols;
    data.protocol = 0;
    data.extension = extension;
    g_array_unref (protocols);

    g_idle_add ((GSourceFunc) run_plugin, &data);
    g_main_loop_run (loop);

    g_main_loop_unref (loop);

    if (data.msg)
        g_string_free (data.msg, TRUE);

    g_slist_free (extensions);
    g_assert (mailtc_module_unload (module, &error));
    g_assert (g_remove (edirectory) == 0);
    g_assert (g_remove (directory) == 0);
    g_assert (g_remove (CONFIGDIR) == 0);

    g_object_unref (module);
    g_free (directory);
    g_free (fullname);
    g_dir_close (dir);

    g_socket_service_stop (service);
    g_object_unref (service);

    g_print ("done\n");
}

int
main (int    argc,
      char** argv)
{
    g_type_init ();

#if 1
    g_test_init (&argc, &argv, NULL);
    g_test_add_func ("/plugin/pop", test_pop);

    return g_test_run ();
#else
    (void) argc;
    (void) argv;
    test_pop ();

    return 0;
#endif
}

