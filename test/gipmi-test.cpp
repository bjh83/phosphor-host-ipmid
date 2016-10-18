#include "gipmi.hpp"
#include "gipmi-master.hpp"
#include "gipmi-slave.hpp"
#include <gtest/gtest.h>

using std::unique_ptr;
using ipmid::IpmiContext;
using ipmid::IpmiMessage;
using ipmid::IpmiMessageBus;
using ipmid::OemGroupRouter;
using ipmid::RootRouter;

namespace gipmi
{

BlockTransferMessage ToBlockTransfer(const IpmiMessage& ipmi_message)
{
    BlockTransferMessage bt_message;
    bt_message.len = ipmi_message.payload.size() + kBlockTransferHeaderSize - 1;
    bt_message.netfn_lun = (ipmi_message.netfn << 2) | (ipmi_message.lun & 3);
    bt_message.seq = ipmi_message.seq;
    bt_message.cmd = ipmi_message.cmd;
    memcpy(bt_message.payload, ipmi_message.payload.data(),
           ipmi_message.payload.size());
    return bt_message;
}

IpmiMessage ToIpmiMessage(const BlockTransferMessage& bt_message)
{
    IpmiMessage ipmi_message;
    ipmi_message.netfn = bt_message.netfn_lun >> 2;
    ipmi_message.lun = bt_message.netfn_lun & 3;
    ipmi_message.seq = bt_message.seq;
    ipmi_message.cmd = bt_message.cmd;
    ipmi_message.payload.resize(bt_message.len + 1 - kBlockTransferHeaderSize);
    memcpy(ipmi_message.payload.data(), bt_message.payload,
           ipmi_message.payload.size());
    return ipmi_message;
}

class BlockTransferTestMasterSlave : public BlockTransferMaster,
    public IpmiMessageBus
{
    public:
        int SendInternal(const BlockTransferMessage& message) override
        {
            IpmiContext context;
            if (root_router_->HandleRequest(context, ToIpmiMessage(message)))
            {
                return 0;
            }
            else
            {
                return -1;
            }
        }

        void SendMessage(const IpmiContext& context,
                         const IpmiMessage& message) override
        {
            ReceiveInternal(ToBlockTransfer(message));
        }

        void set_root_router(RootRouter* root_router)
        {
            root_router_ = root_router;
        }

    private:
        RootRouter* root_router_;
};

class GipmiRpcEchoServer : public GipmiRpcServer
{
    public:
        explicit GipmiRpcEchoServer(OemGroupRouter* router) : GipmiRpcServer(router) {}
        string HandleMessage(const string& message) override
        {
            return message;
        }
};

class GipmiTest : public ::testing::Test
{
    protected:
        GipmiTest()
            : block_transfer_master_slave_(new BlockTransferTestMasterSlave()),
              block_transfer_master_(block_transfer_master_slave_),
              root_router_(unique_ptr<IpmiMessageBus>(block_transfer_master_slave_)),
              server_(root_router_.mutable_oem_group_router()),
              client_(block_transfer_master_)
        {
            block_transfer_master_slave_->set_root_router(&root_router_);
        }

        BlockTransferTestMasterSlave* block_transfer_master_slave_;
        BlockTransferMaster* block_transfer_master_;
        RootRouter root_router_;
        GipmiRpcEchoServer server_;
        GipmiRpcClient client_;
};

TEST_F(GipmiTest, SimpleRequest)
{
    const string request = "request";
    const RpcResult result = client_.SendMessage(request);
    EXPECT_GE(0, result.status);
    EXPECT_EQ(request, result.result);
}

TEST_F(GipmiTest, EmptyRequest)
{
    const RpcResult result = client_.SendMessage("");
    EXPECT_GE(0, result.status);
    EXPECT_EQ("", result.result);
}

TEST_F(GipmiTest, LongRequest)
{
    const string request(1024, 't');
    const RpcResult result = client_.SendMessage(request);
    EXPECT_GE(0, result.status);
    EXPECT_EQ(request, result.result);
}

} // namespace gipmi
