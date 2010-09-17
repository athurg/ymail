#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>
#include <time.h>

#include "mbox.h"

GtkBuilder *builder;

// 用于存储显示数据的模型
GtkListStore *list_store;

unsigned int unread=0, total=0;

gboolean status_icon_button_press_cb(GtkStatusIcon *status_icon, GdkEventButton *event, gpointer user_data)
{
	GtkWidget *window;


	if(event->button==1){//左键
		window = GTK_WIDGET(gtk_builder_get_object(builder,"main_window"));
		if(gtk_widget_get_visible(window))
			gtk_widget_hide(window);
		else
			gtk_widget_show(window);
	}
	printf("you press button %d\n", event->button);
}

void release_liststore(void)
{
	GtkListStore *list_store;
	list_store = GTK_LIST_STORE(gtk_builder_get_object(builder,"liststore"));

	printf("Byle\n");
	g_object_unref(list_store);
}

int refresh(int type)
{
	FILE *mbox_fp=NULL;

	printf("刷新：");

	mbox_fp = fopen("/var/mail/athurg", "rb");
	if(mbox_fp == NULL){
		perror("打开邮箱失败");
		return -1;
	}
	gtk_list_store_clear(list_store);

	for(int i=0; i<1000; i++){
		struct mail_hdr hdr;
		GtkTreeIter iter;
		int ret=0;
		char strstat[10]={0};
		char attach[10]={0};

		ret = parse_header(mbox_fp, &hdr);

		if(ret == MSG_STATUS_UNREAD)
			strcpy(strstat, "未读");
		else if(ret == MSG_STATUS_READED)
			strcpy(strstat, "已读");
		else if(ret == MSG_STATUS_NEWRCV)
			strcpy(strstat, "新来");
		else if(ret<0){
			break;
		}

		if(hdr.type=='a')
			strcpy(attach, "无附件");
		else if(hdr.type=='m')
			strcpy(attach, "有附件");
		else if(hdr.type=='r')
			strcpy(attach, "复合");
		else
			strcpy(attach, "未知");

		gtk_list_store_append(list_store, &iter);
		gtk_list_store_set(list_store, &iter,
				0, strstat,
				1, hdr.time,
				2, hdr.sender,
				3, hdr.subject,
				4, attach,
				-1);
		free_mail_hdr(&hdr);
	}
	printf("完成\n");
}


int refresh_all(void)
{
	refresh(1);
}
int prog_init(int argc, char *argv[])
{
	GtkTreeView *tree_view;


	gtk_init(&argc, &argv);

	//数据模型初始化
	//model = GTK_TREE_MODEL(gtk_list_store_new(5,
	list_store = gtk_list_store_new(5,
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_STRING);

	//图形界面初始化

	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "ui.glade", NULL);
	gtk_builder_connect_signals(builder, NULL);

	tree_view= GTK_TREE_VIEW(gtk_builder_get_object(builder,"treeview"));
	gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(list_store));

	//定时每3秒刷新检测一次邮件
	//gtk_timeout_add(1000*3, (GtkFunction) refresh, NULL);
}

#define DEBUG
int main(int argc, char *argv[])
{
#ifndef DEBUG
	prog_init(argc, argv);

	refresh(1);
	gtk_main();
#else

	FILE *mbox_fp;
	struct mail_hdr hdr, hdr2;
	char buff[1024];
	unsigned long size=0;

	mbox_fp = fopen("/var/mail/athurg", "rb");
	parse_header(mbox_fp, &hdr);
	size += hdr.h_size + hdr.c_size+1;

	fclose(mbox_fp);

	//printf("%d\t%d\t%d\t%c\t%s\n",i, hdr.h_size, hdr.c_size, hdr.type, hdr.subject);
	mbox_fp = fopen("/var/mail/athurg", "rb");
	fseek(mbox_fp, size, SEEK_CUR);
	fgets(buff, 1024, mbox_fp);	printf("%s",buff);

	fclose(mbox_fp);


#endif
	return 0;
}


