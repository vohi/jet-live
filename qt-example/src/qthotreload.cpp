#include "qthotreload.h"

#include <QDebug>
#include <QCoreApplication>
#include <QThread>

#include <iostream>

#include <QLocalSocket>

#include <jet/live/Live.hpp>
#include <jet/live/Utility.hpp>
#include <jet/live/ILiveListener.hpp>

class QtHotReloadPrivate : public QThread
{
    Q_OBJECT
public:
    QtHotReloadPrivate(QtHotReload *q, const QtHotReload::EntryPoint &entry)
    : q_ptr(q), _entry(entry)
    {
        connect(this, &QtHotReloadPrivate::stateChanged, this, &QtHotReloadPrivate::setState);
    }

    // called in the thread
    void onLog(jet::LogSeverity severity, const std::string& message);

    int exec()
    {
        int res = -1;
        while (_restarted) {
            _restarted = false;
            // connect things once the event loop is running
            QCoreApplication::postEvent(this, new QEvent(QEvent::User));
            res = _entry(q_ptr);
            while (_reloading) {
                QThread::msleep(50);
            }
        }
        return res;
    }

    QtHotReload *q_ptr;
    std::function<int(QtHotReload *)> _entry;

    QtHotReload::State _state{QtHotReload::Loaded};
    std::atomic_bool _reloading{false};
    std::atomic_bool _restarted{true};

signals:
    void stateChanged(QtHotReload::State);

protected:
    void run() override
    {
        struct Listener : public jet::ILiveListener
        {
            Listener(QtHotReloadPrivate *that) : m_qt(that) {}
            void onLog(jet::LogSeverity severity, const std::string& message) override
            { m_qt->onLog(severity, message); }
            void onCodePreLoad() override
            {
                m_qt->_reloading = true;
                emit m_qt->q_ptr->preLoad();
            }
            void onCodePostLoad() override
            {
                m_qt->_reloading = false;
                emit m_qt->q_ptr->postLoad();
                emit m_qt->stateChanged(QtHotReload::Loaded);
            }

            QtHotReloadPrivate *m_qt;
        };

        jet::Live _live(std::unique_ptr<jet::ILiveListener>(new Listener(this)));

        QLocalSocket socket;
        socket.connectToServer("qt_hot_reload");
        bool connected = socket.waitForConnected(1000);
        if (connected) {
            std::cerr << "[I]: Connected to local server" << std::endl;
            socket.open();
        }
        while (!isInterruptionRequested()) {
            _live.update();
            if (socket.state() == QLocalSocket::ConnectedState) {
                if (socket.waitForReadyRead(100) && socket.canReadLine()) {
                    const QByteArray command = socket.readLine().simplified();
                    std::cerr << "[I]: Command: " << command.constData() << std::endl;
                    if (command == "quit") {
                        QCoreApplication::quit();
                    } else if (command == "restart") {
                        _reloading = true;
                        _restarted = true;
                        _live.tryReload();
                        QCoreApplication::quit();
                    } else if (command == "reload") {
                        _live.tryReload();
                    }
                }
            } else {
                QThread::msleep(100);
            }
        }
    }

    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::User)
            QCoreApplication::instance()->installEventFilter(this);
        return QThread::event(event);
    }

    bool eventFilter(QObject *receiver, QEvent *event) override
    {
        if (receiver->isWindowType() || receiver->isWidgetType()) {
            switch (event->type()) {
            // Show events are sent before we have the event filter installed
            case QEvent::Expose:
            case QEvent::PolishRequest:
                connectObject(receiver);
                break;
            case QEvent::Destroy:
                disconnectObject(receiver);
                break;
            default:
                break;
            }
        }
        return QThread::eventFilter(receiver, event);
    }

private:
    void setState(QtHotReload::State newState)
    {
        if (newState == _state)
            return;
        if (newState == QtHotReload::Loaded)
            _reloading = false;
        _state = newState;
        emit q_ptr->stateChanged(newState);
        if (_state == QtHotReload::Loaded)
            emit q_ptr->reloaded();
    }

    void connectObject(QObject *receiver)
    {
        const QMetaObject *mo = receiver->metaObject();
        if (mo->indexOfSlot("hotReload()") >= 0)
            connect(q_ptr, SIGNAL(reloaded()), receiver, SLOT(hotReload()));
    }
    void disconnectObject(const QObject *receiver)
    {
        q_ptr->disconnect(SIGNAL(reloaded()), receiver);
    }
};

void QtHotReloadPrivate::onLog(jet::LogSeverity severity, const std::string& message)
{
    std::string severityString;
    switch (severity) {
        case jet::LogSeverity::kInfo:
            // go via signal emissions so that setState is called by right event loop
            if (message == "Initializing...") {
                emit stateChanged(QtHotReload::Initializing);
            } else if (message == "Ready") {
                emit stateChanged(QtHotReload::Ready);
            } else if (message == "Nothing to reload.") {
                // clear state immediately, the signal is onl delivered when the event loop runs
                _reloading = false;
                emit stateChanged(QtHotReload::Loaded);
            } else if (message.find("Compiling:") == 0 || message.find("Success:") == 0) {
                emit stateChanged(QtHotReload::Dirty);
            }

            severityString.append("[I]");
            break;
        case jet::LogSeverity::kWarning:
            severityString.append("[W]");
            break;
        case jet::LogSeverity::kError:
            severityString.append("[E]");
            break;
        default:
            return;  // Skipping debug messages, they are too verbose
    }
    std::cerr << severityString << ": " << message << std::endl;
}

QtHotReload::QtHotReload(const EntryPoint &entry, QObject *parent)
  : QObject(parent)
  , d_ptr(new QtHotReloadPrivate(this, entry))
{
}

int QtHotReload::exec()
{
    d_ptr->start(QThread::LowestPriority);
    return d_ptr->exec();
}

QtHotReload::~QtHotReload()
{
    d_ptr->requestInterruption();
    d_ptr->wait();
}

#include "qthotreload.moc"
