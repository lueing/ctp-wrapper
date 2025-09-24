#ifndef LUEING_DATA_PROVIDER_CTP_CONFIG_H
#define LUEING_DATA_PROVIDER_CTP_CONFIG_H

#include <string>
#include <atomic>
#include <memory>
#include "ThostFtdcUserApiStruct.h"

namespace lueing
{
    struct CtpConfig
    {
        CThostFtdcReqAuthenticateField m_clientPrincipal;
        CThostFtdcReqUserLoginField m_userPrincipal;
        std::string front_hq_address;
        std::string front_trade_address;
        std::atomic_int32_t hq_request_id;
        std::atomic_int32_t tx_request_id;
    };
    typedef struct CtpConfig CtpConfig;
    typedef std::shared_ptr<CtpConfig> CtpConfigPtr;

    CtpConfigPtr CreateCtpConfig(const std::string &yaml_file);
}

#endif // LUEING_DATA_PROVIDER_CTP_CONFIG_H