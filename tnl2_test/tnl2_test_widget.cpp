#include <QtGui>
#include <QtOpenGL>
#include "tnl2_test_widget.h"

extern void render_game_scene(int game_index);
extern void click_game(int game_index, float x, float y);

tnl2_test_widget::tnl2_test_widget(QWidget *parent, int pane) :
		QGLWidget(QGLFormat(QGL::Rgba | QGL::DoubleBuffer | QGL::DirectRendering | QGL::SampleBuffers ), parent)
{
	_pane = pane;
}

//! [6]
void tnl2_test_widget::initializeGL()
{
    qglClearColor(qtPurple.dark());

    //logo = new QtLogo(this, 64);
    //logo->setColor(qtGreen.dark());

	//glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
	//glEnable(GL_LIGHTING);
	//glEnable(GL_LIGHT0);
	//glEnable(GL_MULTISAMPLE);
	//static GLfloat lightPosition[4] = { 0.5, 5.0, 7.0, 1.0 };
	//glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
}
//! [6]


//! [7]
void tnl2_test_widget::paintGL()
{
	render_game_scene(_pane);
}
//! [7]

//! [8]
void tnl2_test_widget::resizeGL(int width, int height)
{
    int side = qMin(width, height);
    //glViewport((width - side) / 2, (height - side) / 2, side, side);
    glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}
//! [8]

//! [9]
void tnl2_test_widget::mousePressEvent(QMouseEvent *event)
{
	QSize s = size();
	click_game(_pane, event->pos().x() / float(s.width()), event->pos().y() / float(s.height()));
}
//! [9]

//! [10]
void tnl2_test_widget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - lastPos.x();
    int dy = event->y() - lastPos.y();
}
//! [10]
