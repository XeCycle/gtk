/*
 * GTK - The GIMP Toolkit
 * Copyright (C) 2022 Red Hat, Inc.
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gtkfontdialog.h"

#include "gtkfontchooserdialog.h"
#include "gtkbutton.h"
#include "gtktypebuiltins.h"
#include <glib/gi18n-lib.h>

/**
 * GtkFontDialog:
 *
 * A `GtkFontDialog` object collects the arguments that
 * are needed to present a font chooser dialog to the
 * user, such as a title for the dialog and whether it
 * should be modal.
 *
 * The dialog is shown with the [function@Gtk.FontDialog.choose_font]
 * function. This API follows the GIO async pattern, and the
 * result can be obtained by calling
 * [function@Gtk.FontDialog.choose_font_finish].
 *
 * See [class@Gtk.FontDialogButton] for a convenient control
 * that uses `GtkFontDialog` and presents the results.
 *
 * `GtkFontDialog was added in GTK 4.10.
 */

/* {{{ GObject implementation */

struct _GtkFontDialog
{
  GObject parent_instance;

  char *title;
  GtkFontChooserLevel level;
  PangoLanguage *language;
  PangoFontMap *fontmap;

  unsigned int modal : 1;

  GtkFontFilterFunc filter;
  gpointer filter_data;
  GDestroyNotify filter_data_destroy;
};

enum
{
  PROP_TITLE = 1,
  PROP_MODAL,
  PROP_LEVEL,
  PROP_LANGUAGE,
  PROP_FONTMAP,

  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES];

G_DEFINE_TYPE (GtkFontDialog, gtk_font_dialog, G_TYPE_OBJECT)

#define DEFAULT_LEVEL (GTK_FONT_CHOOSER_LEVEL_FAMILY| \
                       GTK_FONT_CHOOSER_LEVEL_STYLE| \
                       GTK_FONT_CHOOSER_LEVEL_SIZE)

static void
gtk_font_dialog_init (GtkFontDialog *self)
{
  self->title = g_strdup (_("Pick a Font"));
  self->modal = TRUE;
  self->level = DEFAULT_LEVEL;
  self->language = pango_language_get_default ();
}

static void
gtk_font_dialog_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GtkFontDialog *self = GTK_FONT_DIALOG (object);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;

    case PROP_MODAL:
      g_value_set_boolean (value, self->modal);
      break;

    case PROP_LEVEL:
      g_value_set_flags (value, self->level);
      break;

    case PROP_LANGUAGE:
      g_value_set_boxed (value, self->language);
      break;

    case PROP_FONTMAP:
      g_value_set_object (value, self->fontmap);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gtk_font_dialog_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GtkFontDialog *self = GTK_FONT_DIALOG (object);

  switch (property_id)
    {
    case PROP_TITLE:
      gtk_font_dialog_set_title (self, g_value_get_string (value));
      break;

    case PROP_MODAL:
      gtk_font_dialog_set_modal (self, g_value_get_boolean (value));
      break;

    case PROP_LEVEL:
      gtk_font_dialog_set_level (self, g_value_get_flags (value));
      break;

    case PROP_LANGUAGE:
      gtk_font_dialog_set_language (self, g_value_get_boxed (value));
      break;

    case PROP_FONTMAP:
      gtk_font_dialog_set_fontmap (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gtk_font_dialog_finalize (GObject *object)
{
  GtkFontDialog *self = GTK_FONT_DIALOG (object);

  g_free (self->title);
  g_clear_object (&self->fontmap);
  g_clear_pointer (&self->filter_data, self->filter_data_destroy);

  G_OBJECT_CLASS (gtk_font_dialog_parent_class)->finalize (object);
}

static void
gtk_font_dialog_class_init (GtkFontDialogClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->get_property = gtk_font_dialog_get_property;
  object_class->set_property = gtk_font_dialog_set_property;
  object_class->finalize = gtk_font_dialog_finalize;

  /**
   * GtkFontDialog:title: (attributes org.gtk.Property.get=gtk_font_dialog_get_title org.gtk.Property.set=gtk_font_dialog_set_title)
   *
   * A title that may be shown on the font chooser
   * dialog that is presented by [function@Gtk.FontDialog.choose_font].
   *
   * Since: 4.10
   */
  properties[PROP_TITLE] =
      g_param_spec_string ("title", NULL, NULL,
                           NULL,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GtkFontDialog:modal: (attributes org.gtk.Property.get=gtk_font_dialog_get_modal org.gtk.Property.set=gtk_font_dialog_set_modal)
   *
   * Whether the font chooser dialog is modal.
   *
   * Since: 4.10
   */
  properties[PROP_MODAL] =
      g_param_spec_boolean ("modal", NULL, NULL,
                            TRUE,
                            G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);


  /**
   * GtkFontDialog:level: (attributes org.gtk.Property.get=gtk_font_dialog_get_level org.gtk.Property.set=gtk_font_dialog_set_level)
   *
   * The level of granularity to offer for selecting fonts.
   *
   * Since: 4.10
   */
  properties[PROP_LEVEL] =
      g_param_spec_flags ("level", NULL, NULL,
                          GTK_TYPE_FONT_CHOOSER_LEVEL,
                          DEFAULT_LEVEL,
                          G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GtkFontDialog:language: (attributes org.gtk.Property.get=gtk_font_dialog_get_language org.gtk.Property.set=gtk_font_dialog_set_language)
   *
   * The language for which the font features are selected.
   *
   * Since: 4.10
   */
  properties[PROP_LANGUAGE] =
      g_param_spec_boxed ("language", NULL, NULL,
                          PANGO_TYPE_LANGUAGE,
                          G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GtkFontDialog:fontmap: (attributes org.gtk.Property.get=gtk_font_dialog_get_fontmap org.gtk.Property.set=gtk_font_dialog_set_fontmap)
   *
   * Sets a custom font map to select fonts from.
   *
   * A custom font map can be used to present application-specific
   * fonts instead of or in addition to the normal system fonts.
   *
   * Since: 4.10
   */
  properties[PROP_FONTMAP] =
      g_param_spec_object ("fontmap", NULL, NULL,
                           PANGO_TYPE_FONT_MAP,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);
}

/* }}} */
/* {{{ Constructor */

/**
 * gtk_font_dialog_new:
 *
 * Creates a new `GtkFontDialog` object.
 *
 * Returns: the new `GtkFontDialog`
 *
 * Since: 4.10
 */
GtkFontDialog *
gtk_font_dialog_new (void)
{
  return g_object_new (GTK_TYPE_FONT_DIALOG, NULL);
}

/* }}} */
/* {{{ Getters and setters */

/**
 * gtk_font_dialog_get_title:
 * @self: a `GtkFontDialog`
 *
 * Returns the title that will be shown on the
 * font chooser dialog.
 *
 * Returns: the title
 *
 * Since: 4.10
 */
const char *
gtk_font_dialog_get_title (GtkFontDialog *self)
{
  g_return_val_if_fail (GTK_IS_FONT_DIALOG (self), NULL);

  return self->title;
}

/**
 * gtk_font_dialog_set_title:
 * @self: a `GtkFontDialog`
 * @title: the new title
 *
 * Sets the title that will be shown on the
 * font chooser dialog.
 *
 * Since: 4.10
 */
void
gtk_font_dialog_set_title (GtkFontDialog *self,
                           const char    *title)
{
  char *new_title;

  g_return_if_fail (GTK_IS_FONT_DIALOG (self));
  g_return_if_fail (title != NULL);

  if (g_str_equal (self->title, title))
    return;

  new_title = g_strdup (title);
  g_free (self->title);
  self->title = new_title;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TITLE]);
}

/**
 * gtk_font_dialog_get_modal:
 * @self: a `GtkFontDialog`
 *
 * Returns whether the font chooser dialog
 * blocks interaction with the parent window
 * while it is presented.
 *
 * Returns: `TRUE` if the font chooser dialog is modal
 *
 * Since: 4.10
 */
gboolean
gtk_font_dialog_get_modal (GtkFontDialog *self)
{
  g_return_val_if_fail (GTK_IS_FONT_DIALOG (self), TRUE);

  return self->modal;
}

/**
 * gtk_font_dialog_set_modal:
 * @self: a `GtkFontDialog`
 * @modal: the new value
 *
 * Sets whether the font chooser dialog
 * blocks interaction with the parent window
 * while it is presented.
 *
 * Since: 4.10
 */
void
gtk_font_dialog_set_modal (GtkFontDialog *self,
                           gboolean       modal)
{
  g_return_if_fail (GTK_IS_FONT_DIALOG (self));

  if (self->modal == modal)
    return;

  self->modal = modal;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODAL]);
}

/**
 * gtk_font_dialog_get_level:
 * @self: a `GtkFontDialog`
 *
 * Returns the level of granularity for selecting fonts.
 *
 * Returns: the level of granularity
 *
 * Since: 4.10
 */
GtkFontChooserLevel
gtk_font_dialog_get_level (GtkFontDialog *self)
{
  g_return_val_if_fail (GTK_IS_FONT_DIALOG (self), DEFAULT_LEVEL);

  return self->level;
}

/**
 * gtk_font_dialog_set_level:
 * @self: a `GtkFontDialog`
 * @level: the new value
 *
 * Sets the level of granularity for selecting fonts.
 *
 * Since: 4.10
 */
void
gtk_font_dialog_set_level (GtkFontDialog       *self,
                           GtkFontChooserLevel  level)
{
  g_return_if_fail (GTK_IS_FONT_DIALOG (self));

  if (self->level == level)
    return;

  self->level = level;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_LEVEL]);
}

/**
 * gtk_font_dialog_get_language:
 * @self: a `GtkFontDialog`
 *
 * Returns the language for which font features are applied.
 *
 * Returns: (nullable): the language for font features
 *
 * Since: 4.10
 */
PangoLanguage *
gtk_font_dialog_get_language (GtkFontDialog *self)
{
  g_return_val_if_fail (GTK_IS_FONT_DIALOG (self), NULL);

  return self->language;
}

/**
 * gtk_font_dialog_set_language:
 * @self: a `GtkFontDialog`
 * @language: the language for font features
 *
 * Sets the language for which font features are applied.
 *
 * Since: 4.10
 */
void
gtk_font_dialog_set_language (GtkFontDialog *self,
                              PangoLanguage *language)
{
  g_return_if_fail (GTK_IS_FONT_DIALOG (self));

  if (self->language == language)
    return;

  self->language = language;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_LANGUAGE]);
}

/**
 * gtk_font_dialog_get_fontmap
 * @self: a `GtkFontDialog`
 *
 * Returns the fontmap from which fonts are selected,
 * or `NULL` for the default fontmap.
 *
 * Returns: (nullable) (transfer none): the fontmap
 *
 * Since: 4.10
 */
PangoFontMap *
gtk_font_dialog_get_fontmap (GtkFontDialog *self)
{
  g_return_val_if_fail (GTK_IS_FONT_DIALOG (self), NULL);

  return self->fontmap;
}

/**
 * gtk_font_dialog_set_fontmap:
 * @self: a `GtkFontDialog`
 * @fontmap: (nullable): the fontmap
 *
 * Sets the fontmap from which fonts are selected.
 *
 * If @fontmap is `NULL`, the default fontmap is used.
 *
 * Since: 4.10
 */
void
gtk_font_dialog_set_fontmap (GtkFontDialog *self,
                             PangoFontMap  *fontmap)
{
  g_return_if_fail (GTK_IS_FONT_DIALOG (self));

  if (g_set_object (&self->fontmap, fontmap))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FONTMAP]);
}

/**
 * gtk_font_dialog_set_filter:
 * @self: a `GtkFontDialog`
 * @filter: (nullable): a `GtkFontFilterFunc`
 * @user_data: (closure): data to pass to @filter
 * @destroy: function to call to free @data when it is no longer needed
 *
 * Adds a filter function that decides which fonts to display
 * in the font chooser dialog.
 *
 * Since: 4.10
 */
void
gtk_font_dialog_set_filter (GtkFontDialog     *self,
                            GtkFontFilterFunc  filter,
                            gpointer           filter_data,
                            GDestroyNotify     filter_data_destroy)
{
  g_return_if_fail (GTK_IS_FONT_DIALOG (self));

  g_clear_pointer (&self->filter_data, self->filter_data_destroy);

  self->filter = filter;
  self->filter_data = filter_data;
  self->filter_data_destroy = filter_data_destroy;
}

/* }}} */
/* {{{ Async API */

static void response_cb (GTask *task,
                         int    response);

static void
cancelled_cb (GCancellable *cancellable,
              GTask        *task)
{
  response_cb (task, GTK_RESPONSE_CANCEL);
}

typedef struct
{
  PangoFontDescription *font_desc;
  char *font_features;

} FontResult;

static void
response_cb (GTask *task,
             int    response)
{
  GCancellable *cancellable;

  cancellable = g_task_get_cancellable (task);

  if (cancellable)
    g_signal_handlers_disconnect_by_func (cancellable, cancelled_cb, task);

  if (response == GTK_RESPONSE_OK)
    {
      GtkFontChooserDialog *window;
      FontResult font_result;

      window = GTK_FONT_CHOOSER_DIALOG (g_task_get_task_data (task));

      font_result.font_desc = gtk_font_chooser_get_font_desc (GTK_FONT_CHOOSER (window));
      font_result.font_features = gtk_font_chooser_get_font_features (GTK_FONT_CHOOSER (window));

      g_task_return_pointer (task, &font_result, NULL);
    }
  else
    g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_CANCELLED, "Cancelled");

  g_object_unref (task);
}

static void
dialog_response (GtkDialog *dialog,
                 int        response,
                 GTask     *task)
{
  response_cb (task, response);
}

/**
 * gtk_font_dialog_choose_font:
 * @self: a `GtkFontDialog`
 * @parent: (nullable): the parent `GtkWindow`
 * @initial_font: (nullable): the font to select initially
 * @cancellable: (nullable): a `GCancellable` to cancel the operation
 * @callback: (scope async): a callback to call when the operation is complete
 * @user_data: (closure callback): data to pass to @callback
 *
 * This function initiates a font selection operation by
 * presenting a font chooser dialog to the user.
 *
 * The @callback will be called when the dialog is dismissed.
 * It should call [function@Gtk.FontDialog.choose_font_finish]
 * to obtain the result.
 *
 * Since: 4.10
 */
void
gtk_font_dialog_choose_font (GtkFontDialog        *self,
                             GtkWindow            *parent,
                             PangoFontDescription *initial_font,
                             GCancellable         *cancellable,
                             GAsyncReadyCallback   callback,
                             gpointer              user_data)
{
  GtkFontChooserDialog *window;
  GTask *task;

  g_return_if_fail (GTK_IS_FONT_DIALOG (self));

  window = GTK_FONT_CHOOSER_DIALOG (gtk_font_chooser_dialog_new (self->title, parent));
  gtk_font_chooser_set_level (GTK_FONT_CHOOSER (window), self->level);
  if (self->language)
    gtk_font_chooser_set_language (GTK_FONT_CHOOSER (window), pango_language_to_string (self->language));
  if (self->fontmap)
    gtk_font_chooser_set_font_map (GTK_FONT_CHOOSER (window), self->fontmap);
  if (initial_font)
    gtk_font_chooser_set_font_desc (GTK_FONT_CHOOSER (window), initial_font);
  if (self->filter)
    gtk_font_chooser_set_filter_func (GTK_FONT_CHOOSER (window),
                                      self->filter,
                                      self->filter_data,
                                      self->filter_data_destroy);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, gtk_font_dialog_choose_font);
  g_task_set_task_data (task, window, (GDestroyNotify) gtk_window_destroy);

  if (cancellable)
    g_signal_connect (cancellable, "cancelled", G_CALLBACK (cancelled_cb), task);

  g_signal_connect (window, "response", G_CALLBACK (dialog_response), task);

  gtk_window_present (GTK_WINDOW (window));
}

/**
 * gtk_font_dialog_choose_font_finish:
 * @self: a `GtkFontDialog`
 * @result: a `GAsyncResult`
 * @font_desc: (out caller-allocates): return location for font description
 * @font_features: (out caller-allocates): return location for font features
 * @error: return location for an error
 *
 * Finishes the [function@Gtk.FontDialog.choose_font] call and
 * returns the resulting font description and font features.
 *
 * Returns: `TRUE` if a font was selected. Otherwise,
 *   `FALSE` is returned and @error is set
 *
 * Since: 4.10
 */
gboolean
gtk_font_dialog_choose_font_finish (GtkFontDialog         *self,
                                    GAsyncResult          *result,
                                    PangoFontDescription **font_desc,
                                    char                 **font_features,
                                    GError               **error)
{
  FontResult *font_result;

  font_result = g_task_propagate_pointer (G_TASK (result), error);
  if (font_result)
    {
      *font_desc = font_result->font_desc;
      *font_features = font_result->font_features;
      return TRUE;
    }

  return FALSE;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */