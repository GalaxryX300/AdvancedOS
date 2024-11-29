// main.cpp
#include <QApplication>
#include "../include/mainwindow.h"
#include "../include/advanced_kernel.h"
#include "../include/network.h"
#include "../include/user_auth.h"
#include "../include/disk_manager.h"

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);
	
	// 初始化系统组件
	AdvancedKernel kernel;
	NetworkManager network;
	UserAuth auth;
	DiskManager disk("system.disk", 1024 * 1024 * 1024); // 1GB
	SystemLogger logger("system.log");
	
	// 启动网络服务
	if(!network.start_server(8080)) {
		logger.error("Failed to start network server");
		return 1;
	}
	
	// 创建并显示主窗口
	MainWindow window(kernel, network, auth, disk, logger);
	window.show();
	
	// 启动应用程序事件循环
	return app.exec();
}
