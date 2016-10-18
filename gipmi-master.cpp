#include <functional>
#include <future>
#include <map>
#include <memory>
#include <string>
#include <stdint.h>
#include <glog/logging.h>

#include "gipmi.hpp"
#include "gipmi-master.hpp"

using std::string;

namespace gipmi
{

int BlockTransferMaster::SendMessage(SenderId sender_id,
                                     BlockTransferMessage* message)
{
    message->seq = sender_id;
    return SendInternal(*message);
}

SenderId BlockTransferMaster::RegisterCallback(
    std::function<void(const BlockTransferMessage&)> callback)
{
    SenderId new_id = next_id_++;
    handler_map_[new_id] = callback;
    return new_id;
}

void BlockTransferMaster::ReceiveInternal(const BlockTransferMessage& message)
{
    auto iter = handler_map_.find(message.seq);
    if (iter == handler_map_.end())
    {
        // TODO: Should this be a warning or an error.
        LOG(ERROR) << "No handler for message with seq: " << message.seq;
        // Nothing to do with message, so just drop it on the floor.
        return;
    }
    iter->second(message);
}

GipmiMaster::GipmiMaster(BlockTransferMaster* block_transfer_master) :
    block_transfer_master_(block_transfer_master)
{
    sender_id_ = block_transfer_master_->RegisterCallback(
                     [this](const BlockTransferMessage & bt_message)
    {
        this->HandleBlockTransferResponse(bt_message);
    });
}

int GipmiMaster::SendMessage(const string& message)
{
    GipmiBlockTransferEncoder encoder(message, true);
    while (encoder.HasNext())
    {
        BlockTransferMessage bt_message = encoder.Next();
        int status = block_transfer_master_->SendMessage(sender_id_, &bt_message);
        if (status < 0)
        {
            return status;
        }
    }
    return 0;
}

void GipmiMaster::RegisterCallback(std::function<void(const string&)> callback)
{
    callback_ = callback;
}

void GipmiMaster::HandleBlockTransferResponse(const BlockTransferMessage&
        bt_message)
{
    decoder_.PutNext(bt_message);
    if (decoder_.IsDone())
    {
        callback_(decoder_.Consume());
    }
}

GipmiRpcClient::GipmiRpcClient(std::unique_ptr<GipmiMaster>&& gipmi_master) :
    gipmi_master_(std::move(gipmi_master)) {}

GipmiRpcClient::GipmiRpcClient(BlockTransferMaster* block_transfer_master)
    : GipmiRpcClient(std::unique_ptr<GipmiMaster>(new GipmiMaster(
                         block_transfer_master))) {}

RpcResult GipmiRpcClient::SendMessage(const string& message)
{
    std::promise<string> result_promise;
    std::future<string> result_future = result_promise.get_future();
    gipmi_master_->RegisterCallback([&result_promise](const string & result)
    {
        result_promise.set_value(result);
    });

    RpcResult rpc_result;
    rpc_result.status = gipmi_master_->SendMessage(message);
    if (rpc_result.status < 0)
    {
        return rpc_result;
    }
    result_future.wait();
    rpc_result.result = result_future.get();
    return rpc_result;
}

////////////////////////////////////////////////////////////////////////////////
// For testing
////////////////////////////////////////////////////////////////////////////////

#include <thread>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

class BlockTransferMasterImpl : public BlockTransferMaster
{
    public:
        BlockTransferMasterImpl()
        {
            fd_ = open("/dev/bt-master", O_RDWR);
            if (fd_ < 0)
            {
                LOG(ERROR) << "Could not open /dev/bt-master!";
                return;
            }
            receive_thread_ = std::thread([this]()
            {
                this->ReceiveLoop();
            });
        }

        ~BlockTransferMasterImpl()
        {
            close(fd_);
        }

        int SendInternal(const BlockTransferMessage& message) override
        {
            printf("master: writing message to OS...\n");
            int ret = write(fd_, &message, sizeof(BlockTransferMessage));
            printf("master: writing message complete.\n");
            return ret;
        }

    private:
        int fd_;
        std::thread receive_thread_;

        void ReceiveLoop()
        {
            BlockTransferMessage message;
            while (true)
            {
                int ret = read(fd_, &message, sizeof(BlockTransferMessage));
                if (ret > 0)
                {
                    ReceiveInternal(message);
                }
                else if (ret != -EAGAIN)
                {
                    perror("Could not read from /dev/bt-master!\n");
                }
            }
        }
};

BlockTransferMaster* block_transfer_master;
GipmiRpcClient* client;
std::thread* master_thread;

void InitMaster()
{
    printf("master: Initializing master.\n");
    block_transfer_master = new BlockTransferMasterImpl();
    client = new GipmiRpcClient(block_transfer_master);
    master_thread = new std::thread([]()
    {
        printf("master: master thread starting...\n");
        RpcResult result = client->SendMessage("Hello, world!");
        if (result.status < 0)
        {
            LOG(ERROR) << "master: received error status: %d\n" << result.status;
        }
        else
        {
            printf("master: received response: %s\n", result.result.c_str());
        }
        printf("master: master thread finished.\n");
    });
}

} // namespace gipmi
