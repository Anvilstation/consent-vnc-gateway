#pragma once

#include "application/Interfaces.h"

namespace gateway {

class LocalHostIdentityProvider final : public HostIdentityProvider
{
public:
    HostIdentity current() const override;
};

} // namespace gateway
