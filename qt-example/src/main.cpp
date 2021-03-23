
#include <QtWidgets>

#include "dialog.h"
#include "qthotreload.h"

int main(int argc, char **argv)
{
    QtHotReload hotReload([&]() -> int {
        QApplication app(argc, argv);

        Dialog dialog;
        dialog.show();

        QObject::connect(&hotReload, &QtHotReload::stateChanged, &dialog, [&](QtHotReload::State state) {
            switch (state) {
            case QtHotReload::Initializing:
                dialog.setEnabled(false);
                break;
            case QtHotReload::Ready:
                dialog.setEnabled(true);
                break;
            case QtHotReload::Dirty:
                dialog.setWindowTitle(dialog.windowTitle() + " [dirty]");
                break;
            case QtHotReload::Loaded:
                dialog.hotReload();
                break;
            }
        });
        return app.exec();
    });
    return hotReload.exec();
}
