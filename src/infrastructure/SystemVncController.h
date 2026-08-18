#pragma once

#include "application/Interfaces.h"
#include "config/AppConfig.h"

namespace gateway {

class SystemVncController final : public VncController
{
public:
    explicit SystemVncController(VncSettings settings);

    QString rotateCredential(QString *error) override;
    bool disconnectAll(QString *error) override;

private:
    bool execute(const QStringList &arguments, QString *error) const;

    VncSettings m_settings;
};

} // namespace gateway
