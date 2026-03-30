#include "config.h"

#include <iostream>
#include <cstring>
#include "lueing_os.h"
#include "yaml-cpp/yaml.h"

lueing::CtpConfigPtr lueing::CreateCtpConfig(const std::string& yaml_file)
{
    if (!lueing::filesystem::FilesExists(yaml_file))
    {
        std::cerr << "Config file not found: " << yaml_file << std::endl;
        exit(1);
    }

    CtpConfigPtr config = std::make_shared<CtpConfig>();
    YAML::Node yaml = YAML::LoadFile(yaml_file);

    // client
    strcpy(config->m_clientPrincipal.BrokerID, yaml["client_access"]["BrokerID"].as<std::string>().c_str());
    strcpy(config->m_clientPrincipal.UserID, yaml["client_access"]["UserID"].as<std::string>().c_str());
    strcpy(config->m_clientPrincipal.UserProductInfo,
           yaml["client_access"]["UserProductInfo"].as<std::string>().c_str());
    strcpy(config->m_clientPrincipal.AuthCode, yaml["client_access"]["AuthCode"].as<std::string>().c_str());
    strcpy(config->m_clientPrincipal.AppID, yaml["client_access"]["AppID"].as<std::string>().c_str());

    // user
    strcpy(config->m_userPrincipal.TradingDay, yaml["user_access"]["TradingDay"].as<std::string>().c_str());
    strcpy(config->m_userPrincipal.BrokerID, yaml["user_access"]["BrokerID"].as<std::string>().c_str());
    strcpy(config->m_userPrincipal.UserID, yaml["user_access"]["UserID"].as<std::string>().c_str());
    strcpy(config->m_userPrincipal.Password, yaml["user_access"]["Password"].as<std::string>().c_str());
    strcpy(config->m_userPrincipal.UserProductInfo, yaml["user_access"]["UserProductInfo"].as<std::string>().c_str());
    strcpy(config->m_userPrincipal.InterfaceProductInfo,
           yaml["user_access"]["InterfaceProductInfo"].as<std::string>().c_str());
    strcpy(config->m_userPrincipal.ProtocolInfo, yaml["user_access"]["ProtocolInfo"].as<std::string>().c_str());
    strcpy(config->m_userPrincipal.MacAddress, yaml["user_access"]["MacAddress"].as<std::string>().c_str());
    strcpy(config->m_userPrincipal.OneTimePassword, yaml["user_access"]["OneTimePassword"].as<std::string>().c_str());
    strcpy(config->m_userPrincipal.reserve1, yaml["user_access"]["reserve1"].as<std::string>().c_str());
    strcpy(config->m_userPrincipal.LoginRemark, yaml["user_access"]["LoginRemark"].as<std::string>().c_str());
    config->m_userPrincipal.ClientIPPort = yaml["user_access"]["ClientIPPort"].as<int>();
    strcpy(config->m_userPrincipal.ClientIPAddress, yaml["user_access"]["ClientIPAddress"].as<std::string>().c_str());

    // connect to hq
    config->front_hq_address = yaml["connect_info"]["front_hq_address"].as<std::string>();
    // connect to trade
    config->front_trade_address = yaml["connect_info"]["front_trade_address"].as<std::string>();
    // level1 hq services
    if (yaml["connect_info"]["level1_hq_services"])
    {
        config->level1_hq_services = yaml["connect_info"]["level1_hq_services"].as<std::vector<std::string>>();
    }
    else
    {
        config->level1_hq_services = {};
    }

    // limit
    config->fake_x = yaml["limit"]["fake_x"].as<int>();
    config->stop_loss = yaml["limit"]["stop_loss"].as<float>();
    config->stop_profit = yaml["limit"]["stop_profit"].as<float>();
    config->amt = yaml["limit"]["amt"].as<int>();
    config->x_times = yaml["limit"]["x_times"].as<int>();

    std::cout << "交易是否模拟: \t" << (config->fake_x > 0 ? "是" : "否") << std::endl;
    std::cout << "止损百分比: \t" << config->stop_loss << std::endl;
    std::cout << "止盈百分比: \t" << config->stop_profit << std::endl;
    std::cout << "单笔交易限制手数: \t" << config->amt << std::endl;
    std::cout << "日交易总次数: \t" << config->x_times << std::endl;

    return config;
}
