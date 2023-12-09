#pragma once
#include "Instance/Instance.h"

class MTInstance : public Instance {
public:
    MTInstance(bool debug);
    std::vector<std::shared_ptr<Adapter>> EnumerateAdapters() override;
};
