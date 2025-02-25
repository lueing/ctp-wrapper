//
// Created by crazy on 2025/2/19.
//
#include "gtest/gtest.h"
#include "config.h"

TEST(ConfigTest, CreateCtpConfig) {
    std::string yaml_file = "ctp.yaml";
    auto config = lueing::CreateCtpConfig(yaml_file);
    std::string expect_value = config->m_clientPrincipal.BrokerID;
    EXPECT_EQ(expect_value, "");
    expect_value = config->m_clientPrincipal.UserID;
    EXPECT_EQ(expect_value, "5601");
    expect_value = config->m_clientPrincipal.UserProductInfo;
    EXPECT_EQ(expect_value, "client_luei");
}
