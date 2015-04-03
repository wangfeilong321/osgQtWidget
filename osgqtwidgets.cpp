#include "osgqtwidgets.h"

#include <QHBoxLayout>
#include <osgDB/WriteFile>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgViewer/ViewerEventHandlers>
#include <osgDB/ReadFile>
#include <osg/Point>

#include <osg/Notify>
#include <osg/Endian>

#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReaderWriter>

#include <osgUtil/TriStripVisitor>
#include <osgUtil/SmoothingVisitor>
#include <osg/TriangleFunctor>
#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/LightSource>

#include <string.h>

#include <memory>

OsgWidget::OsgWidget(QWidget *parent)
{
    setThreadingModel(osgViewer::Viewer::SingleThreaded);
    m_root=new osg::Group;

    m_widget=addViewWidget(createCamera(0,0,100,100));

    addEventHandler(new osgViewer::StatsHandler());
    addEventHandler( new osgGA::StateSetManipulator(getCamera()->getOrCreateStateSet()) );

    setSceneData(m_root);
}

QWidget* OsgWidget::addViewWidget(osg::Camera* camera)
{
    setCamera( camera );
    setCameraManipulator( new osgGA::TrackballManipulator );

    osgQt::GraphicsWindowQt* gw = dynamic_cast<osgQt::GraphicsWindowQt*>( camera->getGraphicsContext() );
    return gw ? gw->getGLWidget() : NULL;
}

osg::Camera* OsgWidget::createCamera( int x, int y, int w, int h )
{
    osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->windowName = "SceneView";
    traits->windowDecoration = false;
    traits->x = x;
    traits->y = y;
    traits->width = w;
    traits->height = h;
    traits->doubleBuffer = true;
    traits->alpha = ds->getMinimumNumAlphaBits();
    traits->stencil = ds->getMinimumNumStencilBits();
    traits->sampleBuffers = ds->getMultiSamples();
    traits->samples = ds->getNumMultiSamples();

    osg::Camera * camera = getCamera ();
    camera->setGraphicsContext( new osgQt::GraphicsWindowQt( traits.get() ) );

    camera->setClearColor( osg::Vec4(0.2, 0.2, 0.6, 1.0) );
    camera->setViewport( new osg::Viewport(0, 0, traits->width, traits->height) );
    camera->setProjectionMatrixAsPerspective(
                30.0f, static_cast<double>(traits->width) / static_cast<double>(traits->height), 1.0f, 10000.0f );
    return camera;
}

QWidget* OsgWidget::getWidget()
{
    return m_widget;
}

void OsgWidget::updateScene( osg::Node* node)
{
    if(m_root->getNumChildren())
        m_root->replaceChild(m_root->getChild(0),node);
    else
        m_root->addChild(node);
        getCameraManipulator()->home(0);
}

MainWidget::MainWidget()
{
    OsgWidget* viewer=new OsgWidget;
    QWidget* widget=viewer->getWidget();
    widget->setMinimumSize(QSize(100,100));
    setCentralWidget(widget);
    m_viewThread.reset(new ViewerFrameThread(viewer));
    m_updateOperation=new UpdateOperation();
    viewer->addUpdateOperation(m_updateOperation);
    m_viewThread->start();
    setAcceptDrops(true);

    QMenu* fileMenu = menuBar()->addMenu("&File");
    QAction* openAct = new QAction("&Open",this);
    fileMenu->addAction(openAct);
    connect(openAct,SIGNAL(triggered()),this,SLOT(openFile()));
}

void MainWidget::dropEvent( QDropEvent *event )
{
    if(event->mimeData()->hasFormat("text/uri-list")){
        QString fileName = event->mimeData()->urls().first().toLocalFile();
        m_updateOperation->updateScene(fileName.toStdString());
    }
}

void MainWidget::openFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          "Open File", QString(), "STL Files (*.*)");
    m_updateOperation->updateScene(fileName.toStdString());
}

void MainWidget::dragEnterEvent( QDragEnterEvent *event )
{
    if (event->mimeData()->hasFormat("text/uri-list"))
        event->acceptProposedAction();
}

UpdateOperation::UpdateOperation()
    :osg::Operation("update operation",true)
    ,m_loadedFlag(true)
{

}

void UpdateOperation::updateScene( const std::string& name)
{
    if(m_nodeFileName!=name){
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        m_nodeFileName=name;
        m_loadedFlag=false;
    }
}

void UpdateOperation::operator()( osg::Object* callingObject )
{
    // decided which method to call according to whole has called me.
    if(m_loadedFlag)
        return;
    OsgWidget* viewer = dynamic_cast<OsgWidget*>(callingObject);
    if (viewer&&!m_nodeFileName.empty()){
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        osgDB::ReaderWriter::ReadResult r = osgDB::readNodeFile (m_nodeFileName);
        osg::ref_ptr<osg::Node> node = r.getNode();
        if(node){
            viewer->updateScene(node);
            OSG_WARN<<m_nodeFileName<<" load successfully.\n";
        }else{
            OSG_WARN<<m_nodeFileName<<" load failed.\n";
        }
        m_loadedFlag=true;
    }
}


ViewerFrameThread::~ViewerFrameThread()
{
    cancel();
    while(isRunning())
    {
        OpenThreads::Thread::YieldCurrentThread();
    }
}

int ViewerFrameThread::cancel()
{
    _viewerBase->setDone(true);
    return 0;
}

void ViewerFrameThread::run()
{
    _viewerBase->run();
}

#include "moc_osgqtwidgets.cpp"