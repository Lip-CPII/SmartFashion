#include "lp_mainwindow.h"
#include "ui_lp_mainwindow.h"
#include "plugin/lp_actionplugin.h"

#include "Functionals/lp_functionalregistry.h"
#include "Functionals/lp_functional.h"

#include "LP_Registry.h"

#include "Commands/lp_command.h"
#include "Commands/lp_commandmanager.h"

#include "renderer/lp_glrenderer.h"
#include "renderer/lp_glselector.h"

#include "lp_document.h"

#include <QDir>
#include <QPluginLoader>


LP_MainWindow::LP_MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::LP_MainWindow)
{
    ui->setupUi(this);

    //Renderers
    loadRenderers();

    //Selector
    loadSelector();

    //Load all plugins
    loadPlugins("plugins", ui->menubar);

    //Undo
    loadCommandHistory();

    //Functions
    loadFunctionals();

    //Doc
    loadDocuments();
}

LP_MainWindow::~LP_MainWindow()
{
    delete ui;
}

void LP_MainWindow::loadRenderers()
{
    std::function<void(LP_OpenGLWidget*, LP_GLRenderer*)> linkWidgetRenderer =
            [](LP_OpenGLWidget *w, LP_GLRenderer *r){
        w->connect(w, &LP_OpenGLWidget::initGL, r,
                         &LP_GLRenderer::initGL,Qt::BlockingQueuedConnection);
        w->connect(w, &LP_OpenGLWidget::reshapeGL, r,
                         &LP_GLRenderer::reshapeGL,Qt::QueuedConnection);
        r->connect(r, SIGNAL(textureUpdated(QImage)), w,
                         SLOT(updateTexture(QImage)), Qt::QueuedConnection);
    };

    auto renderer = LP_GLRenderer::Create("Shade");
    ui->openGLWidget->SetRenderer(renderer);   //Fixed NameType
    linkWidgetRenderer(ui->openGLWidget, renderer);

    renderer = LP_GLRenderer::Create("Normal");
    ui->openGLWidget_2->SetRenderer(renderer);   //Fixed NameType
    linkWidgetRenderer(ui->openGLWidget_2, renderer);
}

void LP_MainWindow::loadCommandHistory()
{
    LP_CommandGroup::gCommandGroup->setActiveStack(LP_CommandStack::gCommands.get());

    ui->undoView->setGroup(LP_CommandGroup::gCommandGroup.get());
    ui->undoView->setEmptyLabel(tr("Command History"));
    auto action = LP_CommandGroup::gCommandGroup->createUndoAction(nullptr);
    action->setShortcut(QKeySequence::Undo);
    ui->menuEdit->addAction(action);
    action = LP_CommandGroup::gCommandGroup->createRedoAction(nullptr);
    action->setShortcut(QKeySequence::Redo);
    ui->menuEdit->addAction(action);


    connect(LP_CommandGroup::gCommandGroup->ActiveStack(),
            &QUndoStack::indexChanged,qApp,
            []([[maybe_unused]]int idx){
                QApplication::restoreOverrideCursor();
                if ( !QApplication::overrideCursor()){
                    //qFatal("Event filter removed");
                    qApp->removeEventFilter(LP_CommandGroup::gCommandGroup.get());
                }
                LP_Functional::ClearCurrent();
            });
}

void LP_MainWindow::loadDocuments()
{
    //TODO provide a doc. manager for doc. switching, signaling etc.
    connect(&LP_Document::gDoc, &LP_Document::updateTreeView,
            ui->treeView, [&](){
        //Create a model
        auto pDoc = &LP_Document::gDoc;
        auto tree = ui->treeView;

        auto model = pDoc->ToQStandardModel();
        if ( model ){
            delete tree->model();
            tree->setModel(model);
        }
    },Qt::QueuedConnection);

    connect(&LP_Document::gDoc, &LP_Document::requestGLSetup,
            ui->openGLWidget->Renderer(), &LP_GLRenderer::initObjectGL);
    connect(&LP_Document::gDoc, &LP_Document::requestGLCleanup,
            ui->openGLWidget->Renderer(), &LP_GLRenderer::destroyObjectGL);
    connect(&LP_Document::gDoc, &LP_Document::requestGLSetup,
            ui->openGLWidget_2->Renderer(), &LP_GLRenderer::initObjectGL);
    connect(&LP_Document::gDoc, &LP_Document::requestGLCleanup,
            ui->openGLWidget_2->Renderer(), &LP_GLRenderer::destroyObjectGL);
}

void LP_MainWindow::loadFunctionals()
{
    auto pAct = LP_FunctionalsRegistry::Registry();

    for ( auto it = pAct->begin(); it != pAct->cend(); ++it ){
        auto menusubs = it.value().first.split('/');

        std::function<void(QObject*, QList<QByteArray>&, QAction*)> addAction;
        addAction = [&](QObject *parent, QList<QByteArray> &menusubs, QAction *act){
            auto menu = qobject_cast<QMenu*>(parent);
            if ( menusubs.empty()){
                menu->addAction(act);
                return;
            }
            auto cur = menusubs.takeFirst();
            menu = parent->findChild<QMenu*>(cur);
            if ( !menu ){
                auto tmp = qobject_cast<QMenu*>(parent);
                if ( tmp ){
                    menu = tmp->addMenu(cur);
                    menu->setTitle(cur.right(cur.length() - 4));
                }else{//Request for a new menu in the menubar
                    auto tmp2 = qobject_cast<QMenuBar*>(parent);
                    menu = tmp2->addMenu(cur);
                    menu->setTitle(cur.right(cur.length() - 4));    //Minus the "menu"
                }
            }
            addAction(menu, menusubs, act);
        };
        auto e = this->findChild<QAction*>(it.value().second->objectName());
        if ( !e ){  //If not exist in the GUI
            e = it.value().second;
            addAction(ui->menubar, menusubs, e);
        }else{  //Move the functional
            e->setData(std::move(it.value().second->data()));
        }
        connect(e, &QAction::triggered,[e,this](bool checked){
            Q_UNUSED(checked)
            LP_Functional::ClearCurrent();      //For plugin, current will be destructed when PluginLoader::unload()
            auto f = e->data().value<RegFuncPtr>()(nullptr);

            auto dockui = f->DockUi();
            if ( dockui ){
                ui->scroll_dockui->layout()->addWidget(dockui);
                ui->tabWidget->setCurrentIndex(1);
            }else{
                ui->tabWidget->setCurrentIndex(0);
            }

            ui->openGLWidget->installEventFilter(f);
            connect(ui->openGLWidget->Renderer(),
                    &LP_GLRenderer::postRender,
                    f,
                    &LP_Functional::FunctionalRender,
                    Qt::DirectConnection);
            connect(f, &LP_Functional::glUpdateRequest,
                    ui->openGLWidget->Renderer(), &LP_GLRenderer::updateGL
                    ,Qt::QueuedConnection);
            connect(f,
                    &LP_Functional::glContextRequest,
                    ui->openGLWidget->Renderer(),
                    &LP_GLRenderer::glContextResponse,
                    Qt::BlockingQueuedConnection);
            connect(f,
                    &LP_ActionPlugin::destroyed,
                    f,
                    [this,f](){
                        ui->openGLWidget->removeEventFilter(f);
                    },Qt::DirectConnection);

            LP_Functional::SetCurrent(f);

            bool rc = f->Run();
            qDebug() << rc;
        });
    }
}

void LP_MainWindow::loadSelector()
{

}

void LP_MainWindow::loadPlugins(const QString &path, QMenuBar *menubar)
{
    if ( !mPluginLoader ){
        mPluginLoader = std::make_unique<QPluginLoader>();
    }
    QDir pluginsDir(path);
    const auto entryList = pluginsDir.entryList(QDir::Files | QDir::NoSymLinks);
    for (const QString &fileName : entryList) {
        QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
        QObject *plugin = loader.instance();
        auto action = qobject_cast<LP_ActionPlugin*>(plugin);
        if (action) {
            auto menuName = action->MenuName();
            auto menu = menubar->findChild<QMenu*>( menuName );
            if ( !menu ){
                menu = new QMenu( menuName.right( menuName.length() - 4), ui->menubar );
                menu->setObjectName( menuName );
                menubar->addMenu(menu);
            }
            auto trigger = action->Trigger();
            trigger->setData(pluginsDir.absoluteFilePath(fileName));    //Store the plugin path
            menu->addAction(trigger);
            connect(trigger, &QAction::triggered,
                    [this, trigger](bool checked){
                Q_UNUSED(checked)
                LP_Functional::ClearCurrent();      //For plugin, current will be destructed when PluginLoader::unload()
                if ( mPluginLoader->isLoaded()){
                    bool rc = mPluginLoader->unload();
                    Q_ASSERT(rc);
                }
                mPluginLoader->setFileName(trigger->data().toString());
                if ( !mPluginLoader->load()){
                    return;
                }
                QObject *plugin = mPluginLoader->instance();
                auto action = qobject_cast<LP_ActionPlugin*>(plugin);

                auto dockui = action->DockUi();
                if ( dockui ){
                    ui->scroll_dockui->layout()->addWidget(dockui);
                    ui->tabWidget->setCurrentIndex(1);
                }else{
                    ui->tabWidget->setCurrentIndex(0);
                }

                ui->openGLWidget->installEventFilter(action);

                connect(ui->openGLWidget->Renderer(),
                        &LP_GLRenderer::postRender,
                        action,
                        &LP_ActionPlugin::FunctionalRender,
                        Qt::DirectConnection);
                connect(action, &LP_ActionPlugin::glUpdateRequest,
                        ui->openGLWidget->Renderer(), &LP_GLRenderer::updateGL
                        ,Qt::QueuedConnection);
                connect(action,
                        &LP_ActionPlugin::glContextRequest,
                        ui->openGLWidget->Renderer(),
                        &LP_GLRenderer::glContextResponse,
                        Qt::BlockingQueuedConnection);
                connect(action,
                        &LP_ActionPlugin::destroyed,
                        action,
                        [this,action](){
                            ui->openGLWidget->removeEventFilter(action);
                        },Qt::DirectConnection);
                LP_Functional::SetCurrent(action);
                action->Run();
            });

            qDebug() << plugin;
            loader.unload();
        }
    }

//    std::function<void(QObject*,QString)> enumerateChildren;
//    enumerateChildren = [&enumerateChildren](QObject *o, QString indent){
//        qDebug() << tr("%1%2").arg(indent).arg(o->objectName()) << o;
//        for ( auto c : o->children()){
//            enumerateChildren(c, indent+"-");
//        }
//    };
//    enumerateChildren(ui->menubar,"");
}



void LP_MainWindow::closeEvent(QCloseEvent *event)
{
    if ( QFile::exists("tmp")){
        QFile::remove("tmp");
    }

    LP_Functional::ClearCurrent();
    if ( mPluginLoader->isLoaded()){    //If a plugin is current running
        mPluginLoader->unload();
    }

    QMainWindow::closeEvent(event);
}
