#include "gipmi.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "ipmid-test-utils.hpp"

using ipmid::kOemGroupNetFnRequest;
using ipmid::kOemGroupNetFnResponse;
using ipmid::IpmiMessage;


namespace gipmi
{

class GipmiEncoderTest : public ::testing::Test
{
};

TEST_F(GipmiEncoderTest, SimpleRequestMessage)
{
    const string message = "message";
    IpmiMessage expected_message;
    expected_message.netfn = kOemGroupNetFnRequest;
    expected_message.cmd = kGipmiCommand;
    expected_message.payload.resize(GipmiPayloadSizeToIpmiPayloadSize(
                                        message.size()));
    GipmiMessage* gipmi_expected_message = reinterpret_cast<GipmiMessage*>
                                           (expected_message.payload.data());
    memcpy(gipmi_expected_message->magic, kGipmiMagic.data(), kGipmiMagic.size());
    gipmi_expected_message->index = 0;
    gipmi_expected_message->count = 1;
    memcpy(gipmi_expected_message->payload, message.data(), message.size());

    GipmiEncoder encoder(message, GipmiEncoder::REQUEST);

    EXPECT_TRUE(encoder.HasNext());
    EXPECT_EQ(expected_message, encoder.Next());
    EXPECT_FALSE(encoder.HasNext());
}

TEST_F(GipmiEncoderTest, SimpleResponseMessage)
{
    const string message = "message";
    GipmiEncoder request_encoder(message, GipmiEncoder::REQUEST);

    IpmiMessage expected_message = request_encoder.Next();
    expected_message.netfn = kOemGroupNetFnResponse;

    GipmiEncoder response_encoder(message, GipmiEncoder::RESPONSE);
    EXPECT_EQ(expected_message, response_encoder.Next());
};

TEST_F(GipmiEncoderTest, EmptyMessage)
{
    const string message = "";
    IpmiMessage expected_message;
    expected_message.netfn = kOemGroupNetFnRequest;
    expected_message.cmd = kGipmiCommand;
    expected_message.payload.resize(GipmiPayloadSizeToIpmiPayloadSize(
                                        message.size()));
    GipmiMessage* gipmi_expected_message = reinterpret_cast<GipmiMessage*>
                                           (expected_message.payload.data());
    memcpy(gipmi_expected_message->magic, kGipmiMagic.data(), kGipmiMagic.size());
    gipmi_expected_message->index = 0;
    gipmi_expected_message->count = 1;
    memcpy(gipmi_expected_message->payload, message.data(), message.size());

    GipmiEncoder encoder(message, GipmiEncoder::REQUEST);

    EXPECT_TRUE(encoder.HasNext());
    EXPECT_EQ(expected_message, encoder.Next());
    EXPECT_FALSE(encoder.HasNext());
}

} // namespace gipmi
