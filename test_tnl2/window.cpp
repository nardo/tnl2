#include "window.h"
#include <QtGui>

extern void restart_games(bool game_1_is_server, bool game_2_is_server);
extern void tick_games();

window::window(QWidget *parent) :
    QWidget(parent)
{
	left_pane = new tnl2_test_widget(0, 0);
	right_pane = new tnl2_test_widget(0, 1);

	QMenuBar *menuBar = new QMenuBar(0);
	QMenu *actionMenu = menuBar->addMenu(tr("Action"));
	QAction *clientAction = new QAction("Restart Server and Client", this);
	QAction *serverAction = new QAction("Restart Client and Client", this);
	connect(clientAction, SIGNAL(triggered()), SLOT(restart_server_client()));
	connect(serverAction, SIGNAL(triggered()), SLOT(restart_client_client()));
	actionMenu->addAction(clientAction);
	actionMenu->addAction(serverAction);

	QHBoxLayout *mainLayout = new QHBoxLayout;
	mainLayout->addWidget(left_pane);
	mainLayout->addWidget(right_pane);
	restart_games(true, false);
	setLayout(mainLayout);
	setWindowTitle(tr("tnl2 - test"));

	QTimer *timer = new QTimer(this);
		connect(timer, SIGNAL(timeout()), this, SLOT(tick()));
		timer->start(0);
}

void window::tick()
{
	tick_games();
	left_pane->update();
	right_pane->update();
}

void window::restart_server_client()
{
	restart_games(true, false);
}

void window::restart_client_client()
{
	restart_games(false, false);
}

