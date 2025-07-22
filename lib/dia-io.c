/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * © 2025 Zander Brown <zbrown@gnome.org>
 */

#include "config.h"

#define G_LOG_DOMAIN "DiaIO"

#include <glib/gi18n-lib.h>

#include <glib.h>
#include <libxml/parser.h>
#include <libxml/xmlsave.h>

#include "diacontext.h"

#include "dia-io.h"


typedef struct _ReadContext ReadContext;
struct _ReadContext {
  DiaContext *dia_ctx;
  xmlParserCtxtPtr xml_ctx;

  GFile *source;
  char *uri;
  GFileInputStream *base_stream;
  GBufferedInputStream *buffered_stream;
  GZlibDecompressor *decompressor;
  GConverterInputStream *converter_stream;
  GInputStream *stream;
  GCancellable *cancellable;
  gboolean was_compressed;
  /* TODO: We should try to keep BOM'd files BOM'd */
  gboolean had_bom;
};


static void
read_context_error_handler (gpointer user_data, const xmlError *error)
{
  ReadContext *read_ctx = user_data;

  dia_context_add_message (read_ctx->dia_ctx,
                           _("Unable to parse: %s"),
                           error->message);
}


static inline ReadContext *
read_context_new (GFile *file, DiaContext *ctx)
{
  ReadContext *self = g_new0 (ReadContext, 1);

  g_set_object (&self->dia_ctx, ctx);

  self->xml_ctx = xmlNewParserCtxt ();
  xmlCtxtSetErrorHandler (self->xml_ctx, read_context_error_handler, self);

  g_set_object (&self->source, file);
  self->uri = g_file_get_uri (self->source);
  self->was_compressed = FALSE;

  return self;
}


static void
read_context_free (ReadContext *self)
{
  g_clear_object (&self->dia_ctx);
  g_clear_pointer (&self->xml_ctx, xmlFreeParserCtxt);

  g_clear_object (&self->source);
  g_clear_pointer (&self->uri, g_free);
  g_clear_object (&self->base_stream);
  g_clear_object (&self->buffered_stream);
  g_clear_object (&self->decompressor);
  g_clear_object (&self->converter_stream);
  g_clear_object (&self->stream);
  g_clear_object (&self->cancellable);

  g_free (self);
}


static int
read_context_read (gpointer user_data, char *chunk, int size)
{
  ReadContext *read_ctx = user_data;
  GError *error = NULL;
  ssize_t read = g_input_stream_read (read_ctx->stream,
                                      chunk,
                                      size,
                                      read_ctx->cancellable,
                                      &error);

  g_debug ("%s: read %i (%" G_GSSIZE_FORMAT ")", read_ctx->uri, size, read);

  if (error) {
    dia_context_add_message (read_ctx->dia_ctx,
                             _("Unable to read: %s"),
                             error->message);

    g_clear_error (&error);

    return -1;
  }

  return read;
}


static int
read_context_close (gpointer user_data)
{
  ReadContext *read_ctx = user_data;
  GError *error = NULL;

  g_input_stream_close (read_ctx->stream, read_ctx->cancellable, &error);

  g_debug ("%s: close", read_ctx->uri);

  if (error) {
    dia_context_add_message (read_ctx->dia_ctx,
                             _("Unable to close: %s"),
                             error->message);

    g_clear_error (&error);

    return -1;
  }

  return 0;
}


static inline void
read_context_maybe_mixin_decompressor (ReadContext *ctx)
{
  char peeked[3] = { 0, };
  size_t peeked_len =
    g_buffered_input_stream_peek (ctx->buffered_stream, &peeked, 0, 3);

  /* An uncompressed utf-8 XML file will either begin with a BOM, or have ‘<’
   * as the first byte, so let see what the first three bytes are. */

  if (peeked_len > 2 &&
      peeked[0] == '\xEF' &&
      peeked[1] == '\xBB' &&
      peeked[2] == '\xBF') {
    ctx->had_bom = TRUE;
  } else if (peeked_len > 1) {
    ctx->was_compressed = peeked[0] != '<';
  } else {
    g_warning ("Unable to check for compression");
  }

  if (ctx->was_compressed) {
    ctx->decompressor =
      g_zlib_decompressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP);

    ctx->converter_stream =
      G_CONVERTER_INPUT_STREAM (g_converter_input_stream_new (G_INPUT_STREAM (ctx->stream),
                                                              G_CONVERTER (ctx->decompressor)));

    g_set_object (&ctx->stream, G_INPUT_STREAM (ctx->converter_stream));
  }
}


xmlDocPtr
dia_io_load_document (const char *path,
                      DiaContext *ctx,
                      gboolean   *was_compressed)
{
  GError *error = NULL;
  GFile *file = g_file_new_for_path (path);
  ReadContext *read_ctx = read_context_new (file, ctx);
  xmlDocPtr doc = NULL;

  g_debug ("%s: open", read_ctx->uri);

  read_ctx->cancellable = g_cancellable_new ();
  read_ctx->base_stream = g_file_read (read_ctx->source,
                                       read_ctx->cancellable,
                                       &error);

  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_IS_DIRECTORY)) {
    char *basename = g_file_get_basename (file);

    dia_context_add_message (ctx, _("‘%s’ is a directory"), basename);

    g_clear_pointer (&basename, g_free);

    goto out;
  } else if (error) {
    dia_context_add_message (ctx, _("Unable to open: %s"), error->message);

    goto out;
  }

  /* By wrapping in a buffered stream we gain the ability to peek into
   * the file without needing the storage to be seekable. */
  read_ctx->buffered_stream =
    G_BUFFERED_INPUT_STREAM (g_buffered_input_stream_new (G_INPUT_STREAM (read_ctx->base_stream)));

  g_set_object (&read_ctx->stream, G_INPUT_STREAM (read_ctx->buffered_stream));

  /* Preload a chunk into the buffer so we can peek it */
  g_buffered_input_stream_fill (read_ctx->buffered_stream,
                                -1,
                                NULL,
                                &error);

  if (error) {
    dia_context_add_message (ctx, _("Unable to read: %s"), error->message);

    goto out;
  }

  read_context_maybe_mixin_decompressor (read_ctx);

  doc = xmlCtxtReadIO (read_ctx->xml_ctx,
                       read_context_read,
                       read_context_close,
                       read_ctx,
                       read_ctx->uri,
                       NULL,
                       0);

  if (was_compressed) {
    *was_compressed = read_ctx->was_compressed;
  }

out:
  g_clear_pointer (&read_ctx, read_context_free);
  g_clear_object (&file);
  g_clear_error (&error);

  return doc;
}


typedef struct _WriteContext WriteContext;
struct _WriteContext {
  DiaContext *dia_ctx;
  xmlSaveCtxtPtr xml_ctx;

  GFile *target;
  char *uri;
  GFileOutputStream *base_stream;
  GZlibCompressor *compressor;
  GConverterOutputStream *converter_stream;
  GOutputStream *stream;
  GCancellable *cancellable;
};


static void
write_context_free (WriteContext *self)
{
  g_clear_object (&self->dia_ctx);
  g_clear_pointer (&self->xml_ctx, xmlSaveClose);

  g_clear_object (&self->target);
  g_clear_pointer (&self->uri, g_free);
  g_clear_object (&self->base_stream);
  g_clear_object (&self->compressor);
  g_clear_object (&self->converter_stream);
  g_clear_object (&self->stream);
  g_clear_object (&self->cancellable);

  g_free (self);
}


static int
write_context_write (gpointer user_data, const char *chunk, int size)
{
  WriteContext *write_ctx = user_data;
  GError *error = NULL;
  ssize_t written = g_output_stream_write (write_ctx->stream,
                                           chunk,
                                           size,
                                           write_ctx->cancellable,
                                           &error);

  g_debug ("%s: write %i (%" G_GSSIZE_FORMAT ")", write_ctx->uri, size, written);

  if (error) {
    dia_context_add_message (write_ctx->dia_ctx,
                             _("Unable to write: %s"),
                             error->message);

    g_clear_error (&error);

    return -1;
  }

  return written;
}


static int
write_context_close (gpointer user_data)
{
  WriteContext *write_ctx = user_data;
  GError *error = NULL;

  g_output_stream_close (write_ctx->stream, write_ctx->cancellable, &error);

  g_debug ("%s: close", write_ctx->uri);

  if (error) {
    dia_context_add_message (write_ctx->dia_ctx,
                             _("Unable to close: %s"),
                             error->message);

    g_clear_error (&error);

    return -1;
  }

  return 0;
}


static inline WriteContext *
write_context_new (GFile *file, DiaContext *ctx)
{
  WriteContext *self = g_new0 (WriteContext, 1);

  g_set_object (&self->dia_ctx, ctx);

  self->xml_ctx = xmlSaveToIO (write_context_write,
                               write_context_close,
                               self,
                               "UTF-8",
                               XML_SAVE_AS_XML | XML_SAVE_EMPTY | XML_SAVE_FORMAT);

  g_set_object (&self->target, file);
  self->uri = g_file_get_uri (self->target);

  return self;
}


static inline void
write_context_mixin_compressor (WriteContext *ctx)
{
  char *basename = g_file_get_basename (ctx->target);
  GFileInfo *info = g_file_info_new ();

  ctx->compressor =
    g_zlib_compressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP, 9);

  /* Generally gzip'd files are named name.ext.gz, but we call our compressed
   * files name.dia, the same as uncompressed…
   *
   * Resultingly various tools that work on gzip'd files are inclined to split
   * off the .dia and assume the file should be simply ‘name’.
   *
   * Fortunately gzip supports including a header that, amongst other things,
   * can contain the ‘true name’ of the file, which resolves at least a lot
   * of these issues.
   */
  g_file_info_set_name (info, basename);

  g_zlib_compressor_set_file_info (ctx->compressor, info);

  ctx->converter_stream =
    G_CONVERTER_OUTPUT_STREAM (g_converter_output_stream_new (G_OUTPUT_STREAM (ctx->stream),
                                                              G_CONVERTER (ctx->compressor)));

  g_set_object (&ctx->stream, G_OUTPUT_STREAM (ctx->converter_stream));

  g_clear_pointer (&basename, g_free);
  g_clear_object (&info);
}


gboolean
dia_io_save_document (const char *path,
                      xmlDocPtr   doc,
                      gboolean    compresssed,
                      DiaContext *ctx)
{
  GError *error = NULL;
  GFile *file = g_file_new_for_path (path);
  WriteContext *write_ctx = write_context_new (file, ctx);
  gboolean result = FALSE;

  write_ctx->cancellable = g_cancellable_new ();
  write_ctx->base_stream = g_file_replace (write_ctx->target,
                                           NULL,
                                           TRUE,
                                           G_FILE_CREATE_PRIVATE,
                                           write_ctx->cancellable,
                                           &error);

  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_IS_DIRECTORY)) {
    char *basename = g_file_get_basename (file);

    dia_context_add_message (ctx, _("‘%s’ is a directory"), basename);

    g_clear_pointer (&basename, g_free);

    goto out;
  } else if (error) {
    dia_context_add_message (ctx, _("Unable to open: %s"), error->message);

    goto out;
  }

  g_set_object (&write_ctx->stream, G_OUTPUT_STREAM (write_ctx->base_stream));

  if (compresssed) {
    write_context_mixin_compressor (write_ctx);
  }

  /* Make sure libxml2 doesn't try do it's own compression */
  xmlSetDocCompressMode (doc, 0);

  if (xmlSaveDoc (write_ctx->xml_ctx, doc) != 0) {
    result = FALSE;
    goto out;
  }

  if (xmlSaveFlush (write_ctx->xml_ctx) < 0) {
    result = FALSE;
    goto out;
  }

  result = TRUE;

out:
  g_clear_pointer (&write_ctx, write_context_free);

  return result;
}
