#include "gipmi-slave.hpp"

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <stdint.h>
#include <string.h>

#include "gipmi.hpp"

using std::string;
using std::unique_ptr;
using ipmid::IpmiContext;
using ipmid::IpmiMessage;
using ipmid::kOemGroupMagicSize;
using ipmid::kOemGroupNetFnRequest;
using ipmid::OemGroup;

namespace gipmi
{

GipmiSlave::GipmiSlave(ipmid::OemGroupRouter* router,
                       std::function<void(sd_bus_message*, uint8_t, const string&)> callback)
    : router_(router), callback_(callback) {}

bool GipmiSlave::HandleRequest(const IpmiContext& context,
                               const IpmiMessage& message)
{
    GipmiDecoder* decoder = &in_progress_messages_[message.seq];
    decoder->PutNext(message);
    if (decoder->IsDone())
    {
        callback_(context.context, message.seq, decoder->Consume());
    }
    return true;
}

void GipmiSlave::SendResponse(sd_bus_message* context, uint8_t seq,
                              const string& response)
{
    GipmiEncoder encoder(response, GipmiEncoder::RESPONSE);
    while (encoder.HasNext())
    {
        IpmiContext ipmi_context;
        ipmi_context.context = context;
        IpmiMessage message = encoder.Next();
        message.seq = seq;
        router_->SendResponse(ipmi_context, message);
    }
}

GipmiRpcServer::GipmiRpcServer(ipmid::OemGroupRouter* router)
{
    gipmi_slave_ = new GipmiSlave(
        router,
        [this](sd_bus_message * context, uint8_t seq, const string & message)
    {
        return this->RequestCallback(context, seq, message);
    });
    router->RegisterHandler(kGipmiMagic, unique_ptr<GipmiSlave>(gipmi_slave_));
}

void GipmiRpcServer::RequestCallback(sd_bus_message* context, uint8_t seq,
                                     const string& message)
{
    gipmi_slave_->SendResponse(context, seq, HandleMessage(message));
}

} // namespace gipmi
