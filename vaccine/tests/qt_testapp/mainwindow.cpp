#include "mainwindow.h"
#include "ui_mainwindow.h"

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
  if (m_exitInSeconds <= 0 )
    QApplication::exit();
  else
    ui->m_lTimer->setText( QString("Exiting in ") + QString::number(m_exitInSeconds--) + " seconds." );
}
