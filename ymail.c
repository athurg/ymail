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

/* 图形界面回调函数 */
gboolean status_icon_button_press_cb(GtkStatusIcon *status_icon, GdkEventButton *event, gpointer user_data)
{
	GtkWidget *window;

	if(event->button==1){
		g_warning("左（1）键按下\n");
	} else if(event->button==2){
		g_warning("中（2）键按下\n系统退出");
		gtk_main_quit();
	} else {
		g_warning("其他（%d）键按下\n系统退出", event->button);
	}
}

void ntf_action_cb(NotifyNotification *ntf, gchar *action, gpointer user_data)
{
	static unsigned int pos=0;
	unsigned int max=0;

	if (!strcmp(action, "call")) {
		system("sylpheed");
		return;
	} else if(!strcmp(action, "prev")){
		if(pos>10)
			pos-=10;
		else
			pos=0;
	}else if(!strcmp(action, "next")){
		if(pos < (g_list_length(list)-10))
			pos+=10;
	}else if(!strcmp(action, "read")){
		max = g_list_length(list) - pos;
		max = max > 10 ? 10 : max;
		mark_read(pos,max);
	}

	show_popup(pos);
}


/* 功能性函数 */
int mark_read(unsigned int pos, unsigned int max)
{
	GList *lst;
	FILE *fp, *hdr_fp;
	struct mail_hdr *hdr;
	unsigned int filename_cnt;
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

void notification_set_action(unsigned int btn_msk)
{
	notify_notification_clear_actions(ntf);

	if(btn_msk & NTF_PREV_BTN_MSK)
		notify_notification_add_action(ntf,"prev", "前一页", ntf_action_cb, NULL, NULL);
	
	if(btn_msk & NTF_NEXT_BTN_MSK)
		notify_notification_add_action(ntf,"next", "后一页", ntf_action_cb, NULL, NULL);

	if(btn_msk & NTF_READ_BTN_MSK)
		notify_notification_add_action(ntf,"read", "标记已读", ntf_action_cb, NULL, NULL);

	notify_notification_add_action(ntf, "call", "打开sylpheed", ntf_action_cb, NULL, NULL);
}

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
		g_string_append_printf(ntf_msg, "%s\n", hdr->subject);
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
	gtk_builder_add_from_file(builder, "ui.glade", NULL);
	gtk_builder_connect_signals(builder, NULL);

	status_icon = GTK_STATUS_ICON(gtk_builder_get_object(builder, "status_icon"));
	ntf = notify_notification_new_with_status_icon("暂无邮件", "暂无邮件", "pidgin", status_icon);

	refresh(NULL);
	g_timeout_add_seconds(60, refresh, NULL);

	gtk_main();
	notify_uninit();

	return 0;
}

