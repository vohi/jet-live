#pragma once

#include <QObject>
#include <memory>

class QtHotReloadPrivate;

class QtHotReload : public QObject
{
    Q_OBJECT
    Q_ENUMS(State)
public:
    enum State {
        Initializing,
        Ready,
        Dirty,
        Loaded
    };
    using EntryPoint = std::function<int(QtHotReload *)>;

    QtHotReload(const EntryPoint &entry, QObject *parent = nullptr);
    ~QtHotReload();

    int exec();

signals:
    void stateChanged(State);
    void reloaded();
    void preLoad();
    void postLoad();

private:
    friend QtHotReloadPrivate;
    std::unique_ptr<QtHotReloadPrivate> d_ptr;
};
