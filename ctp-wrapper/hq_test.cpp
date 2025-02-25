//
// Created by crazy on 2025/2/19.
//
#include "gtest/gtest.h"
#include "hq.h"

TEST(HQTest, main) {
    auto config = lueing::CreateCtpConfig("ctp.yaml");
    lueing::ProviderCtp ctp(config);

    ctp.SubscribeMarketData("ag2504", "test01");

    for (int i = 0; i < 100; i++)
    {
        ctp.WaitForData("ag2504", "test01");
        auto &data = ctp.MarketData();
        EXPECT_TRUE(!data.empty());
        std::cout << "data.size is :" << data.size() << " price:" << data["ag2504"].front().LastPrice << std::endl;
    }
}
