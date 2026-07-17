#pragma once

#include <QString>
#include <QWidget>

class IPlugin
{
public:
    virtual ~IPlugin() = default;

    virtual bool load() = 0;
    virtual bool unload() = 0;
    virtual QString name() const = 0;
    virtual QString version() const = 0;
    virtual QWidget* createWidget() = 0;
};

#define WukongPlugin_IID "com.wukong.toolkit.plugin"
Q_DECLARE_INTERFACE(IPlugin, WukongPlugin_IID)