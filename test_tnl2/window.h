#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include "tnl2_test_widget.h"

class window : public QWidget
{
Q_OBJECT
public:
	tnl2_test_widget *left_pane;
	tnl2_test_widget *right_pane;

    explicit window(QWidget *parent = 0);

signals:

public slots:
	void tick();
	void restart_server_client();
	void restart_client_client();
};

#endif // WINDOW_H
