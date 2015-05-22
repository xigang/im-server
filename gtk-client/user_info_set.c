/* *****************************************************************************
 * Author:         Xigang Wang
 * Email:          wangxigang2014@gmail.com
 * Last modfified: 2015-05-10 15:22
 * Filename:       user_info_set.c
 * Note:           任何人可以任意复制代码并运用这些文档，当然包括你的商业用途, 但请遵循GPL
 * *****************************************************************************/
#include "lin.h"
#include "user_info_set.h"

GtkWidget *button_image, *button_preview;
GtkWidget *image_user, *image_user_old, *image_user_new;
gchar *str_image_user, *str_image_user_new;

GtkWidget *create_user_info_dlg()
{
	// 创建 User Info 窗口, 设置 User 信息

	GtkWidget *dialog;
	GtkWidget *notebook;
	GtkWidget *page_user_pic, *page_user_basic_info;
	GtkWidget *label_pic, *label_text;

	dialog = gtk_dialog_new_with_buttons("Change User Information", NULL,
			GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG (dialog), GTK_RESPONSE_OK);
	// 设置固定大小
	gtk_window_set_resizable((GtkWindow *) dialog, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER (dialog), 5);
	gtk_widget_set_size_request(dialog, 420, 265);
	gtk_window_set_position((GtkWindow *) dialog, GTK_WIN_POS_MOUSE);

	// 创建 notebook
	notebook = gtk_notebook_new();

	label_pic = gtk_label_new("Change User Picture");
	label_text = gtk_label_new("Change User Basic Info");

	page_user_pic = create_user_pic_page();
	page_user_basic_info = create_user_basic_info_page();

	// 添加 page 到 notebook
	gtk_notebook_append_page(GTK_NOTEBOOK (notebook), page_user_pic, label_pic);
	gtk_notebook_append_page(GTK_NOTEBOOK (notebook), page_user_basic_info,
			label_text);

	gtk_notebook_set_tab_pos(GTK_NOTEBOOK (notebook), GTK_POS_BOTTOM);

	gtk_box_pack_start(GTK_BOX (GTK_DIALOG(dialog)->vbox), notebook, TRUE,
			TRUE, 10);

	gtk_widget_show_all(dialog);

	return dialog;
}

GtkWidget *create_user_pic_page()
{
	GtkWidget *hbox_main, *vbox_preview;
	GtkWidget *scrolled_window;
	GtkWidget *table;
	GtkWidget *button_bmp;
	GtkWidget *image_bmp;
	int i, j;
	char *buf_path;

	// 注意作为 notebook 的 child, 只能传递类似 vbox 的组件, 不能创建类似 dialog 的窗体, 因为其
	// 没法作为 child 存在

	hbox_main = gtk_hbox_new(FALSE, 0);
	gtk_widget_set_size_request(hbox_main, 400, 180);

	// 创建一个新的滚动窗口
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(scrolled_window, 340, 180);
	gtk_container_set_border_width(GTK_CONTAINER (scrolled_window), 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled_window),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_box_pack_start(GTK_BOX (hbox_main), scrolled_window, TRUE, TRUE, 0);

	// 头像预览
	button_preview = gtk_button_new();
	gtk_widget_set_size_request(button_preview, 52, 52);
	image_user_old = gtk_image_new_from_file(str_image_user);
	gtk_container_add(GTK_CONTAINER(button_preview), image_user_old);
	vbox_preview = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX (vbox_preview), button_preview, FALSE, FALSE, 10);
	gtk_box_pack_start(GTK_BOX (hbox_main), vbox_preview, TRUE, FALSE, 0);

	// 创建一个包含 6 × 10 个格子的表格, 用于存放 user pic
	table = gtk_table_new(10, 6, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE (table), 2);
	gtk_table_set_col_spacings(GTK_TABLE (table), 2);

	// 将表格组装到滚动窗口中
	gtk_scrolled_window_add_with_viewport(
			GTK_SCROLLED_WINDOW (scrolled_window), table);

	// 开始添加 bmp 头像
	for (i = 0; i < 10; i++)//i * 6 + j + 1
		for (j = 0; j < 6; j++)
		{
			buf_path = g_strdup_printf("./Image/newface/%d.bmp", i * 6 + j + 1);
			image_bmp = gtk_image_new_from_file(buf_path);

			button_bmp = gtk_button_new();
			g_signal_connect (G_OBJECT (button_bmp), "button_press_event",
					G_CALLBACK (user_image_click), buf_path);

			gtk_widget_set_size_request(button_bmp, 52, 52);
			gtk_container_add(GTK_CONTAINER(button_bmp), image_bmp);
			gtk_table_attach_defaults(GTK_TABLE (table), button_bmp, j, j + 1,
					i, i + 1);
			gtk_widget_show(button_bmp);
		}

	return hbox_main;
}

GtkWidget *create_user_basic_info_page()
{
	GtkWidget *vbox_main;
	GtkWidget *entry_nickname, *entry_pass_old, *entry_pass_new;
	GtkWidget *label;
	GtkWidget *hbox_nickname, *hbox_pass_old, *hbox_pass_new;

	// 注意作为 notebook 的 child, 只能传递类似 vbox 的组件, 不能创建类似 dialog 的窗体, 因为其
	// 没法作为 child 存在

	vbox_main = gtk_vbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER (vbox_main), 15);

	label = gtk_label_new("NickName:      ");
	entry_nickname = gtk_entry_new();
	hbox_nickname = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX (hbox_nickname), label, FALSE, FALSE, 10);
	gtk_box_pack_start(GTK_BOX (hbox_nickname), entry_nickname, TRUE, FALSE, 0);

	label = gtk_label_new("Old Password: ");
	entry_pass_old = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY (entry_pass_old), FALSE);
	gtk_entry_set_invisible_char(GTK_ENTRY (entry_pass_old), '*');
	hbox_pass_old = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX (hbox_pass_old), label, FALSE, FALSE, 10);
	gtk_box_pack_start(GTK_BOX (hbox_pass_old), entry_pass_old, TRUE, FALSE, 0);

	label = gtk_label_new("New Password:");
	entry_pass_new = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY (entry_pass_new), FALSE);
	gtk_entry_set_invisible_char(GTK_ENTRY (entry_pass_new), '*');
	hbox_pass_new = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX (hbox_pass_new), label, FALSE, FALSE, 10);
	gtk_box_pack_start(GTK_BOX (hbox_pass_new), entry_pass_new, TRUE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX (vbox_main), hbox_nickname, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX (vbox_main), hbox_pass_old, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX (vbox_main), hbox_pass_new, TRUE, TRUE, 0);

	return vbox_main;
}

void user_image_click(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	// 存放新头像的路径信息
	str_image_user_new = g_strdup(data);
	image_user_new = gtk_image_new_from_file((gchar *) data);
	gtk_container_remove(GTK_CONTAINER(button_preview), image_user_old);
	image_user_old = image_user_new;
	gtk_container_add(GTK_CONTAINER(button_preview), image_user_new);
	gtk_widget_show_all(button_preview);
}
