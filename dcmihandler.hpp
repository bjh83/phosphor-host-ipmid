#ifndef __HOST_IPMI_DCMI_HANDLER_H__
#define __HOST_IPMI_DCMI_HANDLER_H__

#include <map>
#include <string>
#include <vector>
#include <sdbusplus/bus.hpp>

namespace dcmi
{

enum Commands
{
    // Get capability bits
    GET_POWER_LIMIT = 0x03,
    SET_POWER_LIMIT = 0x04,
    APPLY_POWER_LIMIT = 0x05,
    GET_ASSET_TAG = 0x06,
    SET_ASSET_TAG = 0x08,
    GET_MGMNT_CTRL_ID_STR = 0x09,
    SET_MGMNT_CTRL_ID_STR = 0x0A,
};

static constexpr auto propIntf = "org.freedesktop.DBus.Properties";
static constexpr auto assetTagIntf =
        "xyz.openbmc_project.Inventory.Decorator.AssetTag";
static constexpr auto assetTagProp = "AssetTag";
static constexpr auto networkServiceName = "xyz.openbmc_project.Network";
static constexpr auto networkConfigObj =
        "/xyz/openbmc_project/network/config";
static constexpr auto networkConfigIntf =
        "xyz.openbmc_project.Network.SystemConfiguration";
static constexpr auto hostNameProp = "HostName";

namespace assettag
{

    using ObjectPath = std::string;
    using Service = std::string;
    using Interfaces = std::vector<std::string>;
    using ObjectTree = std::map<ObjectPath, std::map<Service, Interfaces>>;

} //namespace assettag

static constexpr auto groupExtId = 0xDC;

static constexpr auto assetTagMaxOffset = 62;
static constexpr auto assetTagMaxSize = 63;
static constexpr auto maxBytes = 16;
static constexpr size_t maxCtrlIdStrLen = 63;

/** @struct GetAssetTagRequest
 *
 *  DCMI payload for Get Asset Tag command request.
 */
struct GetAssetTagRequest
{
    uint8_t groupID;            //!< Group extension identification.
    uint8_t offset;             //!< Offset to read.
    uint8_t bytes;              //!< Number of bytes to read.
} __attribute__((packed));

/** @struct GetAssetTagResponse
 *
 *  DCMI payload for Get Asset Tag command response.
 */
struct GetAssetTagResponse
{
    uint8_t groupID;            //!< Group extension identification.
    uint8_t tagLength;          //!< Total asset tag length.
} __attribute__((packed));

/** @struct SetAssetTagRequest
 *
 *  DCMI payload for Set Asset Tag command request.
 */
struct SetAssetTagRequest
{
    uint8_t groupID;            //!< Group extension identification.
    uint8_t offset;             //!< Offset to write.
    uint8_t bytes;              //!< Number of bytes to write.
} __attribute__((packed));

/** @struct SetAssetTagResponse
 *
 *  DCMI payload for Set Asset Tag command response.
 */
struct SetAssetTagResponse
{
    uint8_t groupID;            //!< Group extension identification.
    uint8_t tagLength;          //!< Total asset tag length.
} __attribute__((packed));

/** @brief Read the object tree to fetch the object path that implemented the
 *         Asset tag interface.
 *
 *  @param[in,out] objectTree - object tree
 *
 *  @return On success return the object tree with the object path that
 *          implemented the AssetTag interface.
 */
void readAssetTagObjectTree(dcmi::assettag::ObjectTree& objectTree);

/** @brief Read the asset tag of the server
 *
 *  @return On success return the asset tag.
 */
std::string readAssetTag();

/** @brief Write the asset tag to the asset tag DBUS property
 *
 *  @param[in] assetTag - Asset Tag to be written to the property.
 */
void writeAssetTag(const std::string& assetTag);

/** @brief Read the current power cap value
 *
 *  @param[in] bus - dbus connection
 *
 *  @return On success return the power cap value.
 */
uint32_t getPcap(sdbusplus::bus::bus& bus);

/** @brief Check if the power capping is enabled
 *
 *  @param[in] bus - dbus connection
 *
 *  @return true if the powerCap is enabled and false if the powercap
 *          is disabled.
 */
bool getPcapEnabled(sdbusplus::bus::bus& bus);

/** @struct GetPowerLimitRequest
 *
 *  DCMI payload for Get Power Limit command request.
 */
struct GetPowerLimitRequest
{
    uint8_t groupID;            //!< Group extension identification.
    uint16_t reserved;          //!< Reserved
} __attribute__((packed));

/** @struct GetPowerLimitResponse
 *
 *  DCMI payload for Get Power Limit command response.
 */
struct GetPowerLimitResponse
{
    uint8_t groupID;            //!< Group extension identification.
    uint16_t reserved;          //!< Reserved.
    uint8_t exceptionAction;    //!< Exception action.
    uint16_t powerLimit;        //!< Power limit requested in watts.
    uint32_t correctionTime;    //!< Correction time limit in milliseconds.
    uint16_t reserved1;         //!< Reserved.
    uint16_t samplingPeriod;    //!< Statistics sampling period in seconds.
} __attribute__((packed));

/** @brief Set the power cap value
 *
 *  @param[in] bus - dbus connection
 *  @param[in] powerCap - power cap value
 */
void setPcap(sdbusplus::bus::bus& bus, const uint32_t powerCap);

/** @struct SetPowerLimitRequest
 *
 *  DCMI payload for Set Power Limit command request.
 */
struct SetPowerLimitRequest
{
    uint8_t groupID;            //!< Group extension identification.
    uint16_t reserved;          //!< Reserved
    uint8_t reserved1;          //!< Reserved
    uint8_t exceptionAction;    //!< Exception action.
    uint16_t powerLimit;        //!< Power limit requested in watts.
    uint32_t correctionTime;    //!< Correction time limit in milliseconds.
    uint16_t reserved2;         //!< Reserved.
    uint16_t samplingPeriod;    //!< Statistics sampling period in seconds.
} __attribute__((packed));

/** @struct SetPowerLimitResponse
 *
 *  DCMI payload for Set Power Limit command response.
 */
struct SetPowerLimitResponse
{
    uint8_t groupID;            //!< Group extension identification.
} __attribute__((packed));

/** @brief Enable or disable the power capping
 *
 *  @param[in] bus - dbus connection
 *  @param[in] enabled - enable/disable
 */
void setPcapEnable(sdbusplus::bus::bus& bus, bool enabled);

/** @struct ApplyPowerLimitRequest
 *
 *  DCMI payload for Activate/Deactivate Power Limit command request.
 */
struct ApplyPowerLimitRequest
{
    uint8_t groupID;            //!< Group extension identification.
    uint8_t powerLimitAction;   //!< Power limit activation
    uint16_t reserved;          //!< Reserved
} __attribute__((packed));

/** @struct ApplyPowerLimitResponse
 *
 *  DCMI payload for Acticate/Deactivate Power Limit command response.
 */
struct ApplyPowerLimitResponse
{
    uint8_t groupID;            //!< Group extension identification.
} __attribute__((packed));

/** @struct GetMgmntCtrlIdStrRequest
 *
 *  DCMI payload for Get Management Controller Identifier String cmd request.
 */
struct GetMgmntCtrlIdStrRequest
{
    uint8_t groupID;            //!< Group extension identification.
    uint8_t offset;             //!< Offset to read.
    uint8_t bytes;              //!< Number of bytes to read.
} __attribute__((packed));

/** @struct GetMgmntCtrlIdStrResponse
 *
 *  DCMI payload for Get Management Controller Identifier String cmd response.
 */
struct GetMgmntCtrlIdStrResponse
{
    uint8_t groupID;            //!< Group extension identification.
    uint8_t strLen;             //!< ID string length.
    char data[];                //!< ID string
} __attribute__((packed));

/** @struct SetMgmntCtrlIdStrRequest
 *
 *  DCMI payload for Set Management Controller Identifier String cmd request.
 */
struct SetMgmntCtrlIdStrRequest
{
    uint8_t groupID;            //!< Group extension identification.
    uint8_t offset;             //!< Offset to write.
    uint8_t bytes;              //!< Number of bytes to read.
    char data[];                //!< ID string
} __attribute__((packed));

/** @struct GetMgmntCtrlIdStrResponse
 *
 *  DCMI payload for Get Management Controller Identifier String cmd response.
 */
struct SetMgmntCtrlIdStrResponse
{
    uint8_t groupID;            //!< Group extension identification.
    uint8_t offset;             //!< Last Offset Written.
} __attribute__((packed));

} // namespace dcmi

#endif
