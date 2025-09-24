#include "hq.h"

#include <spdlog/spdlog.h>

#include <utility>
#include "lueing_os.h"

lueing::ProviderCtp::ProviderCtp(CtpConfigPtr config) : hq_handler_(std::move(config))
{
    hq_handler_.Init();
}

lueing::ProviderCtp::~ProviderCtp()
= default;

int lueing::ProviderCtp::SubscribeMarketData(const std::string &instrument, const std::string &subscriber)
{
    std::unique_lock<std::mutex> lock(lock_);
    if (instruments_booked_.contains(instrument))
    {
        instruments_booked_[instrument].insert(subscriber);
        return 0;
    }
    char *instruments_pptr[1] = {const_cast<char *>(instrument.c_str())};
    int result = hq_handler_.GetUserMdApi()->SubscribeMarketData(instruments_pptr, 1);

    if (0 == result)
    {
        instruments_booked_[instrument].insert(subscriber);
        spdlog::info(fmt::format("[行情接口][订阅行情] 请求成功!"));
    }
    else
    {
        spdlog::info(fmt::format("[行情接口][订阅行情] 请求失败，错误序号=[{}]", result));
    }
    return result;
}

int lueing::ProviderCtp::UnSubscribeMarketData(const std::string &instrument, const std::string &subscriber)
{
    std::unique_lock<std::mutex> lock(lock_);
    int result = 0;
    if (!instruments_booked_.contains(instrument))
    {
        return result;
    }
    // 不是最后一位订阅者
    if (instruments_booked_[instrument].size() > 1)
    {
        instruments_booked_[instrument].erase(subscriber);
        return result;
    }
    // 最后一位订阅者, 并且就是当前订阅者
    if (instruments_booked_[instrument].contains(subscriber))
    {
        char *instruments_pptr[1] = {const_cast<char *>(instrument.c_str())};
        result = hq_handler_.GetUserMdApi()->UnSubscribeMarketData(instruments_pptr, 1);

        if (0 == result)
        {
            instruments_booked_[instrument].erase(subscriber);
            spdlog::info(fmt::format("[行情接口][订阅行情] 请求成功!"));
        }
        else
        {
            spdlog::info(fmt::format("[行情接口][订阅行情] 请求失败，错误序号=[{}]", result));
        }
    }

    return result;
}

void lueing::ProviderCtp::WaitForData(const std::string &instrument, const std::string &subscriber)
{
    hq_handler_.GetEvents().Wait(instrument, subscriber);
}

void lueing::CtpHqHandler::Init()
{
#define HQ_FLOW_PATH "./flow-hq/"

    if (!lueing::filesystem::FilesExists(HQ_FLOW_PATH))
    {
        lueing::filesystem::Mkdirs(HQ_FLOW_PATH);
    }

    user_md_api_ = CThostFtdcMdApi::CreateFtdcMdApi(HQ_FLOW_PATH, false, false);
    user_md_api_->RegisterSpi(this);
    user_md_api_->RegisterFront(const_cast<char *>(config_->front_hq_address.c_str()));
    // => 触发 OnFrontConnected
    user_md_api_->Init();
    events_.WaitOnce(EVENT_LOGIN);
}

lueing::CtpHqHandler::CtpHqHandler(std::shared_ptr<CtpConfig> config) : config_(std::move(config))
{
}

lueing::CtpHqHandler::~CtpHqHandler()
{
    if (nullptr != user_md_api_)
    {
        user_md_api_->Release();
        user_md_api_ = nullptr;
    }
}

void lueing::CtpHqHandler::OnFrontConnected()
{
    ReqUserLogin();
}

void lueing::CtpHqHandler::OnFrontDisconnected(int nReason)
{
}

void lueing::CtpHqHandler::OnHeartBeatWarning(int nTimeLapse)
{
}

void lueing::CtpHqHandler::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if (pRspInfo->ErrorID != 0)
    {
        spdlog::info(fmt::format("[行情接口][登录中...] 登录失败, 错误码: {}, 错误信息: {}", pRspInfo->ErrorID,
                                 iconv_.GBK2UTF8(pRspInfo->ErrorMsg)));
        exit(1);
    }
    spdlog::info(fmt::format("[行情接口][登录中...] 登录成功, 交易日: {}", pRspUserLogin->TradingDay));
    events_.NotifyOnce(EVENT_LOGIN);
}

void lueing::CtpHqHandler::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
}

void lueing::CtpHqHandler::OnRspQryMulticastInstrument(CThostFtdcMulticastInstrumentField *pMulticastInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
}

void lueing::CtpHqHandler::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
}

void lueing::CtpHqHandler::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
}

void lueing::CtpHqHandler::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
}

void lueing::CtpHqHandler::OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
}

void lueing::CtpHqHandler::OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
}

void lueing::CtpHqHandler::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
    if (pDepthMarketData)
    {
        std::unique_lock<std::mutex> lock(market_data_lock_);
        std::string instrument = pDepthMarketData->InstrumentID;
        market_data_[instrument].push_back(*pDepthMarketData);
        events_.Notify(instrument);
    }
}

void lueing::CtpHqHandler::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp)
{
}

void lueing::CtpHqHandler::ReqUserLogin()
{
    int result_code = user_md_api_->ReqUserLogin(&config_->m_userPrincipal, config_->hq_request_id.fetch_add(1));
    if (0 == result_code)
    {
        spdlog::info(fmt::format("[行情接口][登录中...] 登录调用成功!"));
    }
    else
    {
        spdlog::info(fmt::format("[行情接口][登录中...] 登录调用失败, 返回码: {}", result_code));
    }
}
