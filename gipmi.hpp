#ifndef GIPMI_HPP_
#define GIPMI_HPP_

#include <array>
#include <string>
#include <stdio.h>
#include <stdint.h>

#include "ipmid-server.hpp"

using std::string;

class sd_bus_message;

namespace gipmi
{

#define ERROR(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)

static const size_t kBlockTransferHeaderSize = 4;
static const size_t kBlockTransferMaxTotalSize = 256;
static const size_t kBlockTransferMaxPayloadSize = kBlockTransferMaxTotalSize -
        kBlockTransferHeaderSize;

struct BlockTransferMessage
{
    uint8_t len;
    uint8_t netfn_lun;
    uint8_t seq;
    uint8_t cmd;
    uint8_t payload[kBlockTransferMaxPayloadSize];
} __attribute__((packed));

struct BlockTransferContext
{
    sd_bus_message* context;
    BlockTransferMessage message;
};

static const ipmid::OemGroup kGipmiMagic = {'g', 'f', 'a'};

uint8_t BlockTransferPayloadSizeToPartialLen(size_t payload_size);
uint8_t BlockTransferPartialLenToPayloadSize(size_t partial_len);

static const size_t kGipmiHeaderSize = 11;
static const size_t kGipmiMaxPayloadSize = kBlockTransferMaxPayloadSize -
        kGipmiHeaderSize;
static const uint8_t kGipmiCommand = 0x34;

struct GipmiMessage
{
    uint8_t magic[ipmid::kOemGroupMagicSize];
    uint32_t index;
    uint32_t count;
    uint8_t payload[kGipmiMaxPayloadSize];
} __attribute__((packed));

uint8_t GipmiPayloadSizeToIpmiPayloadSize(size_t gipmi_payload_size);
uint8_t GipmiPayloadSizeFromIpmiPayloadSize(size_t ipmi_payload_size);

class GipmiEncoder
{
    public:
        enum EncoderMode
        {
            REQUEST,
            RESPONSE
        };

        GipmiEncoder(string message, EncoderMode mode);
        virtual bool HasNext();
        virtual ipmid::IpmiMessage Next();
    private:
        string message_;
        bool is_request_;
        size_t index_ = 0;
        size_t count_;
};

class GipmiDecoder
{
    public:
        virtual bool IsDone();
        virtual void PutNext(const ipmid::IpmiMessage& message);
        virtual string Consume();
    private:
        string message_;
        size_t index_ = 0;
        size_t count_ = 0;
};

class GipmiBlockTransferEncoder
{
    public:
        GipmiBlockTransferEncoder(string message, bool is_request);
        virtual bool HasNext();
        virtual BlockTransferMessage Next();
    private:
        string message_;
        bool is_request_;
        size_t index_ = 0;
        size_t count_;
};

class GipmiBlockTransferDecoder
{
    public:
        virtual bool IsDone();
        virtual void PutNext(const BlockTransferMessage& bt_message);
        virtual string Consume();
    private:
        string message_;
        size_t index_ = 0;
        size_t count_ = 0;
};

} // namespace gipmi
#endif // GIPMI_HPP_
