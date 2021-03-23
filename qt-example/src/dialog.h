#include <QtWidgets>
#include "wigglywidget.h"

class Dialog : public QDialog
{
    Q_OBJECT
public:
    Dialog(QWidget *parent = nullptr);

public slots:
    void hotReload();

private:
    QLineEdit *lineEdit;
    QPushButton *runButton;
    QPushButton *quitButton;
    WigglyWidget *wiggly;

    int runClicked = 0;
};
