#include "qthotreload.h"

#include <QDebug>
#include <QCoreApplication>
#include <QThread>

#include <jet/live/Live.hpp>
#include <jet/live/Utility.hpp>
#include <jet/live/ILiveListener.hpp>

#include <csignal>

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
        static QtHotReloadPrivate *qt_reload_instance;
        qt_reload_instance = this;
        const auto sigusr2 = signal(SIGUSR2, [](int){
            qt_reload_instance->_reloading = true;
            qt_reload_instance->_restarted = true;
            raise(SIGUSR1); // triggers a reload
            QCoreApplication::quit();
        });
        const auto sigint = signal(SIGINT, [](int){
            QCoreApplication::quit();
        });

        int res;
        while (_restarted) {
            _restarted = false;
            res = _entry();
            while (_reloading) {
                QThread::msleep(50);
            }
        }
        signal(SIGINT, sigint);
        signal(SIGUSR2, sigusr2);
        qt_reload_instance = nullptr;
        return res;
    }

    QtHotReload *q_ptr;
    std::function<int()> _entry;

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
        while (!isInterruptionRequested()) {
            _live.update();
            QThread::msleep(100);
        }
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
    }
};

void QtHotReloadPrivate::onLog(jet::LogSeverity severity, const std::string& message)
{
    QByteArray severityString;
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
    qDebug() << severityString << ": " << QString::fromStdString(message);
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
