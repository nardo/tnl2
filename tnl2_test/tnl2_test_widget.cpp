#include <QtGui>
#include <QtOpenGL>
#include "tnl2_test_widget.h"

extern void init_game();
extern void tick_game();
extern void render_game_scene();
extern void click_game(float x, float y);

tnl2_test_widget::tnl2_test_widget(QWidget *parent) :
    QGLWidget(parent)
{
    qtGreen = QColor::fromCmykF(0.40, 0.0, 1.0, 0.0);
    qtPurple = QColor::fromCmykF(0.39, 0.39, 0.0, 0.0);
	init_game();
	QTimer *timer = new QTimer(this);
		connect(timer, SIGNAL(timeout()), this, SLOT(tick()));
		timer->start(0);
}

void tnl2_test_widget::tick()
{
	tick_game();
	update();
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
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_MULTISAMPLE);
    static GLfloat lightPosition[4] = { 0.5, 5.0, 7.0, 1.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
}
//! [6]


//! [7]
void tnl2_test_widget::paintGL()
{
    render_game_scene();
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
	click_game(event->pos().x() / float(s.width()), event->pos().y() / float(s.height()));
}
//! [9]

//! [10]
void tnl2_test_widget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - lastPos.x();
    int dy = event->y() - lastPos.y();
}
//! [10]
