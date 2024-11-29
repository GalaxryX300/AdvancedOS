# ifndef MAINWINDOW_H
# define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QChartView>
#include <QLineSeries>
#include <QPieSeries>
#include <QVBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QDateTime>

#include "advanced_kernel.h"
#include "network.h"
#include "user_auth.h"
#include "disk_manager.h"
#include "logger.h"
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QPieSeries>
#include <QApplication>
#include <QDialog>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QLabel>

using namespace QtCharts;

class MainWindow : public QMainWindow {
    Q_OBJECT
    
private:
    AdvancedKernel& kernel;
    NetworkManager& network;
    UserAuth& auth;
    DiskManager& disk;
    SystemLogger& logger;
    
    // Add last_update member variable
    QDateTime last_update;
    
    // GUI elements
    QTabWidget* tab_widget;
    QTableWidget* process_table;
    QTableWidget* memory_table;
    QTableWidget* file_table;
    QTableWidget* network_table;
    QTableWidget* disk_table;
    QTableWidget* log_table;
    QLineEdit* command_input;
    QPushButton* execute_button;
    
    // Charts
    QChartView* memory_chart_view;
    QChartView* network_chart_view;
    QChartView* disk_chart_view;
    
    QTimer* update_timer;
    QHBoxLayout* create_disk_buttons();
    
    // Initialization methods
    void setup_ui();
    void create_menus();
    void create_tabs();
    void create_process_tab();
    void create_memory_tab();
    void create_file_tab();
    void create_network_tab();
    void create_disk_tab();
    void create_log_tab();
    void create_command_panel();
    
    // Update methods
    void update_process_table();
    void update_process_table_efficient();  // Add efficient update method
    void update_memory_table();
    void update_file_table();
    void update_file_table_efficient();
    void update_network_table();
    void update_disk_table();
    void update_log_table();
    void update_charts();
    
    // Event handlers
    void handle_process_action(const QString& action);
    void handle_file_action(const QString& action);
    void handle_network_action(const QString& action);
    void handle_disk_action(const QString& action);
    
private slots:
    void update_displays();
    void execute_command();
    void handle_login();
    void handle_logout();
    void handle_about();
    void handle_settings();
    
public:
    MainWindow(AdvancedKernel& k, NetworkManager& n, UserAuth& a, 
               DiskManager& d, SystemLogger& l, QWidget *parent = nullptr);
    ~MainWindow();
    
signals:
    void login_status_changed(bool logged_in);
    void system_status_changed(const QString& status);
};

#endif // MAINWINDOW_H