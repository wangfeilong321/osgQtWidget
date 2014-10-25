#ifndef _OSGWIDGET_H_
#define _OSGWIDGET_H_
 
#include <QtGui/QtGui>

#include <osgViewer/Viewer>
#include <osgGA/StateSetManipulator>
#include <osgQt/GraphicsWindowQt>
#include <OpenThreads/Thread>
#include <osg/Light>
 
class ViewerFrameThread : public OpenThreads::Thread
{
 public:
 
 ViewerFrameThread(osgViewer::ViewerBase* viewerBase):
  _viewerBase(viewerBase){}
 
  ~ViewerFrameThread();
  int cancel();
  void run();
  osg::ref_ptr<osgViewer::ViewerBase> _viewerBase;
};
 
class OsgWidget :public osgViewer::Viewer
{
 public:
 
  OsgWidget(QWidget *parent = NULL);
  virtual ~OsgWidget(void) {}
 
  QWidget* getWidget();
  void updateScene(osg::Node*);
 protected:
  QWidget* addViewWidget(osg::Camera*);
  osg::Camera* createCamera( int x, int y, int w, int h );

 private:
  QWidget* m_widget;
  osg::Group* m_root;
  osg::Light* myLight1;
};
 
class UpdateOperation : public osg::Operation
{
 public:
  UpdateOperation();
  void updateScene(const std::string&);
  void operator () (osg::Object* callingObject);
 private:
  std::string m_nodeFileName;
  bool m_loadedFlag;
  OpenThreads::Mutex                  _mutex;
};
 
class MainWidget:public QMainWindow
{
    Q_OBJECT
 public:
  MainWidget();
 protected:
 
  void dragEnterEvent(QDragEnterEvent *event);
 
  void dropEvent(QDropEvent *event);
 protected slots:
  void openFile();

 private:
  QScopedPointer<ViewerFrameThread> m_viewThread;
  osg::ref_ptr<UpdateOperation> m_updateOperation;
};
#endif // _OSGWIDGET_H_
