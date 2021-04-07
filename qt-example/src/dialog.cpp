#include "dialog.h"

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
{
    lineEdit = new QLineEdit;
    lineEdit->setText("hello");
    runButton = new QPushButton("Run");
    quitButton = new QPushButton("Quit");
    wiggly = new WigglyWidget;

    connect(quitButton, &QAbstractButton::clicked, qApp, &QCoreApplication::quit);
    connect(runButton,  &QAbstractButton::clicked, this, [&]{
        runButton->setText(QString("Run (%1)").arg(++runClicked));
    });

    hotReload();
}

void Dialog::hotReload()
{
    setWindowTitle("Hot C++ Reload");

    QFont ft;
    ft.setBold(false);
    ft.setItalic(false);
    ft.setPointSize(30);
    wiggly->setFont(ft);
    wiggly->setText("Reload me!");

    delete layout();

    QVBoxLayout *vbox = new QVBoxLayout;
    QHBoxLayout *hbox1 = new QHBoxLayout;
    QHBoxLayout *hbox2 = new QHBoxLayout;

    hbox1->addWidget(lineEdit);
    hbox1->addWidget(runButton);

    hbox2->addStretch();
    hbox2->addWidget(quitButton);
    vbox->addLayout(hbox1);
    vbox->addWidget(wiggly);
    vbox->addLayout(hbox2);

    setLayout(vbox);
    updateGeometry();
}
