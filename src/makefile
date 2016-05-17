# 此文件由makeobj.sh自动生成
############################################################
# 项目名 : 
# 模块名 : 
# 备  注 : 
############################################################

###### 源文件配置区
#@ c_FILE
c_FILE		=	\
			LOGC.c \
			rbtree.c \
			Util.c \
			main.c \
			Envirment.c \
			Config.c \
			MonitorProcess.c \
			WorkerProcess.c \
			AcceptThread.c \
			ForwardThread.c \
			TimeThread.c \

###### 目标文件、安装目录配置区
include makeinstall
BIN		=	G6
BININST		=	$(_BININST)

###### 编译选项
CFLAGS		=	$(_CFLAGS) \
			-std=gnu99 \

###### 链接选项
LFLAGS		=	$(_LFLAGS) \
			-lpthread \

###### 额外宏定义区
CLEAN_ADDITION	=

###### 加载mktpl模板库
#@ make_all
#@ make_clean
#@ make_install
#@ make_install_BININST
#@ make_install_HDERINST
#@ make_install_DFTHDERINST
#@ make_uninstall
#@ make_uninstall_BININST
#@ make_uninstall_HDERINST
#@ make_uninstall_DFTHDERINST
include $(MKTPLDIR)/makeobj_$(MKTPLOS).inc

###### 目标文件依赖关系
G6		:	$(c_FILE_o)
	$(CC) -o $@ $(c_FILE_o) $(LFLAGS)

