#ifndef OPENBMC_IPMI_FRU_READ_H
#define OPENBMC_IPMI_FRU_READ_H

#include <systemd/sd-bus.h>
#include <array>
#include <string>
#include <map>
#include <vector>

struct IPMIFruData
{
    std::string section;
    std::string property;
    std::string delimiter;
};

using DbusProperty = std::string;
using DbusPropertyVec = std::vector<std::pair<DbusProperty, IPMIFruData>>;

using DbusInterface = std::string;
using DbusInterfaceVec = std::vector<std::pair<DbusInterface, DbusPropertyVec>>;

using FruInstancePath = std::string;
using FruInstanceVec = std::vector<std::pair<FruInstancePath, DbusInterfaceVec>>;

using FruId = uint32_t;
using FruMap = std::map<FruId, FruInstanceVec>;

#endif
