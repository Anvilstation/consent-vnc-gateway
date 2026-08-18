#pragma once

#include "application/Interfaces.h"

namespace gateway {

class QtConsentPrompt final : public ConsentPrompt
{
public:
    explicit QtConsentPrompt(int timeoutSeconds);

    bool ask(const ConnectionRequest &request) override;
    void showCredential(const QString &credential) override;

private:
    int m_timeoutSeconds;
};

} // namespace gateway
