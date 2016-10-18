#include <string>
#include <string.h>
#include <glog/logging.h>

#include "gipmi.hpp"

using std::string;
using ipmid::kIpmiMaxPayloadSize;
using ipmid::kOemGroupMagicSize;
using ipmid::kOemGroupNetFnRequest;
using ipmid::kOemGroupNetFnResponse;
using ipmid::IpmiMessage;

namespace gipmi
{

uint8_t BlockTransferPayloadSizeToPartialLen(size_t payload_size)
{
    return payload_size + kBlockTransferHeaderSize - 1;
}
uint8_t BlockTransferPartialLenToPayloadSize(size_t partial_len)
{
    return partial_len - kBlockTransferHeaderSize + 1;
}

uint8_t GipmiPayloadSizeToBlockTransferLen(size_t gipmi_payload_size)
{
    return BlockTransferPayloadSizeToPartialLen(gipmi_payload_size +
            kGipmiHeaderSize);
}

uint8_t GipmiPayloadSizeFromBlockTransferLen(size_t bt_len)
{
    return BlockTransferPartialLenToPayloadSize(bt_len) - kGipmiHeaderSize;
}

uint8_t GipmiPayloadSizeToIpmiPayloadSize(size_t gipmi_payload_size)
{
    return gipmi_payload_size + kGipmiHeaderSize;
}

uint8_t GipmiPayloadSizeFromIpmiPayloadSize(size_t ipmi_payload_size)
{
    return ipmi_payload_size - kGipmiHeaderSize;
}

GipmiEncoder::GipmiEncoder(string message, EncoderMode mode)
    : message_(message), is_request_(mode == REQUEST)
{
    count_ = message_.size() / kGipmiMaxPayloadSize;
    if (message_.size() % kGipmiMaxPayloadSize || message_.size() == 0)
    {
        count_ += 1;
    }
}

bool GipmiEncoder::HasNext()
{
    return index_ < count_;
}

IpmiMessage GipmiEncoder::Next()
{
    if (index_ >= count_)
    {
        // TODO: Do ERROR handling.
        LOG(ERROR) << "Message has already been fully encoded.";
    }
    IpmiMessage ipmi_message;
    ipmi_message.payload.resize(kIpmiMaxPayloadSize);
    GipmiMessage* gipmi_message = reinterpret_cast<GipmiMessage*>
                                  (ipmi_message.payload.data());
    size_t gipmi_payload_size = message_.copy(
                                    reinterpret_cast<char*>(gipmi_message->payload),
                                    kGipmiMaxPayloadSize,
                                    index_ * kGipmiMaxPayloadSize);
    memcpy(gipmi_message->magic, kGipmiMagic.data(), kOemGroupMagicSize);
    gipmi_message->index = index_++;
    gipmi_message->count = count_;

    ipmi_message.payload.resize(GipmiPayloadSizeToIpmiPayloadSize(
                                    gipmi_payload_size));
    if (is_request_)
    {
        ipmi_message.netfn = kOemGroupNetFnRequest;
    }
    else
    {
        ipmi_message.netfn = kOemGroupNetFnResponse;
    }
    ipmi_message.cmd = kGipmiCommand;
    return ipmi_message;
};

bool GipmiDecoder::IsDone()
{
    return index_ >= count_ && count_ > 0;
}

void GipmiDecoder::PutNext(const IpmiMessage& ipmi_message)
{
    const GipmiMessage* gipmi_message = reinterpret_cast<const GipmiMessage*>
                                        (ipmi_message.payload.data());
    if (!count_)
    {
        count_ = gipmi_message->count;
    }
    if (index_++ != gipmi_message->index)
    {
        // TODO: Do ERROR handling.
        LOG(ERROR) << "Received out of order message. Expected " << index_ - 1
                   << ", but got " << gipmi_message->index;
    }
    message_.append(reinterpret_cast<const char*>(gipmi_message->payload),
                    GipmiPayloadSizeFromIpmiPayloadSize(ipmi_message.payload.size()));
}

string GipmiDecoder::Consume()
{
    count_ = 0;
    string ret;
    ret.swap(message_);
    return ret;
}

GipmiBlockTransferEncoder::GipmiBlockTransferEncoder(string message,
        bool is_request)
    : message_(message), is_request_(is_request)
{
    count_ = message_.size() / kGipmiMaxPayloadSize;
    if (message_.size() % kGipmiMaxPayloadSize || message_.size() == 0)
    {
        count_ += 1;
    }
}

bool GipmiBlockTransferEncoder::HasNext()
{
    return index_ < count_;
}

BlockTransferMessage GipmiBlockTransferEncoder::Next()
{
    if (index_ >= count_)
    {
        // TODO: Do ERROR handling.
        LOG(ERROR) << "Message has already been fully encoded.";
    }
    BlockTransferMessage bt_message;
    GipmiMessage* gipmi_message = reinterpret_cast<GipmiMessage*>
                                  (bt_message.payload);
    size_t gipmi_payload_size = message_.copy(
                                    reinterpret_cast<char*>(gipmi_message->payload),
                                    kGipmiMaxPayloadSize,
                                    index_ * kGipmiMaxPayloadSize);
    memcpy(gipmi_message->magic, kGipmiMagic.data(), kOemGroupMagicSize);
    gipmi_message->index = index_++;
    gipmi_message->count = count_;

    bt_message.len = GipmiPayloadSizeToBlockTransferLen(gipmi_payload_size);
    if (is_request_)
    {
        bt_message.netfn_lun = kOemGroupNetFnRequest << 2;
    }
    else
    {
        bt_message.netfn_lun = kOemGroupNetFnResponse << 2;
    }
    bt_message.cmd = kGipmiCommand;
    return bt_message;
};

bool GipmiBlockTransferDecoder::IsDone()
{
    return index_ >= count_ && count_ > 0;
}

void GipmiBlockTransferDecoder::PutNext(const BlockTransferMessage& bt_message)
{
    const GipmiMessage* gipmi_message = reinterpret_cast<const GipmiMessage*>
                                        (bt_message.payload);
    if (!count_)
    {
        count_ = gipmi_message->count;
    }
    if (index_++ != gipmi_message->index)
    {
        // TODO: Do ERROR handling.
        LOG(ERROR) << "Received out of order message. Expected " << index_ - 1
                   << ", but got " << gipmi_message->index;
    }
    message_.append(reinterpret_cast<const char*>(gipmi_message->payload),
                    GipmiPayloadSizeFromBlockTransferLen(bt_message.len));
}

string GipmiBlockTransferDecoder::Consume()
{
    count_ = 0;
    string ret;
    ret.swap(message_);
    return ret;
}

} // namespace gipmi
