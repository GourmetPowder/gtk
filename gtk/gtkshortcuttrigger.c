/*
 * Copyright © 2018 Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

/**
 * SECTION:gtkshortcuttrigger
 * @Title: GtkShortcutTrigger
 * @Short_description: Triggers to track if shortcuts should be activated
 * @See_also: #GtkShortcut
 *
 * #GtkShortcutTrigger is the object used to track if a #GtkShortcut should be
 * activated. For this purpose, gtk_shortcut_trigger_trigger() can be called
 * on a #GdkEvent.
 *
 * #GtkShortcutTriggers contain functions that allow easy presentation to end
 * users as well as being printed for debugging.
 *
 * All #GtkShortcutTriggers are immutable, you can only specify their properties
 * during construction. If you want to change a trigger, you have to replace it
 * with a new one.
 */

#include "config.h"

#include "gtkshortcuttrigger.h"

#include "gtkaccelgroup.h"

typedef struct _GtkShortcutTriggerClass GtkShortcutTriggerClass;

#define GTK_IS_SHORTCUT_TRIGGER_TYPE(trigger,type) (GTK_IS_SHORTCUT_TRIGGER (trigger) && (trigger)->trigger_class->trigger_type == (type))

struct _GtkShortcutTrigger
{
  const GtkShortcutTriggerClass *trigger_class;

  gatomicrefcount ref_count;
};

struct _GtkShortcutTriggerClass
{
  GtkShortcutTriggerType trigger_type;
  gsize struct_size;
  const char *type_name;

  void            (* finalize)    (GtkShortcutTrigger  *trigger);
  gboolean        (* trigger)     (GtkShortcutTrigger  *trigger,
                                   GdkEvent            *event);
  void            (* print)       (GtkShortcutTrigger  *trigger,
                                   GString             *string);
};

G_DEFINE_BOXED_TYPE (GtkShortcutTrigger, gtk_shortcut_trigger,
                     gtk_shortcut_trigger_ref,
                     gtk_shortcut_trigger_unref)

static void
gtk_shortcut_trigger_finalize (GtkShortcutTrigger *self)
{
  self->trigger_class->finalize (self);

  g_free (self);
}

/*< private >
 * gtk_shortcut_trigger_new:
 * @trigger_class: class structure for this trigger
 *
 * Returns: (transfer full): the newly created #GtkShortcutTrigger
 */
static GtkShortcutTrigger *
gtk_shortcut_trigger_new (const GtkShortcutTriggerClass *trigger_class)
{
  GtkShortcutTrigger *self;

  g_return_val_if_fail (trigger_class != NULL, NULL);

  self = g_malloc0 (trigger_class->struct_size);
  g_atomic_ref_count_init (&self->ref_count);

  self->trigger_class = trigger_class;

  return self;
}

/**
 * gtk_shortcut_trigger_ref:
 * @self: a #GtkShortcutTrigger
 *
 * Acquires a reference on the given #GtkShortcutTrigger.
 *
 * Returns: (transfer full): the #GtkShortcutTrigger with
 *    an additional reference
 */
GtkShortcutTrigger *
gtk_shortcut_trigger_ref (GtkShortcutTrigger *self)
{
  g_return_val_if_fail (GTK_IS_SHORTCUT_TRIGGER (self), NULL);

  g_atomic_ref_count_inc (&self->ref_count);

  return self;
}

/**
 * gtk_shortcut_trigger_unref:
 * @self: (transfer full): a #GtkShortcutTrigger
 *
 * Releases a reference on the given #GtkShortcutTrigger.
 *
 * If the reference was the last, the resources associated
 * to the trigger are freed.
 */
void
gtk_shortcut_trigger_unref (GtkShortcutTrigger *self)
{
  g_return_if_fail (GTK_IS_SHORTCUT_TRIGGER (self));

  if (g_atomic_ref_count_dec (&self->ref_count))
    gtk_shortcut_trigger_finalize (self);
}

/**
 * gtk_shortcut_trigger_get_trigger_type:
 * @self: a #GtkShortcutTrigger
 *
 * Returns the type of the @trigger.
 *
 * Returns: the type of the #GtkShortcutTrigger
 */
GtkShortcutTriggerType
gtk_shortcut_trigger_get_trigger_type (GtkShortcutTrigger *self)
{
  g_return_val_if_fail (GTK_IS_SHORTCUT_TRIGGER (self), GTK_SHORTCUT_TRIGGER_NEVER);

  return self->trigger_class->trigger_type;
}

/**
 * gtk_shortcut_trigger_trigger:
 * @self: a #GtkShortcutTrigger
 * @event: the event to check
 *
 * Checks if the given @event triggers @self. If so,
 * returns %TRUE.
 *
 * Returns: %TRUE if this event triggered the trigger
 **/
gboolean
gtk_shortcut_trigger_trigger (GtkShortcutTrigger *self,
                              GdkEvent           *event)
{
  g_return_val_if_fail (GTK_IS_SHORTCUT_TRIGGER (self), FALSE);

  return self->trigger_class->trigger (self, event);
}

/**
 * gtk_shortcut_trigger_to_string:
 * @self: a #GtkShortcutTrigger
 *
 * Prints the given trigger into a human-readable string.
 * This is a small wrapper around gdk_content_formats_print()
 * to help when debugging.
 *
 * Returns: (transfer full): a new string
 **/
char *
gtk_shortcut_trigger_to_string (GtkShortcutTrigger *self)
{
  GString *string;

  g_return_val_if_fail (self != NULL, NULL);

  string = g_string_new (NULL);
  gtk_shortcut_trigger_print (self, string);

  return g_string_free (string, FALSE);
}

/**
 * gtk_shortcut_trigger_print:
 * @self: a #GtkShortcutTrigger
 * @string: a #GString to print into
 *
 * Prints the given trigger into a string for the developer.
 * This is meant for debugging and logging.
 *
 * The form of the representation may change at any time
 * and is not guaranteed to stay identical.
 **/
void
gtk_shortcut_trigger_print (GtkShortcutTrigger *self,
                            GString            *string)
{
  g_return_if_fail (GTK_IS_SHORTCUT_TRIGGER (self));
  g_return_if_fail (string != NULL);

  return self->trigger_class->print (self, string);
}

/*** GTK_SHORTCUT_TRIGGER_NEVER ***/

typedef struct _GtkNeverTrigger GtkNeverTrigger;

struct _GtkNeverTrigger
{
  GtkShortcutTrigger trigger;

  guint never;
  GdkModifierType modifiers;
};

static void
gtk_never_trigger_finalize (GtkShortcutTrigger *trigger)
{
  g_assert_not_reached ();
}

static gboolean
gtk_never_trigger_trigger (GtkShortcutTrigger *trigger,
                           GdkEvent           *event)
{
  return FALSE;
}

static void
gtk_never_trigger_print (GtkShortcutTrigger *trigger,
                         GString            *string)
{
  g_string_append (string, "<never>");
}

static const GtkShortcutTriggerClass GTK_NEVER_TRIGGER_CLASS = {
  GTK_SHORTCUT_TRIGGER_NEVER,
  sizeof (GtkNeverTrigger),
  "GtkNeverTrigger",
  gtk_never_trigger_finalize,
  gtk_never_trigger_trigger,
  gtk_never_trigger_print
};

static GtkNeverTrigger never = { { &GTK_NEVER_TRIGGER_CLASS, 1 } };

/**
 * gtk_never_trigger_get:
 *
 * Gets the never trigger. This is a singleton for a trigger
 * that never triggers. Use this trigger instead of %NULL
 * because it implements all virtual functions.
 *
 * Returns: (transfer none): The never trigger
 */
GtkShortcutTrigger *
gtk_never_trigger_get (void)
{
  return &never.trigger;
}

/*** GTK_KEYVAL_TRIGGER ***/

typedef struct _GtkKeyvalTrigger GtkKeyvalTrigger;

struct _GtkKeyvalTrigger
{
  GtkShortcutTrigger trigger;

  guint keyval;
  GdkModifierType modifiers;
};

static void
gtk_keyval_trigger_finalize (GtkShortcutTrigger *trigger)
{
}

static gboolean
gtk_keyval_trigger_trigger (GtkShortcutTrigger *trigger,
                            GdkEvent           *event)
{
  GtkKeyvalTrigger *self = (GtkKeyvalTrigger *) trigger;
  GdkModifierType modifiers;
  guint keyval;

  if (gdk_event_get_event_type (event) != GDK_KEY_PRESS)
    return FALSE;

  /* XXX: This needs to deal with groups */
  modifiers = gdk_event_get_modifier_state (event);
  keyval = gdk_key_event_get_keyval (event);

  if (keyval == GDK_KEY_ISO_Left_Tab)
    keyval = GDK_KEY_Tab;
  else
    keyval = gdk_keyval_to_lower (keyval);

  return keyval == self->keyval && modifiers == self->modifiers;
}

static void
gtk_keyval_trigger_print (GtkShortcutTrigger *trigger,
                          GString            *string)
{
  GtkKeyvalTrigger *self = (GtkKeyvalTrigger *) trigger;
  char *accelerator_name;

  accelerator_name = gtk_accelerator_name (self->keyval, self->modifiers);
  g_string_append (string, accelerator_name);
  g_free (accelerator_name);
}

static const GtkShortcutTriggerClass GTK_KEYVAL_TRIGGER_CLASS = {
  GTK_SHORTCUT_TRIGGER_KEYVAL,
  sizeof (GtkKeyvalTrigger),
  "GtkKeyvalTrigger",
  gtk_keyval_trigger_finalize,
  gtk_keyval_trigger_trigger,
  gtk_keyval_trigger_print
};

/**
 * gtk_keyval_trigger_new:
 * @keyval: The keyval to trigger for
 * @modifiers: the modifiers that need to be present
 *
 * Creates a #GtkShortcutTrigger that will trigger whenever
 * the key with the given @keyval and @modifiers is pressed.
 *
 * Returns: A new #GtkShortcutTrigger
 */
GtkShortcutTrigger *
gtk_keyval_trigger_new (guint           keyval,
                        GdkModifierType modifiers)
{
  GtkKeyvalTrigger *self;

  self = (GtkKeyvalTrigger *) gtk_shortcut_trigger_new (&GTK_KEYVAL_TRIGGER_CLASS);

  /* We store keyvals as lower key */
  if (keyval == GDK_KEY_ISO_Left_Tab)
    self->keyval = GDK_KEY_Tab;
  else
    self->keyval = gdk_keyval_to_lower (keyval);
  self->modifiers = modifiers;

  return &self->trigger;
}

/**
 * gtk_keyval_trigger_get_modifiers:
 * @self: a keyval #GtkShortcutTrigger
 *
 * Gets the modifiers that must be present to succeed
 * triggering @self.
 *
 * Returns: the modifiers
 **/
GdkModifierType
gtk_keyval_trigger_get_modifiers (GtkShortcutTrigger *self)
{
  GtkKeyvalTrigger *trigger = (GtkKeyvalTrigger *) self;

  g_return_val_if_fail (GTK_IS_SHORTCUT_TRIGGER_TYPE (self, GTK_SHORTCUT_TRIGGER_KEYVAL), 0);

  return trigger->modifiers;
}

/**
 * gtk_keyval_trigger_get_keyval:
 * @self: a keyval #GtkShortcutTrigger
 *
 * Gets the keyval that must be pressed to succeed
 * triggering @self.
 *
 * Returns: the keyval
 **/
guint
gtk_keyval_trigger_get_keyval (GtkShortcutTrigger *self)
{
  GtkKeyvalTrigger *trigger = (GtkKeyvalTrigger *) self;

  g_return_val_if_fail (GTK_IS_SHORTCUT_TRIGGER_TYPE (self, GTK_SHORTCUT_TRIGGER_KEYVAL), 0);

  return trigger->keyval;
}

/*** GTK_ALTERNATIVE_TRIGGER ***/

typedef struct _GtkAlternativeTrigger GtkAlternativeTrigger;

struct _GtkAlternativeTrigger
{
  GtkShortcutTrigger trigger;

  GtkShortcutTrigger *first;
  GtkShortcutTrigger *second;
};

static void
gtk_alternative_trigger_finalize (GtkShortcutTrigger *trigger)
{
  GtkAlternativeTrigger *self = (GtkAlternativeTrigger *) trigger;

  gtk_shortcut_trigger_unref (self->first);
  gtk_shortcut_trigger_unref (self->second);
}

static gboolean
gtk_alternative_trigger_trigger (GtkShortcutTrigger *trigger,
                                 GdkEvent           *event)
{
  GtkAlternativeTrigger *self = (GtkAlternativeTrigger *) trigger;

  if (gtk_shortcut_trigger_trigger (self->first, event))
    return TRUE;

  if (gtk_shortcut_trigger_trigger (self->second, event))
    return TRUE;

  return FALSE;
}

static void
gtk_alternative_trigger_print (GtkShortcutTrigger *trigger,
                               GString            *string)
                  
{
  GtkAlternativeTrigger *self = (GtkAlternativeTrigger *) trigger;

  gtk_shortcut_trigger_print (self->first, string);
  g_string_append (string, ", ");
  gtk_shortcut_trigger_print (self->second, string);
}

static const GtkShortcutTriggerClass GTK_ALTERNATIVE_TRIGGER_CLASS = {
  GTK_SHORTCUT_TRIGGER_ALTERNATIVE,
  sizeof (GtkAlternativeTrigger),
  "GtkAlternativeTrigger",
  gtk_alternative_trigger_finalize,
  gtk_alternative_trigger_trigger,
  gtk_alternative_trigger_print
};

/**
 * gtk_alternative_trigger_new:
 * @first: (transfer full): The first trigger that may trigger
 * @second: (transfer full): The second trigger that may trigger
 *
 * Creates a #GtkShortcutTrigger that will trigger whenever
 * either of the two given triggers gets triggered.
 *
 * Note that nesting is allowed, so if you want more than two
 * alternative, create a new alternative trigger for each option.
 *
 * Returns: a new #GtkShortcutTrigger
 */
GtkShortcutTrigger *
gtk_alternative_trigger_new (GtkShortcutTrigger *first,
                             GtkShortcutTrigger *second)
{
  GtkAlternativeTrigger *self;

  g_return_val_if_fail (GTK_IS_SHORTCUT_TRIGGER (first), NULL);
  g_return_val_if_fail (GTK_IS_SHORTCUT_TRIGGER (second), NULL);

  self = (GtkAlternativeTrigger *) gtk_shortcut_trigger_new (&GTK_ALTERNATIVE_TRIGGER_CLASS);

  self->first = first;
  self->second = second;

  return &self->trigger;
}

/**
 * gtk_alternative_trigger_get_first:
 * @self: an alternative #GtkShortcutTrigger
 *
 * Gets the first of the two alternative triggers that may
 * trigger @self. gtk_alternative_trigger_get_second() will
 * return the other one.
 *
 * Returns: (transfer none): the first alternative trigger
 **/
GtkShortcutTrigger *
gtk_alternative_trigger_get_first (GtkShortcutTrigger *self)
{
  GtkAlternativeTrigger *trigger = (GtkAlternativeTrigger *) self;

  g_return_val_if_fail (GTK_IS_SHORTCUT_TRIGGER_TYPE (self, GTK_SHORTCUT_TRIGGER_ALTERNATIVE), 0);

  return trigger->first;
}

/**
 * gtk_alternative_trigger_get_second:
 * @self: an alternative #GtkShortcutTrigger
 *
 * Gets the second of the two alternative triggers that may
 * trigger @self. gtk_alternative_trigger_get_first() will
 * return the other one.
 *
 * Returns: (transfer none): the second alternative trigger
 **/
GtkShortcutTrigger *
gtk_alternative_trigger_get_second (GtkShortcutTrigger *self)
{
  GtkAlternativeTrigger *trigger = (GtkAlternativeTrigger *) self;

  g_return_val_if_fail (GTK_IS_SHORTCUT_TRIGGER_TYPE (self, GTK_SHORTCUT_TRIGGER_ALTERNATIVE), 0);

  return trigger->second;
}