#include <QtCore>
#include <iostream>
#include <csignal>

#include "signals.hpp"

class ProcessController : public QProcess
{
public:
    ProcessController()
    : stdinNotifier(0, QSocketNotifier::Read)
    {
        QStringList arguments = QCoreApplication::arguments();
        arguments.takeFirst(); // self
        setProgram(arguments.takeFirst());
        setArguments(arguments);

        if (!standardInput.open(0, QIODevice::ReadOnly|QIODevice::Unbuffered)) {
            std::cerr << "Can't open stdin for reading" << std::endl;
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
                    std::cerr << "Process crashed after reload, trying to repair" << std::endl;
                    rebuild();
                    quit = !run();
                } else {
                    std::cerr << "Process crashed, giving up" << std::endl;
                }
            }
            if (quit)
                QCoreApplication::quit();
        });
        connect(&stdinNotifier, &QSocketNotifier::activated, this, &ProcessController::processStdin);
    }
    ~ProcessController()
    {
        enum class KillState { INT, TERM, KILL, GIVEUP } killState = KillState::INT;
        while (state() == QProcess::Running) {
            switch (killState) {
            case KillState::INT:
                sendSignal(SIGINT);
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

    void sendSignal(int signalId)
    {
#ifdef Q_OS_WIN
        qDebug() << "Not implemented";
#else
        ::kill(processId(), signalId);
#endif
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
            qDebug() << "Running " << cmakeBinary << buildDirectory;
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
            sendSignal(JET_LIVE_RELOAD_SIGNAL);
            break;
        case 'R':
            std::cout << "Restarting" << std::endl;
            sendSignal(JET_LIVE_RESTART_SIGNAL);
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
    QFile standardInput;
    QSocketNotifier stdinNotifier;
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
