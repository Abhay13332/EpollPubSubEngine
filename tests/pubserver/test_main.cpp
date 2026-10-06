#include <gtest/gtest.h>
#include<pubsubprocessor.hpp>
namespace PubSubEngine {
TEST(pubsubProcesser, verifySizeConversion) {

    auto decode=PSProtocol::getSize;
    auto encode=PSProtocol::getByteFromInt;
    EXPECT_EQ(decode(encode(345)),345);
    EXPECT_EQ(decode(encode(65535)),65535);
    EXPECT_EQ(decode(encode(-345)),-345);
    EXPECT_EQ(decode(encode(0)),0);

}

TEST(pubSubProcesser, verifyData){
    auto cmdGen=[](const std::string &&data){
        return PSProtocol::getByteFromInt(data.length())+data;
    };
    auto dummySockTest=[](const std::string &&cmd){
        auto verifyInput=PSProtocol::verifyData;
        auto dummySock=socketIO::SocketClient(3,sockaddr_in());
        dummySock.ReadDataBuff=cmd;
        return std::holds_alternative<protoState::DataArrived>(verifyInput(&dummySock));
    };
    EXPECT_TRUE(dummySockTest(cmdGen("Test1")));
    EXPECT_TRUE(dummySockTest(cmdGen("Test2")));
    EXPECT_TRUE(dummySockTest(cmdGen("Test3")));
    EXPECT_FALSE(dummySockTest("te1"));
    EXPECT_FALSE(dummySockTest("test4"));
    EXPECT_FALSE(dummySockTest("\n\n"+cmdGen("Test6")));
}
}
