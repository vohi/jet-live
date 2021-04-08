#include <QtCore>
#include <iostream>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include <QLocalServer>
#include <QLocalSocket>

class ProcessController : public QProcess
{
public:
    ProcessController()
#ifdef Q_OS_WIN
    : stdinNotifier(GetStdHandle(STD_INPUT_HANDLE))
#else
    : stdinNotifier(0, QSocketNotifier::Read)
#endif
    {
        QStringList arguments = QCoreApplication::arguments();
        arguments.takeFirst(); // self
        setProgram(arguments.takeFirst());
        setArguments(arguments);

        if (!standardInput.open(0, QIODevice::ReadOnly|QIODevice::Unbuffered)) {
            std::cerr << "[E]: Can't open stdin for reading" << std::endl;
            abort();
        }

        connect(this, &QProcess::readyReadStandardOutput, this, [&](){
            std::cout << readAllStandardOutput().constData();
        });
        connect(this, &QProcess::readyReadStandardError, this, [&](){
            std::cerr << readAllStandardError().constData();
        });
        connect(this, &QProcess::finished, this, [&](){
            const char prevCommand = lastCommand;
            lastCommand = 0;
            bool quit = true;
            if (exitStatus() == QProcess::CrashExit) {
                if (prevCommand == 'r') {
                    std::cerr << "[W]: Process crashed after reload, trying to repair" << std::endl;
                    rebuild();
                    quit = !run();
                } else {
                    std::cerr << "[W]: Process crashed, giving up" << std::endl;
                }
            }
            if (quit)
                QCoreApplication::quit();
        });
#ifdef Q_OS_WIN
        connect(&stdinNotifier, &QWinEventNotifier::activated, this, &ProcessController::processStdin);
#else
        connect(&stdinNotifier, &QSocketNotifier::activated, this, &ProcessController::processStdin);
#endif

        QLocalServer::removeServer("qt_hot_reload");
        if (localServer.listen("qt_hot_reload")) {
            connect(&localServer, &QLocalServer::newConnection, this, [&]{
                localSockets.append(localServer.nextPendingConnection());
                std::cerr << "[I]: Client connected" << std::endl;
            });
        } else {
            std::cerr << "[E]: Can't start local socket server" << std::endl;
            abort();
        }
    }
    ~ProcessController()
    {
        enum class KillState { INT, TERM, KILL, GIVEUP } killState = KillState::INT;
        while (state() == QProcess::Running) {
            switch (killState) {
            case KillState::INT:
                sendSignal("quit");
                killState = KillState::TERM;
                break;
            case KillState::TERM:
                terminate();
                killState = KillState::KILL;
                break;
            case KillState::KILL:
                kill();
                killState = KillState::GIVEUP;
                break;
            case KillState::GIVEUP:
                return;
            }
            waitForFinished(1000);
        }
    }

    void sendSignal(const QByteArray &command)
    {
        for (const auto &client : qAsConst(localSockets)) {
            client->write(command + "\n");
            client->waitForBytesWritten();
        }
    }

    bool run()
    {
        start(QIODevice::ReadOnly);
        return waitForStarted();
    }

    void showDialog()
    {
        std::cout << "(r)eload, (R)estart, or (q)uit: " << std::flush;
    }

    void rebuild()
    {
        const QDir exepath(program());
        const QFileInfo pathInfo(exepath.absolutePath());
        QFile autogenFile(pathInfo.dir().absolutePath() + "/CMakeFiles/" + pathInfo.fileName() + "_autogen.dir/AutogenInfo.json");
        if (autogenFile.exists()) {
            autogenFile.open(QIODevice::ReadOnly);
            const QJsonDocument json = QJsonDocument::fromJson(autogenFile.readAll());
            const QString cmakeBinary = json["CMAKE_EXECUTABLE"].toString();
            const QString buildDirectory = json["CMAKE_BINARY_DIR"].toString();
            QProcess::execute(cmakeBinary, {"--build", buildDirectory});
        }
    }

    void processStdin()
    {
        const QByteArray data = standardInput.readLine();
        lastCommand = data[0];
        switch (lastCommand) {
        case 'r':
            std::cout << "Reloading" << std::endl;
            sendSignal("reload");
            break;
        case 'R':
            std::cout << "Restarting" << std::endl;
            sendSignal("restart");
            break;
        case 'q':
            std::cout << "Quitting" << std::endl;
            QCoreApplication::quit();
            return;
        default:
            std::cerr << "Unknown command: '" << data.simplified().constData() << "'" << std::endl;
            break;
        }

        showDialog();
    }

private:
    QLocalServer localServer;
    QList<QLocalSocket*> localSockets;
    QFile standardInput;
#ifdef Q_OS_WIN
    QWinEventNotifier stdinNotifier;
#else
    QSocketNotifier stdinNotifier;
#endif
    char lastCommand;
};

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <executable> [arguments...]" << std::endl << std::endl;
        return 1;
    }

    QCoreApplication app(argc, argv);

    ProcessController controller;
    if (!controller.run()) {
        std::cerr << "Failed to start " << controller.program().toUtf8().constData() << std::endl;
        return 2;
    }

    controller.showDialog();
    return app.exec();
}
