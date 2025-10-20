#include "hq.h"

#include <spdlog/spdlog.h>
#include <cpr/cpr.h>
#include <fmt/ranges.h>
#include <nlohmann/json.hpp>

#include <utility>
#include "lueing_os.h"

lueing::CtpHq::CtpHq(CtpConfigPtr config) : hq_handler_(std::move(config))
{
    hq_handler_.CreateHqContext();
}

lueing::CtpHq::~CtpHq()
= default;

int lueing::CtpHq::SubscribeMarketData(const std::string &instrument, const std::string &subscriber)
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

int lueing::CtpHq::UnSubscribeMarketData(const std::string &instrument, const std::string &subscriber)
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

void lueing::CtpHq::WaitForData(const std::string &instrument, const std::string &subscriber)
{
    hq_handler_.GetEvents().Wait(instrument, subscriber);
}

void lueing::CtpHqHandler::CreateHqContext()
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

lueing::CtpHqHandler::CtpHqHandler(CtpConfigPtr config) : config_(std::move(config))
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

lueing::Level1Hq::Level1Hq(const lueing::CtpConfigPtr& config) {
    this->svc_address_ = config->level1_hq_services;
}

lueing::Level1Hq::~Level1Hq() = default;

void lueing::Level1Hq::poll(const std::vector<std::string>& codes, bool validate, std::vector<Quote> &out_quotes) {
    // randomly select a service address
    if (svc_address_.empty()) {
        spdlog::error("No service addresses available.");
        return;
    }
    // Shuffle the service addresses and pick the first one
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(svc_address_.begin(), svc_address_.end(), g);
    const std::string& address = svc_address_.front();
    // Make the HTTP GET request
    std::string joined_codes = fmt::format("{}", fmt::join(codes, ","));
    cpr::Response r = cpr::Get(cpr::Url{ address }, cpr::Parameters{{"codes", joined_codes}, {"validate", validate ? "true" : "false"}});
    if (r.status_code != 200) {
        spdlog::error("Failed to fetch data from {}: HTTP {}", address, r.status_code);
        return;
    }
    // Parse the JSON response
    try {
        auto json_response = nlohmann::json::parse(r.text);
        for (const auto& item : json_response["data"]) {
            Quote quote{};
            quote.price = item.value("lastPrice", 0.0);
            quote.volume = item.value("amount", 0LL);
            quote.turnover = item.value("money", 0.0);
            quote.open = item.value("open", 0.0);
            quote.high = item.value("high", 0.0);
            quote.low = item.value("low", 0.0);
            quote.pre_close = item.value("preClose", 0.0);
            quote.time = item.value("time", 0LL);
            quote.code = item.value("code", "");
            out_quotes.push_back(quote);
        }
    } catch (const std::exception& e) {
        spdlog::error("JSON parsing error: {}", e.what());
    }
}

lueing::Quote lueing::Level1Hq::get_latest_quote(const std::string &code) {
    std::vector<Quote> out_quotes;
    std::vector<std::string> codes = {code};
    poll(codes, false, out_quotes);
    if (!out_quotes.empty()) {
        return out_quotes.back();
    }
    throw std::runtime_error("Failed to get latest quote");
}
