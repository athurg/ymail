#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>
#include <time.h>
#include <dirent.h>
#include <libnotify/notify.h>
#include "mbox.h"

#define NTF_PREV_BTN_MSK	(1<<0)
#define NTF_NEXT_BTN_MSK	(1<<1)
#define NTF_READ_BTN_MSK	(1<<2)

GtkBuilder *builder;
NotifyNotification * ntf;
GList *list = NULL;

void show_popup(unsigned int pos);
int refresh(gpointer data);
int mark_read(unsigned int pos, unsigned int max);
void get_menu_pos(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data);

/* 图形界面回调函数 */
void open_mail_client_cb(GtkWidget *widget, gpointer data)
{
	if (fork()) {
		g_message("Call sylpheed.....");
		execlp("sylpheed",NULL);
	}
}


/*
 * 	配置窗口提交按钮回调函数
 */
void config_submit_cb(GtkWidget *widget, gpointer data)
{
	GtkWidget *window;

	window = GTK_WIDGET(gtk_builder_get_object(builder, "config_window"));
	gtk_widget_hide(window);
	g_debug("提交修改");
}

/*
 * 	通知区域图标按键回调函数
 *
 * 说明：
 * 	按键通过evnet->button的值来区分，1=左键、2=中键、3=右键……
 */
gboolean status_icon_button_press_cb(GtkStatusIcon *status_icon, GdkEventButton *event, gpointer user_data)
{
	GtkWidget *widget;

	if(event->button==1){
		show_popup(0);
	} else if(event->button==3){
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "menu"));
		gtk_menu_popup(GTK_MENU(widget), NULL, NULL, NULL, NULL, 0, 0);
	}
}

/*
 * 	Notification信息窗口按钮回调函数
 *
 * 说明：
 * 	按钮通过action来区分，action对应按钮的名称
 */
void ntf_action_cb(NotifyNotification *ntf, gchar *action, gpointer user_data)
{
	static unsigned int pos=0;
	unsigned int max=0;

	if(!strcmp(action, "prev")){	//上一页
		if(pos>10)
			pos-=10;
		else
			pos=0;
	}else if(!strcmp(action, "next")){	//下一页
		if(pos < (g_list_length(list)-10))
			pos+=10;
	}else if(!strcmp(action, "read")){	//标记本页为已读
		max = g_list_length(list) - pos;
		max = max > 10 ? 10 : max;
		mark_read(pos,max);
	}

	show_popup(pos);
}


/*
 * 	标记已读函数
 *
 * 说明：
 * 	用于将邮件标记为已读
 */
int mark_read(unsigned int pos, unsigned int max)
{
	GList *lst;
	FILE *fp, *hdr_fp;
	struct mail_hdr *hdr;
	unsigned int filename_cnt=0;
	char new_filename[100]={0};
	char old_filename[100]={0};

	fp = popen("ls -v -r ~/Mail/inbox/", "r");
	if (fp == NULL) {
		perror("Fail to open pipe!");
		return -1;
	}

	fscanf(fp, "%d", &filename_cnt);
	filename_cnt++;

	for (int i=0; i<max; i++){
		lst = g_list_nth(list, pos);
		hdr = lst->data;
		g_message("处理%s => %d\t", hdr->filename, filename_cnt);
		sprintf(new_filename, "/home/athurg/Mail/inbox/%d", filename_cnt++);
		sprintf(old_filename, "/home/athurg/Mail/new/%s", hdr->filename);
		rename(old_filename, new_filename);
		free_mail_hdr(hdr);
		list = g_list_delete_link(list, lst);
	}
}

void free_list(gpointer hdr_p, gpointer user_data)
{
	free_mail_hdr(hdr_p);
}

int refresh(gpointer data)
{
	FILE *hdr_fp;
	DIR *dirp;
	struct dirent *direntp;


	dirp = opendir("/home/athurg/Mail/new");
	if (dirp == NULL) {
		perror("Fail to open dir:");
		return -1;
	}

	chdir("/home/athurg/Mail/new");

	g_debug("刷新了");
	//清除原有数据
	if(g_list_length(list)>0) {
		g_list_foreach(list, free_list, NULL);
		//FIXME
		//		这里按道理g_list_free应该是可以将list所有的内存
		//	释放，并将list返回为NULL，但是奇怪的是没有（free后立刻
		//	用g_list_length测算结果为1）。这里只有手动置为NULL
		g_list_free(list);
		list = NULL;
	}

	while(direntp = readdir(dirp)){
		GtkTreeIter iter;
		struct mail_hdr *header = malloc(sizeof(struct mail_hdr));

		//跳过以【.】开头的文件
		if(direntp->d_name[0]=='.')	continue;

		hdr_fp = fopen(direntp->d_name, "r");
		if(hdr_fp == NULL){
			perror("Open file failed");
			return -2;
		}

		if(0==parse_header(hdr_fp, header)){
			header->filename = strdup(direntp->d_name);
			list = g_list_append(list, header);
		}
		fclose(hdr_fp);
	}

	closedir(dirp);

	show_popup(0);
	return TRUE;
}

/*
 * 	设置notification提示窗口按钮函数
 */
void notification_set_action(unsigned int btn_msk)
{
	notify_notification_clear_actions(ntf);

	if(btn_msk & NTF_PREV_BTN_MSK)
		notify_notification_add_action(ntf,"prev", "前一页", ntf_action_cb, NULL, NULL);
	
	if(btn_msk & NTF_NEXT_BTN_MSK)
		notify_notification_add_action(ntf,"next", "后一页", ntf_action_cb, NULL, NULL);

	if(btn_msk & NTF_READ_BTN_MSK)
		notify_notification_add_action(ntf,"read", "标记已读", ntf_action_cb, NULL, NULL);
}

/*
 * 	显示notification提示窗口函数
 */
void show_popup(unsigned int pos)
{
	GList *lst;
	struct mail_hdr *hdr;
	unsigned int max=0, btn_msk=0;
	GString *ntf_msg = g_string_new("");

	if(g_list_length(list)==0){
		return;
	}

	if (g_list_length(list)<pos) {
		notification_set_action(NTF_PREV_BTN_MSK | NTF_READ_BTN_MSK);
		notify_notification_update(ntf, "没有了", "没有了", "pidgin");
		notify_notification_show(ntf, NULL);
		return;
	}

	if(pos >= 10){
		btn_msk |= NTF_PREV_BTN_MSK;
	}

	max = g_list_length(list) - pos;
	if (max > 10) {
		max = 10;
		btn_msk |= NTF_NEXT_BTN_MSK;
	}

	btn_msk |= NTF_READ_BTN_MSK;

	for(int i=0; i<max; i++){
		lst = g_list_nth(list, pos + i);
		hdr = lst->data;
		g_string_append_printf(ntf_msg, "%s: %s\n", hdr->sender, hdr->subject);
	}

	// 添加通知区域提示信息
	notification_set_action(btn_msk);
	notify_notification_update(ntf, "新邮件", ntf_msg->str, "pidgin");
	notify_notification_show(ntf, NULL);

	g_string_free(ntf_msg, TRUE);
}

int main(int argc, char **argv)
{
	GtkStatusIcon *status_icon;

	if (fork()) {
		g_message("Running as a daemon!");
		return 0;
	}
	gtk_init(&argc, &argv);

	notify_init("p");

	//图形界面初始化
	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "/home/athurg/所有项目/ymail/ui.glade", NULL);
	gtk_builder_connect_signals(builder, NULL);

	status_icon = GTK_STATUS_ICON(gtk_builder_get_object(builder, "status_icon"));
	ntf = notify_notification_new_with_status_icon("暂无邮件", "暂无邮件", NULL, status_icon);

	refresh(NULL);
	g_timeout_add_seconds(60, refresh, NULL);

	gtk_main();
	notify_uninit();

	return 0;
}

