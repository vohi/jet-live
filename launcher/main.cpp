#include <QtCore>
#include <iostream>
#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <csignal>
#endif

#include <QLocalServer>
#include <QLocalSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkInterface>

class ProcessController : public QObject
{
public:
    ProcessController()
#ifdef Q_OS_WIN
    : stdinNotifier(GetStdHandle(STD_INPUT_HANDLE))
#else
    : stdinNotifier(0, QSocketNotifier::Read)
#endif
    {
        arguments = QCoreApplication::arguments();
        arguments.takeFirst(); // argv[0], i.e. self

        int connectData = arguments.indexOf("--connect");
        if (connectData > -1) {
            QString host = arguments.takeAt(connectData);
            host = arguments.takeAt(connectData);
            QString port = arguments.takeAt(connectData);
            tcpClient = new QTcpSocket(this);
            tcpClient->connectToHost(host, port.toInt());
            connect(tcpClient, &QIODevice::readyRead, this, &ProcessController::readTcp);
        }

        if (!tcpClient) {
            if (!standardInput.open(0, QIODevice::ReadOnly|QIODevice::Unbuffered)) {
                std::cerr << "[E]: Can't open stdin for reading" << std::endl;
                abort();
            }
        }

        if (!tcpClient) {
#ifdef Q_OS_WIN
            connect(&stdinNotifier, &QWinEventNotifier::activated, this, &ProcessController::readStdin);
#else
            connect(&stdinNotifier, &QSocketNotifier::activated, this, &ProcessController::readStdin);
            signal(SIGINT, [](int){
                QCoreApplication::quit();
            });
#endif
        }

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
        for (const auto &process : qAsConst(processes)) {
            enum class KillState { INT, TERM, KILL, GIVEUP } killState = KillState::INT;
            while (process->state() == QProcess::Running) {
                switch (killState) {
                case KillState::INT:
                    sendCommand("quit");
#ifndef Q_OS_WIN
                    ::kill(process->processId(), SIGINT);
#endif
                    killState = KillState::TERM;
                    break;
                case KillState::TERM:
                    process->terminate();
                    killState = KillState::KILL;
                    break;
                case KillState::KILL:
                    process->kill();
                    killState = KillState::GIVEUP;
                    break;
                case KillState::GIVEUP:
                    continue;
                }
                process->waitForFinished(5000);
            }
        }
    }

    void sendCommand(const QByteArray &command)
    {
        std::cerr << "Sending " << command.constData() << " to " << localSockets.count() << " local clients" << std::endl;
        for (const auto &client : qAsConst(localSockets)) {
            client->write(command + "\n");
            client->waitForBytesWritten();
        }
        std::cerr << "Sending " << command.constData() << " to " << tcpSockets.count() << " remote clients" << std::endl;
        for (const auto &client : qAsConst(tcpSockets)) {
            client->write(command + "\n");
            client->waitForBytesWritten();
        }
    }

    bool run()
    {
        bool success = true;
        for (const auto &argument : qAsConst(arguments)) {
            QProcess *process = new QProcess(this);
            processes.append(process);
            if (argument.contains(":")) {
                QStringList segments = argument.split(":");
                QString vmname = segments.takeFirst();
                if (!tcpServer.isListening()) {
                    QHostAddress serverAddress;
                    for (const auto &address : QNetworkInterface::allAddresses()) {
                        if (address.isLoopback())
                            continue;
                        if (address == QHostAddress(QHostAddress::LocalHost))
                            continue;
                        if (address.protocol() != QAbstractSocket::IPv4Protocol)
                            continue;
                        serverAddress = address;
                        break;
                    }
                    tcpServer.listen(serverAddress);
                    connect(&tcpServer, &QTcpServer::newConnection, this, [&]{
                        std::cerr << "[I]: New tcp connection" << std::endl;
                        tcpSockets.append(tcpServer.nextPendingConnection());
                    });
                }
                segments.append("--connect");
                segments.append(tcpServer.serverAddress().toString());
                segments.append(QString::number(tcpServer.serverPort()));
                process->setProgram("minicoin");
                process->setObjectName(vmname.toUtf8());
                process->setArguments({"run", "launch", "--exe", "launcher", "--args", segments.join(" "), vmname});
            } else {
                process->setObjectName("Local");
                process->setProgram(argument);
            }

            connect(process, &QProcess::readyReadStandardOutput, this, [process](){
                std::cerr << "(" << process->objectName().toUtf8().constData() << ") ";
                std::cerr << process->readAllStandardOutput().constData();
            });

            connect(process, &QProcess::readyReadStandardError, this, [process](){
                std::cerr << "(" << process->objectName().toUtf8().constData() << ") ";
                std::cerr << process->readAllStandardError().constData();
            });

            connect(process, &QProcess::finished, this, [process, this](){
                const char prevCommand = lastCommand;
                lastCommand = 0;
                bool quit = true;
                if (process->exitStatus() == QProcess::CrashExit) {
                    if (prevCommand == 'r') {
                        std::cerr << "[W]: Process crashed after reload, trying to repair" << std::endl;
                        rebuild();
                        quit = !run();
                    } else {
                        std::cerr << "[W]: Process crashed, giving up: " << process->program().toUtf8().constData() << std::endl;
                    }
                }
                processes.removeAll(process);
                if (processes.isEmpty() && quit)
                    QCoreApplication::quit();
            });

            process->start(QIODevice::ReadOnly);
            if (!process->waitForStarted()) {
                std::cerr << "Failed to start " << process->program().toUtf8().constData() << std::endl;
                success = false;
                break;
            }
        }

        return success;
    }

    void showDialog()
    {
        std::cout << "(r)eload, (R)estart, or (q)uit: " << std::flush;
    }

    void rebuild()
    {
        for (const auto &process : qAsConst(processes)) {
            const QDir exepath(process->program());
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
    }

    void readStdin()
    {
        const QByteArray data = standardInput.readLine();
        processCommand(data.simplified());
    }
    void readTcp()
    {
        const QByteArray data = tcpClient->readLine();
        processCommand(data.simplified());
    }

    void processCommand(const QByteArray &data)
    {
        lastCommand = data.isEmpty() ? 0 : data[0];

        std::cerr << "[I]: Sending ";
        switch (lastCommand) {
        case 0:
            break;
        case 'r':
            std::cerr << "Reloading";
            sendCommand("reload");
            break;
        case 'R':
            std::cerr << "Restarting";
            sendCommand("Restart");
            break;
        case 'q':
            std::cerr << "Quitting";
            QCoreApplication::quit();
            return;
        default:
            std::cerr << "Unknown '" << data.simplified().constData() << "'";
            break;
        }
        std::cerr << " command to clients" << localSockets.count() << "/" << tcpSockets.count() << std::endl;

        showDialog();
    }

private:
    QStringList arguments;
    QTcpServer tcpServer;
    QTcpSocket *tcpClient = nullptr;
    QLocalServer localServer;
    QList<QLocalSocket*> localSockets;
    QList<QTcpSocket*> tcpSockets;
    QList<QProcess*> processes;
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
        return 2;
    }

    controller.showDialog();
    return app.exec();
}
