
#include <QtWidgets>

#include "dialog.h"
#include "qthotreload.h"

int main(int argc, char **argv)
{
    QtHotReload hotReload([&]{
        QApplication app(argc, argv);

        Dialog dialog;
        dialog.show();

        return app.exec();
    });
    return hotReload.exec();
}
