#include "gipmi-slave.hpp"

#include <string.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "ipmid-test-utils.hpp"

using std::to_string;
using std::unique_ptr;

using ipmid::IpmiContext;
using ipmid::IpmiHandler;
using ipmid::IpmiMessage;
using ipmid::OemGroupRouter;
using ipmid::OemGroup;
using testing::Return;

namespace gipmi
{

class MockOemGroupRouter : public OemGroupRouter
{
    public:
        MockOemGroupRouter() : OemGroupRouter(nullptr) {}

        void RegisterHandler(OemGroup, unique_ptr<IpmiHandler>&& handler) override
        {
            handler_ = std::move(handler);
        }

        MOCK_METHOD2(SendResponse, void(const IpmiContext&, const IpmiMessage&));

        unique_ptr<IpmiHandler> handler_;
};

class MockRpcServer : public GipmiRpcServer
{
    public:
        MockRpcServer(OemGroupRouter* router) : GipmiRpcServer(router) {}

        MOCK_METHOD1(HandleMessage, string(const string& message));
};

class GipmiSlaveTest : public ::testing::Test
{
    protected:
        GipmiSlaveTest() :
            rpc_server_(&oem_group_router_),
            slave_(static_cast<GipmiSlave*>(oem_group_router_.handler_.get())) {}

        MockOemGroupRouter oem_group_router_;
        MockRpcServer rpc_server_;
        GipmiSlave* slave_;
};

TEST_F(GipmiSlaveTest, SimpleRequest)
{
    const string input = "input";
    GipmiEncoder request_encoder(input, GipmiEncoder::REQUEST);
    const IpmiContext context = IpmiContext();

    const string output = "output";
    GipmiEncoder response_encoder(output, GipmiEncoder::RESPONSE);

    EXPECT_CALL(rpc_server_, HandleMessage(input))
    .WillOnce(Return(output));

    EXPECT_CALL(oem_group_router_, SendResponse(context, response_encoder.Next()));

    slave_->HandleRequest(context, request_encoder.Next());
}

TEST_F(GipmiSlaveTest, EmptyRequest)
{
    GipmiEncoder request_encoder("", GipmiEncoder::REQUEST);
    const IpmiContext context = IpmiContext();

    EXPECT_CALL(rpc_server_, HandleMessage(""))
    .WillOnce(Return(""));

    GipmiEncoder response_encoder("", GipmiEncoder::RESPONSE);

    EXPECT_CALL(oem_group_router_, SendResponse(context, response_encoder.Next()));

    slave_->HandleRequest(context, request_encoder.Next());
}

} // namespace gipmi
