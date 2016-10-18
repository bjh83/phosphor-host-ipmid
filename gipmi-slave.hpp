#ifndef GIPMI_SLAVE_HPP_
#define GIPMI_SLAVE_HPP_

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <stdint.h>

#include "gipmi.hpp"

using std::string;

namespace gipmi
{

class GipmiSlave : public ipmid::IpmiHandler
{
    public:
        explicit GipmiSlave(
            ipmid::OemGroupRouter* router,
            std::function<void(sd_bus_message*, uint8_t, const string&)> callback);

        virtual void SendResponse(sd_bus_message* context, uint8_t seq,
                                  const string& response);
        bool HandleRequest(const ipmid::IpmiContext& context,
                           const ipmid::IpmiMessage& message) override;

    private:
        ipmid::OemGroupRouter* router_;
        std::function<void(sd_bus_message*, uint8_t, const string&)> callback_;
        std::map<uint8_t, GipmiDecoder> in_progress_messages_;
};

class GipmiRpcServer
{
    public:
        explicit GipmiRpcServer(ipmid::OemGroupRouter* router);
        virtual string HandleMessage(const string& message) = 0;

    private:
        // TODO: The GipmiRpcServer is logically owned by the GipmiSlave, so we should
        // flip this relationship.
        GipmiSlave* gipmi_slave_;

        virtual void RequestCallback(sd_bus_message* context, uint8_t seq,
                                     const string& message);
};

} // namespace gipmi
#endif // GIPMI_SLAVE_HPP_
