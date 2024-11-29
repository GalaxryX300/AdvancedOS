// mainwindow.cpp - 主窗口实现
#include "../include/mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <QChart>
#include <QPieSeries>
#include <QLineSeries>
#include <QValueAxis>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QPieSeries>
#include <QHeaderView>
#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QTimer>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>

using namespace QtCharts;

// 在 MainWindow 构造函数中
MainWindow::MainWindow(AdvancedKernel& k, NetworkManager& n, UserAuth& a,
	DiskManager& d, SystemLogger& l, QWidget *parent)
: QMainWindow(parent), kernel(k), network(n), auth(a), disk(d), logger(l) {
	setup_ui();
	
	// 优化定时更新机制
	update_timer = new QTimer(this);
	connect(update_timer, &QTimer::timeout, this, &MainWindow::update_displays);
	update_timer->start(2000); // 改为2秒更新一次
	
	last_update = QDateTime::currentDateTime(); // 初始化上次更新时间
	logger.info("GUI system started");
}

MainWindow::~MainWindow() {
	logger.info("GUI system shutdown");
}

void MainWindow::setup_ui() {
	setWindowTitle("Advanced Operating System");
	resize(1024, 768);
	
	create_menus();
	create_tabs();
	create_command_panel();
	
	statusBar()->showMessage("Ready");
}

void MainWindow::create_menus() {
	// 创建文件菜单
	QMenu* file_menu = menuBar()->addMenu("File");
	QAction* logout_action = file_menu->addAction("Logout");
	file_menu->addSeparator();
	QAction* exit_action = file_menu->addAction("Exit");
	
	connect(logout_action, &QAction::triggered, this, &MainWindow::handle_logout);
	connect(exit_action, &QAction::triggered, this, &QWidget::close);
	
	// 创建工具菜单
	QMenu* tools_menu = menuBar()->addMenu("Tools");
	QAction* settings_action = tools_menu->addAction("Settings");
	
	connect(settings_action, &QAction::triggered, this, &MainWindow::handle_settings);
	
	// 创建帮助菜单
	QMenu* help_menu = menuBar()->addMenu("Help");
	QAction* about_action = help_menu->addAction("About");
	
	connect(about_action, &QAction::triggered, this, &MainWindow::handle_about);
}

void MainWindow::create_process_tab() {
	QWidget* process_widget = new QWidget;
	QVBoxLayout* layout = new QVBoxLayout;
	
	// 创建进程表格
	process_table = new QTableWidget;
	process_table->setColumnCount(5);
	process_table->setHorizontalHeaderLabels({
		"PID", "Name", "State", "Priority", "Memory Usage"
	});
	
	// 设置表格属性
	process_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	process_table->setSelectionMode(QAbstractItemView::SingleSelection);
	process_table->setUpdatesEnabled(false); // 初始禁用更新
	process_table->setSortingEnabled(true);  // 启用排序
	process_table->verticalHeader()->setVisible(false); // 隐藏行号
	process_table->setShowGrid(false); // 隐藏网格线
	process_table->setAlternatingRowColors(true); // 交替行颜色
	
	// 创建按钮
	QHBoxLayout* button_layout = new QHBoxLayout;
	QPushButton* new_process_btn = new QPushButton("New Process");
	QPushButton* kill_process_btn = new QPushButton("Kill Process");
	QPushButton* nice_process_btn = new QPushButton("Change Priority");
	
	button_layout->addWidget(new_process_btn);
	button_layout->addWidget(kill_process_btn);
	button_layout->addWidget(nice_process_btn);
	
	// 优化新建进程操作
	connect(new_process_btn, &QPushButton::clicked, [this]() {
		QString name = QInputDialog::getText(this, "New Process",
			"Enter process name:", QLineEdit::Normal, "", nullptr);
		
		if(!name.isEmpty()) {
			QApplication::setOverrideCursor(Qt::WaitCursor);
			kernel.create_process(name.toStdString());
			logger.info("Created new process: " + name.toStdString());
			update_process_table_efficient();
			QApplication::restoreOverrideCursor();
		}
	});
	
	// 优化终止进程操作
	connect(kill_process_btn, &QPushButton::clicked, [this]() {
		QList<QTableWidgetItem*> selected = process_table->selectedItems();
		if(selected.isEmpty()) {
			QMessageBox::warning(this, "Warning", "Please select a process first");
			return;
		}
		
		int row = process_table->row(selected[0]);
		int pid = process_table->item(row, 0)->text().toInt();
		
		QApplication::setOverrideCursor(Qt::WaitCursor);
		kernel.terminate_process(pid);
		logger.info("Terminated process: " + std::to_string(pid));
		update_process_table_efficient();
		QApplication::restoreOverrideCursor();
	});
	
	// 优化更改优先级操作
	connect(nice_process_btn, &QPushButton::clicked, [this]() {
		QList<QTableWidgetItem*> selected = process_table->selectedItems();
		if(selected.isEmpty()) {
			QMessageBox::warning(this, "Warning", "Please select a process first");
			return;
		}
		
		int row = process_table->row(selected[0]);
		int pid = process_table->item(row, 0)->text().toInt();
		bool ok;
		int new_priority = QInputDialog::getInt(this, "Change Priority",
			"Enter new priority (-20 to 19):", 0, -20, 19, 1, &ok);
		
		if(ok) {
			QApplication::setOverrideCursor(Qt::WaitCursor);
			if(kernel.change_process_priority(pid, new_priority)) {
				logger.info("Changed priority of process " + std::to_string(pid) + 
					" to " + std::to_string(new_priority));
			} else {
				logger.warning("Failed to change priority of process " + std::to_string(pid));
				QMessageBox::warning(this, "Error", "Failed to change process priority");
			}
			update_process_table_efficient();
			QApplication::restoreOverrideCursor();
		}
	});
	
	layout->addWidget(process_table);
	layout->addLayout(button_layout);
	
	process_widget->setLayout(layout);
	tab_widget->addTab(process_widget, "Processes");
	
	// 设置列自动拉伸
	for(int i = 0; i < process_table->columnCount(); i++) {
		process_table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
	}
	
	process_table->setUpdatesEnabled(true); // 恢复更新
}

void MainWindow::update_process_table_efficient() {
	QApplication::setOverrideCursor(Qt::WaitCursor);
	
	auto processes = kernel.get_process_list();
	
	// 禁用表格更新
	process_table->setUpdatesEnabled(false);
	
	// 清空表格但保持列不变
	process_table->clearContents();
	process_table->setRowCount(processes.size());
	
	// 批量更新所有数据
	for(size_t i = 0; i < processes.size(); i++) {
		QTableWidgetItem* pidItem = new QTableWidgetItem;
		pidItem->setData(Qt::DisplayRole, processes[i]->pid);
		
		QTableWidgetItem* nameItem = new QTableWidgetItem(
			QString::fromStdString(processes[i]->name));
		
		QTableWidgetItem* stateItem = new QTableWidgetItem(
			processes[i]->state == RUNNING ? "Running" :
			processes[i]->state == READY ? "Ready" : "Blocked");
		
		QTableWidgetItem* priorityItem = new QTableWidgetItem;
		priorityItem->setData(Qt::DisplayRole, processes[i]->priority);
		
		QTableWidgetItem* memoryItem = new QTableWidgetItem;
		memoryItem->setData(Qt::DisplayRole, QString::number(processes[i]->virtual_memory_size / 1024.0, 'f', 2) + " KB");
		
		process_table->setItem(i, 0, pidItem);
		process_table->setItem(i, 1, nameItem);
		process_table->setItem(i, 2, stateItem);
		process_table->setItem(i, 3, priorityItem);
		process_table->setItem(i, 4, memoryItem);
	}
	
	process_table->setUpdatesEnabled(true);
	QApplication::restoreOverrideCursor();
}


void MainWindow::create_memory_tab() {
	QWidget* memory_widget = new QWidget;
	QVBoxLayout* layout = new QVBoxLayout;
	
	// 创建内存表格 - 移除页面错误列
	memory_table = new QTableWidget;
	memory_table->setColumnCount(3);  // 改为3列
	memory_table->setHorizontalHeaderLabels({
		"Total Memory", "Used Memory", "Free Memory"
	});
	memory_table->setRowCount(1);
	
	// 创建内存使用图表
	memory_chart_view = new QChartView;
	QChart* chart = new QChart;
	chart->setTitle("Memory Usage Over Time");
	
	// 创建X轴（时间）
	QDateTimeAxis* axisX = new QDateTimeAxis;
	axisX->setFormat("hh:mm:ss");
	axisX->setTitleText("Time");
	axisX->setGridLineVisible(true);
	
	// 创建Y轴（内存使用百分比）
	QValueAxis* axisY = new QValueAxis;
	axisY->setRange(0, 100);
	axisY->setTitleText("Memory Usage (%)");
	axisY->setLabelFormat("%.1f%%");
	axisY->setGridLineVisible(true);
	
	// 创建内存使用数据序列
	QLineSeries* used_memory_series = new QLineSeries;
	used_memory_series->setName("Used Memory");
	
	chart->addSeries(used_memory_series);
	chart->addAxis(axisX, Qt::AlignBottom);
	chart->addAxis(axisY, Qt::AlignLeft);
	used_memory_series->attachAxis(axisX);
	used_memory_series->attachAxis(axisY);
	
	memory_chart_view->setChart(chart);
	memory_chart_view->setRenderHint(QPainter::Antialiasing);
	
	// 设置表格和图表的大小比例
	memory_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	memory_table->setFixedHeight(100);
	memory_chart_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	memory_chart_view->setMinimumHeight(300);  // 设置最小高度确保图表可见
	
	layout->addWidget(memory_table);
	layout->addWidget(memory_chart_view);
	
	memory_widget->setLayout(layout);
	tab_widget->addTab(memory_widget, "Memory");
	
	// 设置列自动拉伸
	for(int i = 0; i < memory_table->columnCount(); i++) {
		memory_table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
	}
}

void MainWindow::create_network_tab() {
	QWidget* network_widget = new QWidget;
	QVBoxLayout* layout = new QVBoxLayout;
	
	// 创建网络表格
	network_table = new QTableWidget;
	network_table->setColumnCount(4);
	network_table->setHorizontalHeaderLabels({
		"Client ID", "IP Address", "Data Sent", "Data Received"
	});
	
	// 创建网络流量图表
	network_chart_view = new QChartView;
	QChart* chart = new QChart;
	chart->setTitle("Network Traffic");
	chart->setAnimationOptions(QChart::NoAnimation); // 禁用动画以提高实时性能
	
	// 创建时间轴
	QDateTimeAxis* axisX = new QDateTimeAxis;
	axisX->setTickCount(5);
	axisX->setFormat("hh:mm:ss");
	axisX->setTitleText("Time");
	
	// 创建数值轴
	QValueAxis* axisY = new QValueAxis;
	axisY->setRange(0, 100);
	axisY->setLabelFormat("%.2f KB");
	axisY->setTitleText("Data Transfer");
	
	// 创建数据系列
	QLineSeries* sent_series = new QLineSeries;
	QLineSeries* received_series = new QLineSeries;
	
	sent_series->setName("Data Sent");
	received_series->setName("Data Received");
	
	// 设置线条样式
	QPen pen_sent(Qt::blue);
	pen_sent.setWidth(2);
	sent_series->setPen(pen_sent);
	
	QPen pen_received(Qt::red);
	pen_received.setWidth(2);
	received_series->setPen(pen_received);
	
	// 添加初始数据点
	QDateTime current = QDateTime::currentDateTime();
	qint64 current_msecs = current.toMSecsSinceEpoch();
	sent_series->append(current_msecs, 0);
	received_series->append(current_msecs, 0);
	
	// 添加系列到图表
	chart->addSeries(sent_series);
	chart->addSeries(received_series);
	
	// 添加坐标轴
	chart->addAxis(axisX, Qt::AlignBottom);
	chart->addAxis(axisY, Qt::AlignLeft);
	
	sent_series->attachAxis(axisX);
	sent_series->attachAxis(axisY);
	received_series->attachAxis(axisX);
	received_series->attachAxis(axisY);
	
	// 设置图表外观
	chart->legend()->setVisible(true);
	chart->legend()->setAlignment(Qt::AlignBottom);
	
	network_chart_view->setChart(chart);
	network_chart_view->setRenderHint(QPainter::Antialiasing);
	network_chart_view->setMinimumHeight(300);
	
	// 布局设置
	layout->addWidget(network_table);
	layout->addWidget(network_chart_view, 1);
	
	network_widget->setLayout(layout);
	tab_widget->addTab(network_widget, "Network");
}

// 在 mainwindow.cpp 中修改 update_displays 函数
void MainWindow::update_displays() {
	// 只更新当前显示的标签页
	int current_tab = tab_widget->currentIndex();
	
	// 检查更新间隔
	QDateTime now = QDateTime::currentDateTime();
	if(last_update.msecsTo(now) < 1000) {
		return;
	}
	
	last_update = now;
	QApplication::setOverrideCursor(Qt::WaitCursor);
	
	// 禁用所有表格更新
	if(process_table) process_table->setUpdatesEnabled(false);
	if(memory_table) memory_table->setUpdatesEnabled(false);
	if(file_table) file_table->setUpdatesEnabled(false);
	if(network_table) network_table->setUpdatesEnabled(false);
	if(disk_table) disk_table->setUpdatesEnabled(false);
	if(log_table) log_table->setUpdatesEnabled(false);
	
	// 根据当前标签页选择性更新
	switch(current_tab) {
		case 0: // Processes
		update_process_table_efficient();
		break;
		case 1: // Memory
		update_memory_table();
		break;
		case 2: // Files
		update_file_table_efficient();
		break;
		case 3: // Network
		update_network_table();
		break;
		case 4: // Disk
		update_disk_table();
		break;
		case 5: // Logs
		update_log_table();
		break;
	}
	
	// 恢复所有表格更新
	if(process_table) process_table->setUpdatesEnabled(true);
	if(memory_table) memory_table->setUpdatesEnabled(true);
	if(file_table) file_table->setUpdatesEnabled(true);
	if(network_table) network_table->setUpdatesEnabled(true);
	if(disk_table) disk_table->setUpdatesEnabled(true);
	if(log_table) log_table->setUpdatesEnabled(true);
	
	QApplication::restoreOverrideCursor();
}

void MainWindow::update_memory_table() {
	auto info = kernel.get_system_info();
	memory_table->setRowCount(1);
	
	// 更新表格数据 - 移除页面错误显示
	memory_table->setItem(0, 0, new QTableWidgetItem(
		QString::number(info.total_memory / 1024) + " KB"));
	memory_table->setItem(0, 1, new QTableWidgetItem(
		QString::number(info.used_memory / 1024) + " KB"));
	memory_table->setItem(0, 2, new QTableWidgetItem(
		QString::number(info.free_memory / 1024) + " KB"));
	
	// 更新内存图表
	QChart* chart = memory_chart_view->chart();
	QLineSeries* series = qobject_cast<QLineSeries*>(chart->series().at(0));
	if(series) {
		// 计算内存使用百分比
		qreal used_percentage = (qreal)info.used_memory / info.total_memory * 100;
		qint64 current_time = QDateTime::currentMSecsSinceEpoch();
		
		// 添加新的数据点
		series->append(current_time, used_percentage);
		
		// 保持最近30个数据点
		if(series->count() > 30) {
			series->remove(0);
		}
		
		// 获取X轴和Y轴
		QDateTimeAxis* axisX = qobject_cast<QDateTimeAxis*>(chart->axes(Qt::Horizontal).first());
		QValueAxis* axisY = qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical).first());
		
		if(axisX && axisY) {
			// 更新X轴范围以显示最近的数据点
			if(series->count() > 1) {
				QDateTime first = QDateTime::fromMSecsSinceEpoch(series->at(0).x());
				QDateTime last = QDateTime::fromMSecsSinceEpoch(series->at(series->count()-1).x());
				axisX->setRange(first, last);
			}
			
			// 调整Y轴范围以适应数据
			qreal maxY = 0;
			for(int i = 0; i < series->count(); ++i) {
				maxY = qMax(maxY, series->at(i).y());
			}
			axisY->setRange(0, qMin(100.0, maxY * 1.1));  // 留出10%的余量，但不超过100%
		}
	}
}
// 修改 update_network_table() 函数来修复网络流量显示
void MainWindow::update_network_table() {
	QApplication::setOverrideCursor(Qt::WaitCursor);
	
	auto stats = network.get_stats();
	QChart* chart = network_chart_view->chart();
	
	if (chart->series().size() >= 2) {
		QLineSeries* sent_series = qobject_cast<QLineSeries*>(chart->series().at(0));
		QLineSeries* received_series = qobject_cast<QLineSeries*>(chart->series().at(1));
		
		if (sent_series && received_series) {
			QDateTime current = QDateTime::currentDateTime();
			qint64 current_msecs = current.toMSecsSinceEpoch();
			
			// 添加新的数据点
			sent_series->append(current_msecs, stats.total_bytes_sent / 1024.0);
			received_series->append(current_msecs, stats.total_bytes_received / 1024.0);
			
			// 保持最近30个数据点
			while (sent_series->count() > 30) {
				sent_series->remove(0);
				received_series->remove(0);
			}
			
			// 更新X轴范围
			QDateTimeAxis* axisX = qobject_cast<QDateTimeAxis*>(
				chart->axes(Qt::Horizontal).first());
			if (axisX && sent_series->count() > 1) {
				QDateTime first = QDateTime::fromMSecsSinceEpoch(
					sent_series->at(0).x());
				QDateTime last = QDateTime::fromMSecsSinceEpoch(
					current_msecs);
				axisX->setRange(first, last);
			}
			
			// 更新Y轴范围
			QValueAxis* axisY = qobject_cast<QValueAxis*>(
				chart->axes(Qt::Vertical).first());
			if (axisY) {
				double maxValue = 0;
				for (int i = 0; i < sent_series->count(); ++i) {
					maxValue = qMax(maxValue, sent_series->at(i).y());
					maxValue = qMax(maxValue, received_series->at(i).y());
				}
				axisY->setRange(0, maxValue * 1.2); // 留出20%的余量
			}
		}
	}
	
	// 更新表格显示（保持原有的表格更新代码）
	
	QApplication::restoreOverrideCursor();
}

void MainWindow::update_disk_table() {
	auto partitions = disk.list_partitions();
	disk_table->setRowCount(partitions.size());
	
	// 更新表格数据
	for(size_t i = 0; i < partitions.size(); i++) {
		disk_table->setItem(i, 0, new QTableWidgetItem(
			QString::fromStdString(partitions[i].name)));
		disk_table->setItem(i, 1, new QTableWidgetItem(
			QString::number(partitions[i].size / (1024.0 * 1024.0), 'f', 2) + " MB"));
		disk_table->setItem(i, 2, new QTableWidgetItem(
			QString::number(partitions[i].used_space / (1024.0 * 1024.0), 'f', 2) + " MB"));
		disk_table->setItem(i, 3, new QTableWidgetItem(
			QString::fromStdString(partitions[i].mount_point)));
	}
	
	// 更新饼图
	QChart* chart = disk_chart_view->chart();
	QPieSeries* series = qobject_cast<QPieSeries*>(chart->series().first());
	if(series) {
		series->clear();
		
		for(const auto& partition : partitions) {
			// 计算使用率
			double usage_percent = partition.size > 0 ? 
			(100.0 * partition.used_space / partition.size) : 0.0;
			
			// 添加切片
			QString label = QString::fromStdString(partition.name) + 
			QString(" (%1%)").arg(usage_percent, 0, 'f', 1);
			QPieSlice* slice = series->append(label, partition.used_space);
			
			// 设置颜色
			QColor color;
			if(usage_percent > 90) {
				color = Qt::red;
			} else if(usage_percent > 70) {
				color = QColor(255, 165, 0);
			} else {
				color = QColor(50, 205, 50);
			}
			
			slice->setColor(color);
			slice->setLabelVisible(true);
			slice->setBorderWidth(1);
			slice->setLabelArmLengthFactor(0.15);
		}
	}
}

void MainWindow::create_disk_tab() {
	QWidget* disk_widget = new QWidget;
	QVBoxLayout* layout = new QVBoxLayout;
	
	// 创建磁盘表格
	disk_table = new QTableWidget;
	disk_table->setColumnCount(4);
	disk_table->setHorizontalHeaderLabels({
		"Partition", "Total Size", "Used", "Mount Point"
	});
	
	// 创建饼图
	disk_chart_view = new QChartView;
	QChart* chart = new QChart;
	chart->setTitle("Disk Usage");
	chart->setAnimationOptions(QChart::AllAnimations);
	
	QPieSeries* series = new QPieSeries;
	series->setHoleSize(0.35); // 设置空心饼图效果
	series->setPieSize(0.7);   // 设置饼图大小
	
	// 添加初始数据点以确保图表显示
	series->append("Root", 100);
	QPieSlice* slice = series->slices().first();
	slice->setColor(QColor(0, 128, 0));
	slice->setLabelVisible(true);
	
	chart->addSeries(series);
	chart->legend()->setAlignment(Qt::AlignRight);
	chart->legend()->setVisible(true);
	
	disk_chart_view->setChart(chart);
	disk_chart_view->setRenderHint(QPainter::Antialiasing);
	disk_chart_view->setMinimumHeight(300);
	
	// 布局设置
	layout->addWidget(disk_table);
	layout->addWidget(disk_chart_view, 1);
	layout->addLayout(create_disk_buttons());
	
	disk_widget->setLayout(layout);
	tab_widget->addTab(disk_widget, "Disk");
}

void MainWindow::update_log_table() {
	auto logs = logger.get_recent_logs(100);  // 显示最近100条日志
	log_table->setRowCount(logs.size());
	
	for(size_t i = 0; i < logs.size(); i++) {
		// 格式化时间戳
		QDateTime time;
		time.setSecsSinceEpoch(logs[i].timestamp);
		
		log_table->setItem(i, 0, new QTableWidgetItem(
			time.toString("yyyy-MM-dd hh:mm:ss")));
		log_table->setItem(i, 1, new QTableWidgetItem(
			QString::fromStdString(
				logger.level_to_string(logs[i].level))));
		log_table->setItem(i, 2, new QTableWidgetItem(
			QString::fromStdString(logs[i].message)));
		if(!logs[i].source.empty()) {
			log_table->setItem(i, 3, new QTableWidgetItem(
				QString::fromStdString(logs[i].source)));
		}
	}
}

void MainWindow::handle_login() {
	QString username = QInputDialog::getText(this, "Login", "Username:");
	if(username.isEmpty()) return;
	
	QString password = QInputDialog::getText(this, "Login", "Password:",
		QLineEdit::Password);
	if(password.isEmpty()) return;
	
	if(auth.login(username.toStdString(), password.toStdString())) {
		logger.info("User logged in: " + username.toStdString());
		emit login_status_changed(true);
		statusBar()->showMessage("Logged in as: " + username);
	} else {
		QMessageBox::warning(this, "Login Failed", 
			"Invalid username or password");
		logger.warning("Login failed for user: " + username.toStdString());
	}
}

void MainWindow::handle_logout() {
	auth.logout();
	logger.info("User logged out");
	emit login_status_changed(false);
	statusBar()->showMessage("Logged out");
}

void MainWindow::handle_about() {
	QMessageBox::about(this, "About",
		"Advanced Operating System\n"
		"Version 1.0\n"
		"A demonstration of operating system concepts");
}

void MainWindow::handle_settings() {
	// TODO: Implement settings dialog
}

void MainWindow::create_command_panel() {
	QWidget* panel = new QWidget;
	QHBoxLayout* layout = new QHBoxLayout;
	
	command_input = new QLineEdit;
	execute_button = new QPushButton("Execute");
	
	layout->addWidget(command_input);
	layout->addWidget(execute_button);
	
	panel->setLayout(layout);
	statusBar()->addWidget(panel);
	
	connect(execute_button, &QPushButton::clicked, this, &MainWindow::execute_command);
	connect(command_input, &QLineEdit::returnPressed, this, &MainWindow::execute_command);
}

void MainWindow::create_log_tab() {
	QWidget* log_widget = new QWidget;
	QVBoxLayout* layout = new QVBoxLayout;
	
	log_table = new QTableWidget;
	log_table->setColumnCount(4);
	log_table->setHorizontalHeaderLabels({
		"Time", "Level", "Message", "Source"
	});
	
	QHBoxLayout* button_layout = new QHBoxLayout;
	QPushButton* clear_btn = new QPushButton("Clear Logs");
	QPushButton* export_btn = new QPushButton("Export Logs");
	
	button_layout->addWidget(clear_btn);
	button_layout->addWidget(export_btn);
	
	layout->addWidget(log_table);
	layout->addLayout(button_layout);
	
	log_widget->setLayout(layout);
	tab_widget->addTab(log_widget, "Logs");
	
	connect(clear_btn, &QPushButton::clicked, [this]() {
		logger.clear_logs();
		update_log_table();
	});
	
	connect(export_btn, &QPushButton::clicked, [this]() {
		QString filename = QFileDialog::getSaveFileName(this,
			"Export Logs", "", "Log Files (*.log);;All Files (*)");
		if(!filename.isEmpty()) {
			logger.export_logs(filename.toStdString());
		}
	});
}

void MainWindow::execute_command() {
	QString cmd = command_input->text();
	if(cmd.isEmpty()) return;
	
	logger.info("Executing command: " + cmd.toStdString());
	// TODO: 实现命令执行逻辑
	
	command_input->clear();
}

void MainWindow::update_charts() {
	// Charts already updated in their respective update methods
}

void MainWindow::create_tabs() {
	tab_widget = new QTabWidget;
	setCentralWidget(tab_widget);
	
	create_process_tab();
	create_memory_tab();
	create_file_tab();
	create_network_tab();
	create_disk_tab();
	create_log_tab();
}
void MainWindow::update_file_table() {
	QApplication::setOverrideCursor(Qt::WaitCursor);
	
	// 清理旧的项目以防内存泄漏
	for(int i = 0; i < file_table->rowCount(); i++) {
		for(int j = 0; j < file_table->columnCount(); j++) {
			QTableWidgetItem* item = file_table->item(i, j);
			if(item) delete item;
		}
	}
	
	auto files = disk.list_files();
	file_table->setRowCount(files.size());
	
	// 每处理10行就让UI有机会响应
	for(size_t i = 0; i < files.size(); i++) {
		file_table->setItem(i, 0, new QTableWidgetItem(
			QString::fromStdString(files[i].name)));
		file_table->setItem(i, 1, new QTableWidgetItem(
			QString::fromStdString(files[i].type)));
		file_table->setItem(i, 2, new QTableWidgetItem(
			QString::number(files[i].size / 1024.0, 'f', 2) + " KB"));
		
		QDateTime modified;
		modified.setSecsSinceEpoch(files[i].modified_time);
		file_table->setItem(i, 3, new QTableWidgetItem(
			modified.toString("yyyy-MM-dd hh:mm:ss")));
		
		if(i % 10 == 0) {
			QApplication::processEvents();  // 让UI有机会响应
		}
	}
	
	QApplication::restoreOverrideCursor();
}

void MainWindow::create_file_tab() {
	QWidget* file_widget = new QWidget;
	QVBoxLayout* layout = new QVBoxLayout;
	
	// 创建文件表格
	file_table = new QTableWidget;
	file_table->setColumnCount(4);
	file_table->setHorizontalHeaderLabels({
		"Name", "Type", "Size", "Modified"
	});
	
	// 设置表格选择模式为整行选择
	file_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	file_table->setSelectionMode(QAbstractItemView::SingleSelection);
	
	QHBoxLayout* button_layout = new QHBoxLayout;
	QPushButton* new_file_btn = new QPushButton("New File");
	QPushButton* new_dir_btn = new QPushButton("New Directory");
	QPushButton* delete_btn = new QPushButton("Delete");
	
	button_layout->addWidget(new_file_btn);
	button_layout->addWidget(new_dir_btn);
	button_layout->addWidget(delete_btn);
	
	// 连接新建文件按钮
	connect(new_file_btn, &QPushButton::clicked, [this]() {
		// 创建对话框
		QDialog* dialog = new QDialog(this);
		dialog->setWindowTitle("Create New File");
		QVBoxLayout* layout = new QVBoxLayout(dialog);
		
		// 创建输入控件
		QLineEdit* filename_input = new QLineEdit(dialog);
		filename_input->setPlaceholderText("Enter file name");
		QTextEdit* content_input = new QTextEdit(dialog);
		content_input->setPlaceholderText("Enter file content");
		
		// 添加到布局
		layout->addWidget(new QLabel("File Name:"));
		layout->addWidget(filename_input);
		layout->addWidget(new QLabel("Content:"));
		layout->addWidget(content_input);
		
		// 添加按钮
		QDialogButtonBox* buttonBox = new QDialogButtonBox(
			QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		layout->addWidget(buttonBox);
		
		connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
		connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
		
		dialog->setFixedSize(400, 300);
		
		if(dialog->exec() == QDialog::Accepted) {
			QString filename = filename_input->text();
			QString content = content_input->toPlainText();
			
			if(!filename.isEmpty()) {
				bool success = disk.write_file(filename.toStdString(), 
					content.toStdString());
				if(success) {
					logger.info("Created new file: " + filename.toStdString());
					update_file_table();
				} else {
					QMessageBox::warning(this, "Error", "Failed to create file");
				}
			}
		}
		
		delete dialog;
	});
		
	// 连接新建目录按钮
	connect(new_dir_btn, &QPushButton::clicked, [this, new_dir_btn]() {
		// 禁用按钮防止重复点击
		new_dir_btn->setEnabled(false);
		
		// 创建对话框获取目录名
		QDialog* dialog = new QDialog(this);
		dialog->setWindowTitle("Create New Directory");
		QVBoxLayout* layout = new QVBoxLayout(dialog);
		
		// 创建输入控件
		QLineEdit* dirname_input = new QLineEdit(dialog);
		dirname_input->setPlaceholderText("Enter directory name");
		
		// 添加到布局
		layout->addWidget(new QLabel("Directory Name:"));
		layout->addWidget(dirname_input);
		
		// 添加按钮
		QDialogButtonBox* buttonBox = new QDialogButtonBox(
			QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		layout->addWidget(buttonBox);
		
		connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
		connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
		
		dialog->setFixedSize(300, 150);
		
		if(dialog->exec() == QDialog::Accepted) {
			QString dirname = dirname_input->text();
			
			if(!dirname.isEmpty()) {
				// 使用 QTimer::singleShot 延迟执行目录创建操作
				QTimer::singleShot(0, this, [this, dirname, new_dir_btn]() {
					bool success = disk.create_directory(dirname.toStdString());
					if(success) {
						logger.info("Created new directory: " + dirname.toStdString());
						// 使用 QTimer::singleShot 延迟刷新界面
						QTimer::singleShot(0, this, [this]() {
							update_file_table();
						});
					} else {
						QMessageBox::warning(this, "Error", "Failed to create directory");
					}
					new_dir_btn->setEnabled(true);
				});
			} else {
				new_dir_btn->setEnabled(true);
			}
		} else {
			new_dir_btn->setEnabled(true);
		}
		
		delete dialog;
	});
	
	// 连接删除按钮
	connect(delete_btn, &QPushButton::clicked, [this, delete_btn]() {
		// 获取选中的项目
		QList<QTableWidgetItem*> selected = file_table->selectedItems();
		if(selected.isEmpty()) {
			QMessageBox::warning(this, "Warning", "Please select a file or directory first");
			return;
		}
		
		// 获取选中项的信息
		int row = file_table->row(selected[0]);
		QString name = file_table->item(row, 0)->text();
		QString type = file_table->item(row, 1)->text();
		
		// 确认删除
		if(QMessageBox::question(this, "Confirm Delete",
			QString("Are you sure you want to delete %1 '%2'?")
			.arg(type.toLower())
			.arg(name),
			QMessageBox::Yes | QMessageBox::No,
			QMessageBox::No) != QMessageBox::Yes) {
			return;
		}
		
		// 禁用按钮防止重复点击
		delete_btn->setEnabled(false);
		
		// 异步执行删除操作
		QTimer::singleShot(0, this, [this, name, type, delete_btn]() {
			bool success;
			if(type.contains("Directory", Qt::CaseInsensitive)) {
				success = disk.delete_directory(name.toStdString());
			} else {
				success = disk.delete_file(name.toStdString());
			}
			
			if(success) {
				logger.info("Deleted " + type.toLower().toStdString() + ": " + name.toStdString());
				// 使用 QTimer::singleShot 延迟刷新界面
				QTimer::singleShot(0, this, [this]() {
					update_file_table();
				});
			} else {
				QMessageBox::warning(this, "Error", 
					QString("Failed to delete %1").arg(name));
				logger.warning("Failed to delete " + type.toLower().toStdString() + ": " + name.toStdString());
			}
			delete_btn->setEnabled(true);
		});
	});
	
	// 连接新建目录按钮和删除按钮的代码保持不变...
	
	layout->addWidget(file_table);
	layout->addLayout(button_layout);
	
	file_widget->setLayout(layout);
	tab_widget->addTab(file_widget, "Files");
	
	// 设置列自动拉伸
	for(int i = 0; i < file_table->columnCount(); i++) {
		file_table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
	}
}

// Add this function to mainwindow.cpp
void MainWindow::update_file_table_efficient() {
	QApplication::setOverrideCursor(Qt::WaitCursor);
	
	auto files = disk.list_files();
	
	// Disable table updates while we make changes
	file_table->setUpdatesEnabled(false);
	
	// Clear contents but keep the columns
	file_table->clearContents();
	file_table->setRowCount(files.size());
	
	// Batch update all data
	for(size_t i = 0; i < files.size(); i++) {
		QTableWidgetItem* nameItem = new QTableWidgetItem(
			QString::fromStdString(files[i].name));
		
		QTableWidgetItem* typeItem = new QTableWidgetItem(
			QString::fromStdString(files[i].type));
		
		QTableWidgetItem* sizeItem = new QTableWidgetItem;
		sizeItem->setData(Qt::DisplayRole, 
			QString::number(files[i].size / 1024.0, 'f', 2) + " KB");
		
		QDateTime modified;
		modified.setSecsSinceEpoch(files[i].modified_time);
		QTableWidgetItem* modifiedItem = new QTableWidgetItem(
			modified.toString("yyyy-MM-dd hh:mm:ss"));
		
		file_table->setItem(i, 0, nameItem);
		file_table->setItem(i, 1, typeItem);
		file_table->setItem(i, 2, sizeItem);
		file_table->setItem(i, 3, modifiedItem);
	}
	
	file_table->setUpdatesEnabled(true);
	QApplication::restoreOverrideCursor();
}

QHBoxLayout* MainWindow::create_disk_buttons() {
	QHBoxLayout* button_layout = new QHBoxLayout;
	
	QPushButton* format_btn = new QPushButton("Format");
	QPushButton* mount_btn = new QPushButton("Mount");
	QPushButton* unmount_btn = new QPushButton("Unmount");
	
	format_btn->setFixedWidth(100);
	mount_btn->setFixedWidth(100);
	unmount_btn->setFixedWidth(100);
	
	button_layout->addStretch();
	button_layout->addWidget(format_btn);
	button_layout->addWidget(mount_btn);
	button_layout->addWidget(unmount_btn);
	button_layout->addStretch();
	
	// 连接按钮信号槽（保持原有的连接代码）
	
	return button_layout;
}
