#ifndef TNL2_TEST_WIDGET_H
#define TNL2_TEST_WIDGET_H

#include <QGLWidget>

class tnl2_test_widget : public QGLWidget
{
Q_OBJECT
public:
    explicit tnl2_test_widget(QWidget *parent = 0);
protected:
    QPoint lastPos;
    QColor qtGreen;
    QColor qtPurple;
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

signals:

public slots:
	void tick();

};

#endif // TNL2_TEST_WIDGET_H
