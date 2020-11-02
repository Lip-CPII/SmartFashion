#ifndef LP_MAINWINDOW_H
#define LP_MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class LP_MainWindow; }
QT_END_NAMESPACE

class QPluginLoader;

class LP_MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    LP_MainWindow(QWidget *parent = nullptr);
    ~LP_MainWindow();

protected:
    void loadRenderers();
    void loadCommandHistory();
    void loadDocuments();
    void loadFunctionals();
    void loadSelector();
    void loadPlugins(const QString &path, QMenuBar *menubar);

    // QWidget interface
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::LP_MainWindow *ui;
    std::unique_ptr<QPluginLoader> mPluginLoader;


};
#endif // LP_MAINWINDOW_H
