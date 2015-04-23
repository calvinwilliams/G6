# 此文件由makeobj.sh自动生成
############################################################
# 项目名 : 
# 模块名 : 
# 备  注 : 
############################################################

###### 源文件配置区
#@ c_FILE
c_FILE		=	\
			slow_client.c \
			slow_server.c \

###### 目标文件、安装目录配置区
include makeinstall
BIN		=	slow_client	slow_server

###### 编译选项
CFLAGS		=	$(_CFLAGS) \

###### 链接选项
LFLAGS		=	$(_LFLAGS) \

###### 额外宏定义区
CLEAN_ADDITION	=

###### 加载mktpl模板库
#@ make_all
#@ make_clean
#@ make_install
#@ make_install_BININST
#@ make_install_DFTHDERINST
#@ make_uninstall
#@ make_uninstall_BININST
#@ make_uninstall_DFTHDERINST
include $(MKTPLDIR)/makeobj_$(MKTPLOS).inc

###### 目标文件依赖关系
slow_client		:	slow_client.c
	$(CC) -o $@ slow_client.c $(LFLAGS)

slow_server		:	slow_server.c
	$(CC) -o $@ slow_server.c $(LFLAGS)

