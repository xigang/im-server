#ifndef TREEVIEW_H_
#define TREEVIEW_H_

enum
{
	ICON = 0, NAME, IP, COLOR, N_COLUMNS
};

enum
{
	PARENT_ITEM, CHILD_ITEM
};

typedef struct
{
	int item_type;
	char id[15]; // user id
	char name[20]; // user nickname
	struct in_addr ip_addr; // user ip address at in_addr format
	int face; // user face num
	char online; // user is online
} s_user_info, Tree_Item;

GtkWidget *create_tree_view();
void setup_tree_view(GtkWidget *treeview);
void setup_tree_view_model(GtkWidget *treeview, GArray *array_user,
		gchar *my_id);
void row_activated(GtkTreeView *treeview, GtkTreePath *path,
		GtkTreeViewColumn *column, gpointer data);

#endif /* TREEVIEW_H_ */
