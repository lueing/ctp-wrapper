#ifndef LUEING_DATA_PROVIDER_CTP_HQ_H
#define LUEING_DATA_PROVIDER_CTP_HQ_H

#include "config.h"
#include <memory>

#include "ThostFtdcMdApi.h"
#include "events.h"
#include "lueing_iconv.h"

namespace lueing {
    class CtpHqHandler : public CThostFtdcMdSpi {
    public:
        void CreateHqContext();

        Events &GetEvents() { return events_; }

        CThostFtdcMdApi *GetUserMdApi() { return user_md_api_; }

        absl::node_hash_map<std::string, std::vector<CThostFtdcDepthMarketDataField>> &
        MarketData() { return market_data_; }

        std::mutex &MarketDataLock() { return market_data_lock_; }

    public:
        explicit CtpHqHandler(CtpConfigPtr config);
        ~CtpHqHandler();

    public:
        void OnFrontConnected() override;

        void OnFrontDisconnected(int nReason) override;

        void OnHeartBeatWarning(int nTimeLapse) override;

        void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                       bool bIsLast) override;

        void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                        bool bIsLast) override;

        void OnRspQryMulticastInstrument(CThostFtdcMulticastInstrumentField *pMulticastInstrument,
                                                 CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                                 bool bIsLast) override;

        void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo,
                           int nRequestID, bool bIsLast) override;

        void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo,
                             int nRequestID, bool bIsLast) override;

        void OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo,
                            int nRequestID, bool bIsLast) override;

        void OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo,
                              int nRequestID, bool bIsLast) override;

        void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) override;

        void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp) override;

    private:
        void ReqUserLogin();

    private:
        CThostFtdcMdApi *user_md_api_ = nullptr;
        CtpConfigPtr config_;
        Events events_;
        LueingIconv iconv_;
        absl::node_hash_map<std::string, std::vector<CThostFtdcDepthMarketDataField>> market_data_;
        std::mutex market_data_lock_;
    };

    class CtpHq {
    private:
        std::mutex lock_;
        CtpHqHandler hq_handler_;
        absl::node_hash_map<std::string, absl::node_hash_set<std::string>> instruments_booked_;

    public:
        explicit CtpHq(CtpConfigPtr config);

        ~CtpHq();

    public:
        // 订阅行情
        int SubscribeMarketData(const std::string &instrument, const std::string &subscriber);

        // 取消订阅行情
        int UnSubscribeMarketData(const std::string &instrument, const std::string &subscriber);

        // 等待数据
        void WaitForData(const std::string &instrument, const std::string &subscriber);

        // 市场数据
        absl::node_hash_map<std::string, std::vector<CThostFtdcDepthMarketDataField>> &
        MarketData() { return hq_handler_.MarketData(); };
        std::mutex &MarketDataLock() { return hq_handler_.MarketDataLock(); }
    };

    typedef std::shared_ptr<CtpHq> ProviderCtpPtr;

} // namespace lueing

#endif // LUEING_DATA_PROVIDER_CTP_HQ_H