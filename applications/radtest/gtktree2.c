/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include "gtktree2.h"
#include "gtktreeitem2.h"

#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtklist.h>

enum {
  SELECTION_CHANGED,
  SELECT_CHILD,
  UNSELECT_CHILD,
  LAST_SIGNAL
};

static void gtk_tree2_class_init      (GtkTree2Class   *klass);
static void gtk_tree2_init            (GtkTree2        *tree);
static void gtk_tree2_destroy         (GtkObject      *object);
static void gtk_tree2_map             (GtkWidget      *widget);
static void gtk_tree2_unmap           (GtkWidget      *widget);
static void gtk_tree2_realize         (GtkWidget      *widget);
static void gtk_tree2_draw            (GtkWidget      *widget,
				      GdkRectangle   *area);
static gint gtk_tree2_expose          (GtkWidget      *widget,
				      GdkEventExpose *event);
static gint gtk_tree2_motion_notify   (GtkWidget      *widget,
				      GdkEventMotion *event);
static gint gtk_tree2_button_press    (GtkWidget      *widget,
				      GdkEventButton *event);
static gint gtk_tree2_button_release  (GtkWidget      *widget,
				      GdkEventButton *event);
static void gtk_tree2_size_request    (GtkWidget      *widget,
				      GtkRequisition *requisition);
static void gtk_tree2_size_allocate   (GtkWidget      *widget,
				      GtkAllocation  *allocation);
static void gtk_tree2_add             (GtkContainer   *container,
				      GtkWidget      *widget);
static void gtk_tree2_forall          (GtkContainer   *container,
				      gboolean	      include_internals,
				      GtkCallback     callback,
				      gpointer        callback_data);

static void gtk_real_tree2_select_child   (GtkTree2       *tree,
					  GtkWidget     *child);
static void gtk_real_tree2_unselect_child (GtkTree2       *tree,
					  GtkWidget     *child);

static GtkType gtk_tree2_child_type  (GtkContainer   *container);

static GtkContainerClass *parent_class = NULL;
static guint tree_signals[LAST_SIGNAL] = { 0 };

GtkType
gtk_tree2_get_type (void)
{
  static GtkType tree_type = 0;
  
  if (!tree_type)
    {
      static const GtkTypeInfo tree_info =
      {
	"GtkTree2",
	sizeof (GtkTree2),
	sizeof (GtkTree2Class),
	(GtkClassInitFunc) gtk_tree2_class_init,
	(GtkObjectInitFunc) gtk_tree2_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      
      tree_type = gtk_type_unique (gtk_container_get_type (), &tree_info);
    }
  
  return tree_type;
}

static void
gtk_tree2_class_init (GtkTree2Class *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;
  
  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;
  container_class = (GtkContainerClass*) class;
  
  parent_class = gtk_type_class (gtk_container_get_type ());
  
  tree_signals[SELECTION_CHANGED] =
    gtk_signal_new ("selection_changed",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GtkTree2Class, selection_changed),
		    gtk_marshal_NONE__NONE,
		    GTK_TYPE_NONE, 0);
  tree_signals[SELECT_CHILD] =
    gtk_signal_new ("select_child",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GtkTree2Class, select_child),
		    gtk_marshal_NONE__POINTER,
		    GTK_TYPE_NONE, 1,
		    GTK_TYPE_WIDGET);
  tree_signals[UNSELECT_CHILD] =
    gtk_signal_new ("unselect_child",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GtkTree2Class, unselect_child),
		    gtk_marshal_NONE__POINTER,
		    GTK_TYPE_NONE, 1,
		    GTK_TYPE_WIDGET);
  
  gtk_object_class_add_signals (object_class, tree_signals, LAST_SIGNAL);
  
  object_class->destroy = gtk_tree2_destroy;
  
  widget_class->map = gtk_tree2_map;
  widget_class->unmap = gtk_tree2_unmap;
  widget_class->realize = gtk_tree2_realize;
  widget_class->draw = gtk_tree2_draw;
  widget_class->expose_event = gtk_tree2_expose;
  widget_class->motion_notify_event = gtk_tree2_motion_notify;
  widget_class->button_press_event = gtk_tree2_button_press;
  widget_class->button_release_event = gtk_tree2_button_release;
  widget_class->size_request = gtk_tree2_size_request;
  widget_class->size_allocate = gtk_tree2_size_allocate;
  
  container_class->add = gtk_tree2_add;
  container_class->remove = 
    (void (*)(GtkContainer *, GtkWidget *)) gtk_tree2_remove_item;
  container_class->forall = gtk_tree2_forall;
  container_class->child_type = gtk_tree2_child_type;
  
  class->selection_changed = NULL;
  class->select_child = gtk_real_tree2_select_child;
  class->unselect_child = gtk_real_tree2_unselect_child;
}

static GtkType
gtk_tree2_child_type (GtkContainer     *container)
{
  return GTK_TYPE_TREE_ITEM2;
}

static void
gtk_tree2_init (GtkTree2 *tree)
{
  tree->children = NULL;
  tree->root_tree = NULL;
  tree->selection = NULL;
  tree->tree_owner = NULL;
  tree->selection_mode = GTK_SELECTION_SINGLE;
  tree->indent_value = 9;
  tree->current_indent = 0;
  tree->level = 0;
  tree->view_mode = GTK_TREE2_VIEW_LINE;
  tree->view_line = 1;
}

GtkWidget*
gtk_tree2_new (void)
{
  return GTK_WIDGET (gtk_type_new (gtk_tree2_get_type ()));
}

void
gtk_tree2_append (GtkTree2   *tree,
		 GtkWidget *tree_item)
{
  g_return_if_fail (tree != NULL);
  g_return_if_fail (GTK_IS_TREE2 (tree));
  g_return_if_fail (tree_item != NULL);
  g_return_if_fail (GTK_IS_TREE_ITEM2 (tree_item));
  
  gtk_tree2_insert (tree, tree_item, -1);
}

void
gtk_tree2_prepend (GtkTree2   *tree,
		  GtkWidget *tree_item)
{
  g_return_if_fail (tree != NULL);
  g_return_if_fail (GTK_IS_TREE2 (tree));
  g_return_if_fail (tree_item != NULL);
  g_return_if_fail (GTK_IS_TREE_ITEM2 (tree_item));
  
  gtk_tree2_insert (tree, tree_item, 0);
}

void
gtk_tree2_insert (GtkTree2   *tree,
		 GtkWidget *tree_item,
		 gint       position)
{
  gint nchildren;
  
  g_return_if_fail (tree != NULL);
  g_return_if_fail (GTK_IS_TREE2 (tree));
  g_return_if_fail (tree_item != NULL);
  g_return_if_fail (GTK_IS_TREE_ITEM2 (tree_item));
  
  nchildren = g_list_length (tree->children);
  
  if ((position < 0) || (position > nchildren))
    position = nchildren;
  
  if (position == nchildren)
    tree->children = g_list_append (tree->children, tree_item);
  else
    tree->children = g_list_insert (tree->children, tree_item, position);
  
  gtk_widget_set_parent (tree_item, GTK_WIDGET (tree));
  
  if (GTK_WIDGET_REALIZED (tree_item->parent))
    gtk_widget_realize (tree_item);

  if (GTK_WIDGET_VISIBLE (tree_item->parent) && GTK_WIDGET_VISIBLE (tree_item))
    {
      if (GTK_WIDGET_MAPPED (tree_item->parent))
	gtk_widget_map (tree_item);

      gtk_widget_queue_resize (tree_item);
    }
}

static void
gtk_tree2_add (GtkContainer *container,
	      GtkWidget    *child)
{
  GtkTree2 *tree;
  
  g_return_if_fail (container != NULL);
  g_return_if_fail (GTK_IS_TREE2 (container));
  g_return_if_fail (GTK_IS_TREE_ITEM2 (child));
  
  tree = GTK_TREE2 (container);
  
  tree->children = g_list_append (tree->children, child);
  
  gtk_widget_set_parent (child, GTK_WIDGET (container));
  
  if (GTK_WIDGET_REALIZED (child->parent))
    gtk_widget_realize (child);

  if (GTK_WIDGET_VISIBLE (child->parent) && GTK_WIDGET_VISIBLE (child))
    {
      if (GTK_WIDGET_MAPPED (child->parent))
	gtk_widget_map (child);

      gtk_widget_queue_resize (child);
    }
  
  if (!tree->selection && (tree->selection_mode == GTK_SELECTION_BROWSE))
    gtk_tree2_select_child (tree, child);
}

static gint
gtk_tree2_button_press (GtkWidget      *widget,
		       GdkEventButton *event)
{
  GtkTree2 *tree;
  GtkWidget *item;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_TREE2 (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  tree = GTK_TREE2 (widget);
  item = gtk_get_event_widget ((GdkEvent*) event);
  
  while (item && !GTK_IS_TREE_ITEM2 (item))
    item = item->parent;
  
  if (!item || (item->parent != widget))
    return FALSE;
  
  switch(event->button) 
    {
    case 1:
      gtk_tree2_select_child (tree, item);
      break;
    case 2:
      if(GTK_TREE_ITEM2(item)->subtree) gtk_tree_item2_expand(GTK_TREE_ITEM2(item));
      break;
    case 3:
      if(GTK_TREE_ITEM2(item)->subtree) gtk_tree_item2_collapse(GTK_TREE_ITEM2(item));
      break;
    }
  
  return TRUE;
}

static gint
gtk_tree2_button_release (GtkWidget      *widget,
			 GdkEventButton *event)
{
  GtkTree2 *tree;
  GtkWidget *item;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_TREE2 (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  tree = GTK_TREE2 (widget);
  item = gtk_get_event_widget ((GdkEvent*) event);
  
  return TRUE;
}

gint
gtk_tree2_child_position (GtkTree2   *tree,
			 GtkWidget *child)
{
  GList *children;
  gint pos;
  
  
  g_return_val_if_fail (tree != NULL, -1);
  g_return_val_if_fail (GTK_IS_TREE2 (tree), -1);
  g_return_val_if_fail (child != NULL, -1);
  
  pos = 0;
  children = tree->children;
  
  while (children)
    {
      if (child == GTK_WIDGET (children->data)) 
	return pos;
      
      pos += 1;
      children = children->next;
    }
  
  
  return -1;
}

void
gtk_tree2_clear_items (GtkTree2 *tree,
		      gint     start,
		      gint     end)
{
  GtkWidget *widget;
  GList *clear_list;
  GList *tmp_list;
  guint nchildren;
  guint index;
  
  g_return_if_fail (tree != NULL);
  g_return_if_fail (GTK_IS_TREE2 (tree));
  
  nchildren = g_list_length (tree->children);
  
  if (nchildren > 0)
    {
      if ((end < 0) || (end > nchildren))
	end = nchildren;
      
      if (start >= end)
	return;
      
      tmp_list = g_list_nth (tree->children, start);
      clear_list = NULL;
      index = start;
      while (tmp_list && index <= end)
	{
	  widget = tmp_list->data;
	  tmp_list = tmp_list->next;
	  index++;
	  
	  clear_list = g_list_prepend (clear_list, widget);
	}
      
      gtk_tree2_remove_items (tree, clear_list);
    }
}

static void
gtk_tree2_destroy (GtkObject *object)
{
  GtkTree2 *tree;
  GtkWidget *child;
  GList *children;
  
  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_TREE2 (object));
  
  tree = GTK_TREE2 (object);
  
  children = tree->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      gtk_widget_ref (child);
      gtk_widget_unparent (child);
      gtk_widget_destroy (child);
      gtk_widget_unref (child);
    }
  
  g_list_free (tree->children);
  tree->children = NULL;
  
  if (tree->root_tree == tree)
    {
      GList *node;
      for (node = tree->selection; node; node = node->next)
	gtk_widget_unref ((GtkWidget *)node->data);
      g_list_free (tree->selection);
      tree->selection = NULL;
    }
  
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gtk_tree2_draw (GtkWidget    *widget,
	       GdkRectangle *area)
{
  GtkTree2 *tree;
  GtkWidget *subtree;
  GtkWidget *child;
  GdkRectangle child_area;
  GList *children;
  
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_TREE2 (widget));
  g_return_if_fail (area != NULL);
  
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      tree = GTK_TREE2 (widget);
      
      children = tree->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;
	  
	  if (gtk_widget_intersect (child, area, &child_area))
	    gtk_widget_draw (child, &child_area);
	  
	  if((subtree = GTK_TREE_ITEM2(child)->subtree) &&
	     GTK_WIDGET_VISIBLE(subtree) &&
	     gtk_widget_intersect (subtree, area, &child_area))
	    gtk_widget_draw (subtree, &child_area);
	}
    }
  
}

static gint
gtk_tree2_expose (GtkWidget      *widget,
		 GdkEventExpose *event)
{
  GtkTree2 *tree;
  GtkWidget *child;
  GdkEventExpose child_event;
  GList *children;
  
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_TREE2 (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      tree = GTK_TREE2 (widget);
      
      child_event = *event;
      
      children = tree->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;
	  
	  if (GTK_WIDGET_NO_WINDOW (child) &&
	      gtk_widget_intersect (child, &event->area, &child_event.area))
	    gtk_widget_event (child, (GdkEvent*) &child_event);
	}
    }
  
  
  return FALSE;
}

static void
gtk_tree2_forall (GtkContainer *container,
		 gboolean      include_internals,
		 GtkCallback   callback,
		 gpointer      callback_data)
{
  GtkTree2 *tree;
  GtkWidget *child;
  GList *children;
  
  
  g_return_if_fail (container != NULL);
  g_return_if_fail (GTK_IS_TREE2 (container));
  g_return_if_fail (callback != NULL);
  
  tree = GTK_TREE2 (container);
  children = tree->children;
  
  while (children)
    {
      child = children->data;
      children = children->next;

      (* callback) (child, callback_data);

      if (include_internals && GTK_TREE_ITEM2(child)->subtree)
      (* callback) (GTK_TREE_ITEM2(child)->subtree, callback_data);
    }
}

static void
gtk_tree2_map (GtkWidget *widget)
{
  GtkTree2 *tree;
  GtkWidget *child;
  GList *children;
  
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_TREE2 (widget));
  
  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
  tree = GTK_TREE2 (widget);
  
  if(GTK_IS_TREE2(widget->parent)) 
    {
      /* set root tree for this tree */
      tree->root_tree = GTK_TREE2(widget->parent)->root_tree;
      
      tree->level = GTK_TREE2(GTK_WIDGET(tree)->parent)->level+1;
      tree->indent_value = GTK_TREE2(GTK_WIDGET(tree)->parent)->indent_value;
      tree->current_indent = GTK_TREE2(GTK_WIDGET(tree)->parent)->current_indent + 
	tree->indent_value;
      tree->view_mode = GTK_TREE2(GTK_WIDGET(tree)->parent)->view_mode;
      tree->view_line = GTK_TREE2(GTK_WIDGET(tree)->parent)->view_line;
    } 
  else
    tree->root_tree = tree;
  
  children = tree->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (GTK_WIDGET_VISIBLE (child) &&
	  !GTK_WIDGET_MAPPED (child))
	gtk_widget_map (child);
      
      if (GTK_TREE_ITEM2 (child)->subtree)
	{
	  child = GTK_WIDGET (GTK_TREE_ITEM2 (child)->subtree);
	  
	  if (GTK_WIDGET_VISIBLE (child) && !GTK_WIDGET_MAPPED (child))
	    gtk_widget_map (child);
	}
    }

  gdk_window_show (widget->window);
}

static gint
gtk_tree2_motion_notify (GtkWidget      *widget,
			GdkEventMotion *event)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_TREE2 (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
#ifdef TREE_DEBUG
  g_message("gtk_tree2_motion_notify\n");
#endif /* TREE_DEBUG */
  
  return FALSE;
}

static void
gtk_tree2_realize (GtkWidget *widget)
{
  GdkWindowAttr attributes;
  gint attributes_mask;
  
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_TREE2 (widget));
  
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;
  
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  
  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, widget);
  
  widget->style = gtk_style_attach (widget->style, widget->window);
  gdk_window_set_background (widget->window, 
			     &widget->style->base[GTK_STATE_NORMAL]);
}

void
gtk_tree2_remove_item (GtkTree2      *container,
		      GtkWidget    *widget)
{
  GList *item_list;
  
  g_return_if_fail (container != NULL);
  g_return_if_fail (GTK_IS_TREE2 (container));
  g_return_if_fail (widget != NULL);
  g_return_if_fail (container == GTK_TREE2 (widget->parent));
  
  item_list = g_list_append (NULL, widget);
  
  gtk_tree2_remove_items (GTK_TREE2 (container), item_list);
  
  g_list_free (item_list);
}

/* used by gtk_tree2_remove_items to make the function independant of
   order in list of items to remove.
   Sort item bu depth in tree */
static gint 
gtk_tree2_sort_item_by_depth(GtkWidget* a, GtkWidget* b)
{
  if((GTK_TREE2(a->parent)->level) < (GTK_TREE2(b->parent)->level))
    return 1;
  if((GTK_TREE2(a->parent)->level) > (GTK_TREE2(b->parent)->level))
    return -1;
  
  return 0;
}

void
gtk_tree2_remove_items (GtkTree2 *tree,
		       GList   *items)
{
  GtkWidget *widget;
  GList *selected_widgets;
  GList *tmp_list;
  GList *sorted_list;
  GtkTree2 *real_tree;
  GtkTree2 *root_tree;
  
  g_return_if_fail (tree != NULL);
  g_return_if_fail (GTK_IS_TREE2 (tree));
  
#ifdef TREE_DEBUG
  g_message("+ gtk_tree2_remove_items [ tree %#x items list %#x ]\n", (int)tree, (int)items);
#endif /* TREE_DEBUG */
  
  /* We may not yet be mapped, so we actively have to find our
   * root tree
   */
  if (tree->root_tree)
    root_tree = tree->root_tree;
  else
    {
      GtkWidget *tmp = GTK_WIDGET (tree);
      while (tmp->parent && GTK_IS_TREE2 (tmp->parent))
	tmp = tmp->parent;
      
      root_tree = GTK_TREE2 (tmp);
    }
  
  tmp_list = items;
  selected_widgets = NULL;
  sorted_list = NULL;
  widget = NULL;
  
#ifdef TREE_DEBUG
  g_message("* sort list by depth\n");
#endif /* TREE_DEBUG */
  
  while (tmp_list)
    {
      
#ifdef TREE_DEBUG
      g_message ("* item [%#x] depth [%d]\n", 
		 (int)tmp_list->data,
		 (int)GTK_TREE2(GTK_WIDGET(tmp_list->data)->parent)->level);
#endif /* TREE_DEBUG */
      
      sorted_list = g_list_insert_sorted(sorted_list,
					 tmp_list->data,
					 (GCompareFunc)gtk_tree2_sort_item_by_depth);
      tmp_list = g_list_next(tmp_list);
    }
  
#ifdef TREE_DEBUG
  /* print sorted list */
  g_message("* sorted list result\n");
  tmp_list = sorted_list;
  while(tmp_list)
    {
      g_message("* item [%#x] depth [%d]\n", 
		(int)tmp_list->data,
		(int)GTK_TREE2(GTK_WIDGET(tmp_list->data)->parent)->level);
      tmp_list = g_list_next(tmp_list);
    }
#endif /* TREE_DEBUG */
  
#ifdef TREE_DEBUG
  g_message("* scan sorted list\n");
#endif /* TREE_DEBUG */
  
  tmp_list = sorted_list;
  while (tmp_list)
    {
      widget = tmp_list->data;
      tmp_list = tmp_list->next;
      
#ifdef TREE_DEBUG
      g_message("* item [%#x] subtree [%#x]\n", 
		(int)widget, (int)GTK_TREE_ITEM2_SUBTREE(widget));
#endif /* TREE_DEBUG */
      
      /* get real owner of this widget */
      real_tree = GTK_TREE2(widget->parent);
#ifdef TREE_DEBUG
      g_message("* subtree having this widget [%#x]\n", (int)real_tree);
#endif /* TREE_DEBUG */
      
      
      if (widget->state == GTK_STATE_SELECTED)
	{
	  selected_widgets = g_list_prepend (selected_widgets, widget);
#ifdef TREE_DEBUG
	  g_message("* selected widget - adding it in selected list [%#x]\n",
		    (int)selected_widgets);
#endif /* TREE_DEBUG */
	}
      
      /* remove this item from its real parent */
#ifdef TREE_DEBUG
      g_message("* remove widget from its owner tree\n");
#endif /* TREE_DEBUG */
      real_tree->children = g_list_remove (real_tree->children, widget);
      
      /* remove subtree associate at this item if it exist */      
      if(GTK_TREE_ITEM2(widget)->subtree) 
	{
#ifdef TREE_DEBUG
	  g_message("* remove subtree associate at this item [%#x]\n",
		    (int) GTK_TREE_ITEM2(widget)->subtree);
#endif /* TREE_DEBUG */
	  if (GTK_WIDGET_MAPPED (GTK_TREE_ITEM2(widget)->subtree))
	    gtk_widget_unmap (GTK_TREE_ITEM2(widget)->subtree);
	  
	  gtk_widget_unparent (GTK_TREE_ITEM2(widget)->subtree);
	  GTK_TREE_ITEM2(widget)->subtree = NULL;
	}
      
      /* really remove widget for this item */
#ifdef TREE_DEBUG
      g_message("* unmap and unparent widget [%#x]\n", (int)widget);
#endif /* TREE_DEBUG */
      if (GTK_WIDGET_MAPPED (widget))
	gtk_widget_unmap (widget);
      
      gtk_widget_unparent (widget);
      
      /* delete subtree if there is no children in it */
/*       if(real_tree->children == NULL && 
	 real_tree != root_tree)
	{
*/
#ifdef TREE_DEBUG
	  g_message("* owner tree don't have children ... destroy it\n");
#endif /* TREE_DEBUG */
/*
	  gtk_tree_item2_remove_subtree(GTK_TREE_ITEM2(real_tree->tree_owner));
	}
*/      
#ifdef TREE_DEBUG
      g_message("* next item in list\n");
#endif /* TREE_DEBUG */
    }
  
  if (selected_widgets)
    {
#ifdef TREE_DEBUG
      g_message("* scan selected item list\n");
#endif /* TREE_DEBUG */
      tmp_list = selected_widgets;
      while (tmp_list)
	{
	  widget = tmp_list->data;
	  tmp_list = tmp_list->next;
	  
#ifdef TREE_DEBUG
	  g_message("* widget [%#x] subtree [%#x]\n", 
		    (int)widget, (int)GTK_TREE_ITEM2_SUBTREE(widget));
#endif /* TREE_DEBUG */
	  
	  /* remove widget of selection */
	  root_tree->selection = g_list_remove (root_tree->selection, widget);
	  
	  /* unref it to authorize is destruction */
	  gtk_widget_unref (widget);
	}
      
      /* emit only one selection_changed signal */
      gtk_signal_emit (GTK_OBJECT (root_tree), 
		       tree_signals[SELECTION_CHANGED]);
    }
  
#ifdef TREE_DEBUG
  g_message("* free selected_widgets list\n");
#endif /* TREE_DEBUG */
  g_list_free (selected_widgets);
  g_list_free (sorted_list);
  
  if (root_tree->children && !root_tree->selection &&
      (root_tree->selection_mode == GTK_SELECTION_BROWSE))
    {
#ifdef TREE_DEBUG
      g_message("* BROWSE mode, select another item\n");
#endif /* TREE_DEBUG */
      widget = root_tree->children->data;
      gtk_tree2_select_child (root_tree, widget);
    }
  
  if (GTK_WIDGET_VISIBLE (root_tree))
    {
#ifdef TREE_DEBUG
      g_message("* query queue resizing for root_tree\n");
#endif /* TREE_DEBUG */      
      gtk_widget_queue_resize (GTK_WIDGET (root_tree));
    }
}

void
gtk_tree2_select_child (GtkTree2   *tree,
		       GtkWidget *tree_item)
{
  g_return_if_fail (tree != NULL);
  g_return_if_fail (GTK_IS_TREE2 (tree));
  g_return_if_fail (tree_item != NULL);
  g_return_if_fail (GTK_IS_TREE_ITEM2 (tree_item));
  
  gtk_signal_emit (GTK_OBJECT (tree), tree_signals[SELECT_CHILD], tree_item);
}

void
gtk_tree2_select_item (GtkTree2   *tree,
		      gint       item)
{
  GList *tmp_list;
  
  g_return_if_fail (tree != NULL);
  g_return_if_fail (GTK_IS_TREE2 (tree));
  
  tmp_list = g_list_nth (tree->children, item);
  if (tmp_list)
    gtk_tree2_select_child (tree, GTK_WIDGET (tmp_list->data));
  
}

static void
gtk_tree2_size_allocate (GtkWidget     *widget,
			GtkAllocation *allocation)
{
  GtkTree2 *tree;
  GtkWidget *child, *subtree;
  GtkAllocation child_allocation;
  GList *children;
  
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_TREE2 (widget));
  g_return_if_fail (allocation != NULL);
  
  tree = GTK_TREE2 (widget);
  
  widget->allocation = *allocation;
  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_move_resize (widget->window,
			    allocation->x, allocation->y,
			    allocation->width, allocation->height);
  
  if (tree->children)
    {
      child_allocation.x = GTK_CONTAINER (tree)->border_width;
      child_allocation.y = GTK_CONTAINER (tree)->border_width;
      child_allocation.width = MAX (1, (gint)allocation->width - child_allocation.x * 2);
      
      children = tree->children;
      
      while (children)
	{
	  child = children->data;
	  children = children->next;
	  
	  if (GTK_WIDGET_VISIBLE (child))
	    {
	      GtkRequisition child_requisition;
	      gtk_widget_get_child_requisition (child, &child_requisition);
	      
	      child_allocation.height = child_requisition.height;
	      
	      gtk_widget_size_allocate (child, &child_allocation);
	      
	      child_allocation.y += child_allocation.height;
	      
	      if((subtree = GTK_TREE_ITEM2(child)->subtree))
		if(GTK_WIDGET_VISIBLE (subtree))
		  {
		    child_allocation.height = subtree->requisition.height;
		    gtk_widget_size_allocate (subtree, &child_allocation);
		    child_allocation.y += child_allocation.height;
		  }
	    }
	}
    }
  
}

static void
gtk_tree2_size_request (GtkWidget      *widget,
		       GtkRequisition *requisition)
{
  GtkTree2 *tree;
  GtkWidget *child, *subtree;
  GList *children;
  GtkRequisition child_requisition;
  
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_TREE2 (widget));
  g_return_if_fail (requisition != NULL);
  
  tree = GTK_TREE2 (widget);
  requisition->width = 0;
  requisition->height = 0;
  
  children = tree->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (GTK_WIDGET_VISIBLE (child))
	{
	  gtk_widget_size_request (child, &child_requisition);
	  
	  requisition->width = MAX (requisition->width, child_requisition.width);
	  requisition->height += child_requisition.height;
	  
	  if((subtree = GTK_TREE_ITEM2(child)->subtree) &&
	     GTK_WIDGET_VISIBLE (subtree))
	    {
	      gtk_widget_size_request (subtree, &child_requisition);
	      
	      requisition->width = MAX (requisition->width, 
					child_requisition.width);
	      
	      requisition->height += child_requisition.height;
	    }
	}
    }
  
  requisition->width += GTK_CONTAINER (tree)->border_width * 2;
  requisition->height += GTK_CONTAINER (tree)->border_width * 2;
  
  requisition->width = MAX (requisition->width, 1);
  requisition->height = MAX (requisition->height, 1);
  
}

static void
gtk_tree2_unmap (GtkWidget *widget)
{
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_TREE2 (widget));
  
  GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
  gdk_window_hide (widget->window);
  
}

void
gtk_tree2_unselect_child (GtkTree2   *tree,
			 GtkWidget *tree_item)
{
  g_return_if_fail (tree != NULL);
  g_return_if_fail (GTK_IS_TREE2 (tree));
  g_return_if_fail (tree_item != NULL);
  g_return_if_fail (GTK_IS_TREE_ITEM2 (tree_item));
  
  gtk_signal_emit (GTK_OBJECT (tree), tree_signals[UNSELECT_CHILD], tree_item);
}

void
gtk_tree2_unselect_item (GtkTree2 *tree,
			gint     item)
{
  GList *tmp_list;
  
  g_return_if_fail (tree != NULL);
  g_return_if_fail (GTK_IS_TREE2 (tree));
  
  tmp_list = g_list_nth (tree->children, item);
  if (tmp_list)
    gtk_tree2_unselect_child (tree, GTK_WIDGET (tmp_list->data));
  
}

static void
gtk_real_tree2_select_child (GtkTree2   *tree,
			    GtkWidget *child)
{
  GList *selection, *root_selection;
  GList *tmp_list;
  GtkWidget *tmp_item;
  
  g_return_if_fail (tree != NULL);
  g_return_if_fail (GTK_IS_TREE2 (tree));
  g_return_if_fail (child != NULL);
  g_return_if_fail (GTK_IS_TREE_ITEM2 (child));
  
  root_selection = tree->root_tree->selection;
  
  switch (tree->root_tree->selection_mode)
    {
    case GTK_SELECTION_SINGLE:
      
      selection = root_selection;
      
      /* remove old selection list */
      while (selection)
	{
	  tmp_item = selection->data;
	  
	  if (tmp_item != child)
	    {
	      gtk_tree_item2_deselect (GTK_TREE_ITEM2 (tmp_item));
	      
	      tmp_list = selection;
	      selection = selection->next;
	      
	      root_selection = g_list_remove_link (root_selection, tmp_list);
	      gtk_widget_unref (tmp_item);
	      
	      g_list_free (tmp_list);
	    }
	  else
	    selection = selection->next;
	}
      
      if (child->state == GTK_STATE_NORMAL)
	{
	  gtk_tree_item2_select (GTK_TREE_ITEM2 (child));
	  root_selection = g_list_prepend (root_selection, child);
	  gtk_widget_ref (child);
	}
      else if (child->state == GTK_STATE_SELECTED)
	{
	  gtk_tree_item2_deselect (GTK_TREE_ITEM2 (child));
	  root_selection = g_list_remove (root_selection, child);
	  gtk_widget_unref (child);
	}
      
      tree->root_tree->selection = root_selection;
      
      gtk_signal_emit (GTK_OBJECT (tree->root_tree), 
		       tree_signals[SELECTION_CHANGED]);
      break;
      
      
    case GTK_SELECTION_BROWSE:
      selection = root_selection;
      
      while (selection)
	{
	  tmp_item = selection->data;
	  
	  if (tmp_item != child)
	    {
	      gtk_tree_item2_deselect (GTK_TREE_ITEM2 (tmp_item));
	      
	      tmp_list = selection;
	      selection = selection->next;
	      
	      root_selection = g_list_remove_link (root_selection, tmp_list);
	      gtk_widget_unref (tmp_item);
	      
	      g_list_free (tmp_list);
	    }
	  else
	    selection = selection->next;
	}
      
      tree->root_tree->selection = root_selection;
      
      if (child->state == GTK_STATE_NORMAL)
	{
	  gtk_tree_item2_select (GTK_TREE_ITEM2 (child));
	  root_selection = g_list_prepend (root_selection, child);
	  gtk_widget_ref (child);
	  tree->root_tree->selection = root_selection;
	  gtk_signal_emit (GTK_OBJECT (tree->root_tree), 
			   tree_signals[SELECTION_CHANGED]);
	}
      break;
      
    case GTK_SELECTION_MULTIPLE:
      if (child->state == GTK_STATE_NORMAL)
	{
	  gtk_tree_item2_select (GTK_TREE_ITEM2 (child));
	  root_selection = g_list_prepend (root_selection, child);
	  gtk_widget_ref (child);
	  tree->root_tree->selection = root_selection;
	  gtk_signal_emit (GTK_OBJECT (tree->root_tree), 
			   tree_signals[SELECTION_CHANGED]);
	}
      else if (child->state == GTK_STATE_SELECTED)
	{
	  gtk_tree_item2_deselect (GTK_TREE_ITEM2 (child));
	  root_selection = g_list_remove (root_selection, child);
	  gtk_widget_unref (child);
	  tree->root_tree->selection = root_selection;
	  gtk_signal_emit (GTK_OBJECT (tree->root_tree), 
			   tree_signals[SELECTION_CHANGED]);
	}
      break;
      
    case GTK_SELECTION_EXTENDED:
      break;
    }
}

static void
gtk_real_tree2_unselect_child (GtkTree2   *tree,
			      GtkWidget *child)
{
  g_return_if_fail (tree != NULL);
  g_return_if_fail (GTK_IS_TREE2 (tree));
  g_return_if_fail (child != NULL);
  g_return_if_fail (GTK_IS_TREE_ITEM2 (child));
  
  switch (tree->selection_mode)
    {
    case GTK_SELECTION_SINGLE:
    case GTK_SELECTION_MULTIPLE:
    case GTK_SELECTION_BROWSE:
      if (child->state == GTK_STATE_SELECTED)
	{
	  GtkTree2* root_tree = GTK_TREE2_ROOT_TREE(tree);
	  gtk_tree_item2_deselect (GTK_TREE_ITEM2 (child));
	  root_tree->selection = g_list_remove (root_tree->selection, child);
	  gtk_widget_unref (child);
	  gtk_signal_emit (GTK_OBJECT (tree->root_tree), 
			   tree_signals[SELECTION_CHANGED]);
	}
      break;
      
    case GTK_SELECTION_EXTENDED:
      break;
    }
}

void
gtk_tree2_set_selection_mode (GtkTree2       *tree,
			     GtkSelectionMode mode) 
{
  g_return_if_fail (tree != NULL);
  g_return_if_fail (GTK_IS_TREE2 (tree));
  
  tree->selection_mode = mode;
}

void
gtk_tree2_set_view_mode (GtkTree2       *tree,
			GtkTree2ViewMode mode) 
{
  g_return_if_fail (tree != NULL);
  g_return_if_fail (GTK_IS_TREE2 (tree));
  
  tree->view_mode = mode;
}

void
gtk_tree2_set_view_lines (GtkTree2       *tree,
			 guint          flag) 
{
  g_return_if_fail (tree != NULL);
  g_return_if_fail (GTK_IS_TREE2 (tree));
  
  tree->view_line = flag;
}