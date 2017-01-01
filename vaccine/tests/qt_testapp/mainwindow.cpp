#include "mainwindow.h"
#include "ui_mainwindow.h"

extern struct catch_result { bool is_ready; int ret; } catch_result;

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  if (qApp->arguments().size() > 1 )
    m_exitInSeconds = qApp->arguments().at(1).toInt();
  else
    m_exitInSeconds = 5;

  startTimer(1000);
  ui->setupUi(this);
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::timerEvent(QTimerEvent *event)
{
  if (catch_result.is_ready)
    QApplication::exit(catch_result.ret);
  else if (m_exitInSeconds <= 0 )
    QApplication::exit();
  else
    ui->m_lTimer->setText( QString("Exiting in ") + QString::number(m_exitInSeconds--) + " seconds." );
}
