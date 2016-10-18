#ifndef GIPMI_MASTER_HPP_
#define GIPMI_MASTER_HPP_

#include <functional>
#include <future>
#include <map>
#include <memory>
#include <string>
#include <stdint.h>

#include "gipmi.hpp"

using std::string;

namespace gipmi
{

typedef uint8_t SenderId;

class BlockTransferMaster
{
    public:
        virtual int SendMessage(SenderId sender_id, BlockTransferMessage* message);
        virtual SenderId RegisterCallback(
            std::function<void(const BlockTransferMessage&)> callback);

    protected:
        virtual int SendInternal(const BlockTransferMessage& message) = 0;
        virtual void ReceiveInternal(const BlockTransferMessage& message);

    private:
        SenderId next_id_ = 0;
        std::map<SenderId, std::function<void(const BlockTransferMessage&)>>
                handler_map_;
};

class GipmiMaster
{
    public:
        explicit GipmiMaster(BlockTransferMaster* block_transfer_master);

        virtual int SendMessage(const string& message);

        virtual void RegisterCallback(std::function<void(const string&)> callback);

    private:
        SenderId sender_id_;
        BlockTransferMaster* block_transfer_master_;
        GipmiBlockTransferDecoder decoder_;
        std::function<void(const string&)> callback_;

        void HandleBlockTransferResponse(const BlockTransferMessage& bt_message);
};

struct RpcResult
{
    int status;
    string result;
};

class GipmiRpcClient
{
    public:
        explicit GipmiRpcClient(std::unique_ptr<GipmiMaster>&& gipmi_master);
        explicit GipmiRpcClient(BlockTransferMaster* block_transfer_master);

        virtual RpcResult SendMessage(const string& message);

    private:
        std::unique_ptr<GipmiMaster> gipmi_master_;
};

} // namespace gipmi
#endif // GIPMI_MASTER_HPP_
