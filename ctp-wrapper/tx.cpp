#include "tx.h"
#include <fmt/core.h>
#include "spdlog/spdlog.h"

lueing::CtpTx::CtpTx(CtpConfigPtr config) : tx_handler_(std::move(config))
{
    tx_handler_.CreateTxContext();
}

double lueing::CtpTx::Order(const std::string &exchange, const std::string &contract, TxDirection direction, double price, int amt) {
    return tx_handler_.Order(exchange, contract, direction, price, amt);
}

lueing::CtpTx::~CtpTx() = default;

lueing::CtpTxHandler::CtpTxHandler(CtpConfigPtr config) : config_(std::move(config))
{
}

lueing::CtpTxHandler::~CtpTxHandler()
{
    if (nullptr == user_tx_api_) {
        return;
    }
    user_tx_api_->Release();
    user_tx_api_ = nullptr;
}

void lueing::CtpTxHandler::CreateTxContext()
{
    if (nullptr != user_tx_api_) {
        return;
    }
    user_tx_api_ = CThostFtdcTraderApi::CreateFtdcTraderApi("./flow-tx/");
    user_tx_api_->RegisterSpi(this);
    user_tx_api_->SubscribePrivateTopic(THOST_TERT_QUICK);
    user_tx_api_->SubscribePublicTopic(THOST_TERT_QUICK);
    user_tx_api_->RegisterFront(const_cast<char*>(config_->front_trade_address.c_str()));
    user_tx_api_->Init();

    events_.WaitOnce(EVENT_TX_LOGIN);
}

void lueing::CtpTxHandler::OnFrontConnected() {
    int result = user_tx_api_->ReqAuthenticate(&config_->m_clientPrincipal, config_->tx_request_id.fetch_add(1));
    if (0 == result) {
        spdlog::info(fmt::format("[TX] FrontConnected and invoke check client success."));
    } else {
        spdlog::error(fmt::format("[TX] FrontConnected but invoke check client failed."));
    }
}

void lueing::CtpTxHandler::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo->ErrorID != 0) {
        std::string error_msg = gbk_to_utf8_converter_.GBK2UTF8(pRspInfo->ErrorMsg);
        spdlog::error(fmt::format("[TX] Client check failed. ErrorID=[{}], ErrorMsg=[{}]", pRspInfo->ErrorID, error_msg));
        return;
    }
    spdlog::info(fmt::format("[TX] Client check success."));
    // => 客户端认证成功后，发起登录请求
    int result = user_tx_api_->ReqUserLogin(&config_->m_userPrincipal, config_->tx_request_id.fetch_add(1));
    if (0 == result) {
        spdlog::info(fmt::format("[TX] Invoke user login success."));
    } else {
        spdlog::error(fmt::format("[TX] Invoke user login failed."));
    }
}

void lueing::CtpTxHandler::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo->ErrorID != 0) {
        std::string error_msg = gbk_to_utf8_converter_.GBK2UTF8(pRspInfo->ErrorMsg);
        spdlog::error(fmt::format("[TX] User login failed. ErrorID=[{}], ErrorMsg=[{}]", pRspInfo->ErrorID, error_msg));
        return;
    }
    spdlog::info(fmt::format("[TX] User login success. TradingDay=[{}], LoginTime=[{}], BrokerID=[{}], UserID=[{}], SystemName=[{}]",
        pRspUserLogin->TradingDay,
        pRspUserLogin->LoginTime,
        pRspUserLogin->BrokerID,
        pRspUserLogin->UserID,
        pRspUserLogin->SystemName));
    // 确认结算单
    CThostFtdcSettlementInfoConfirmField Confirm{};

    strcpy(Confirm.BrokerID, pRspUserLogin->BrokerID);
    strcpy(Confirm.InvestorID, pRspUserLogin->UserID);
    int result = user_tx_api_->ReqSettlementInfoConfirm(&Confirm, config_->tx_request_id.fetch_add(1));
    if (result == 0) {
        spdlog::info(fmt::format("[TX] Confirm success."));
    } else {
        spdlog::info(fmt::format("[TX] Confirm fail."));
    }
    events_.Notify(EVENT_TX_LOGIN);
}

double lueing::CtpTxHandler::Order(const std::string &exchange, const std::string &contract, lueing::TxDirection direction, double price, int amt) {
    CThostFtdcInputOrderField ord = {};
    int orderRef = config_->tx_request_id.fetch_add(1);
    double avg = -1;

    strcpy(ord.BrokerID, config_->m_userPrincipal.BrokerID);
    strcpy(ord.InvestorID, config_->m_userPrincipal.reserve1);
    strcpy(ord.InstrumentID, contract.c_str());
    strcpy(ord.UserID, config_->m_userPrincipal.UserID);
    strcpy(ord.ExchangeID, exchange.c_str());
    sprintf(ord.OrderRef, "%d", orderRef);
    ord.RequestID = config_->tx_request_id.fetch_add(1);

    ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    ord.Direction = direction;
    ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
    ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    ord.LimitPrice = price;
    ord.VolumeTotalOriginal = amt;
    ord.TimeCondition = THOST_FTDC_TC_GFD;               // 当日有效
    ord.VolumeCondition = THOST_FTDC_VC_AV;              // 任何数量
    ord.MinVolume = 1;
    ord.ContingentCondition = THOST_FTDC_CC_Immediately; //立即
    ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose; //非强平
    ord.IsAutoSuspend = 0;
    int a = user_tx_api_->ReqOrderInsert(&ord, config_->tx_request_id.fetch_add(1));
    spdlog::info(fmt::format((a == 0) ? "[TX] 报单成功，序号=[{}], 合约:{} 数量:{} 方向: {}" : "[TX] 报单失败，序号=[{}] 合约:{} 数量:{}",
                             a, contract, amt, TX_PUT == direction ? "空" : "多"));

    std::string order = fmt::format("{}", orderRef);
    events_.Wait(contract, fmt::format("{}", order));

    auto it = trade_data_.find(order);

    if (it != trade_data_.end()) {
        int total_volume = 0;
        double total_price = 0;
        for (const auto& trade : it->second) {
            total_volume += trade.Volume;
            total_price += trade.Price * trade.Volume;
        }
        if (total_volume > 0) {
            avg = total_price / total_volume;
        }
    }    

    // round to 2 decimal places
    if (avg > 0) {
        avg = std::round(price * 100) / 100.0;
    }
    return avg;
}

void lueing::CtpTxHandler::OnRtnTrade(CThostFtdcTradeField *pTrade) {
    if (nullptr == pTrade) {
        return;
    }
    spdlog::info(fmt::format("[TX] 成交，合约:{} 数量:{} 价格:{}", pTrade->InstrumentID, pTrade->Volume, pTrade->Price));
    trade_data_[pTrade->OrderRef].push_back(*pTrade);
    if (trade_data_.contains(pTrade->OrderRef)) {
        events_.Notify(pTrade->InstrumentID);
    }
}

void lueing::CtpTxHandler::OnRtnOrder(CThostFtdcOrderField *pOrder) {
    if (THOST_FTDC_OST_AllTraded == pOrder->OrderStatus) {
        trade_finish_.emplace(pOrder->OrderRef);
    }
}

// Empty implementations for all CThostFtdcTraderSpi methods
// void lueing::CtpTxHandler::OnFrontConnected() {}
void lueing::CtpTxHandler::OnFrontDisconnected(int nReason) {}
void lueing::CtpTxHandler::OnHeartBeatWarning(int nTimeLapse) {}
// void lueing::CtpTxHandler::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
// void lueing::CtpTxHandler::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField *pUserPasswordUpdate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspTradingAccountPasswordUpdate(CThostFtdcTradingAccountPasswordUpdateField *pTradingAccountPasswordUpdate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspUserAuthMethod(CThostFtdcRspUserAuthMethodField *pRspUserAuthMethod, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspGenUserCaptcha(CThostFtdcRspGenUserCaptchaField *pRspGenUserCaptcha, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspGenUserText(CThostFtdcRspGenUserTextField *pRspGenUserText, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspParkedOrderInsert(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryMaxOrderVolume(CThostFtdcQryMaxOrderVolumeField *pQryMaxOrderVolume, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspRemoveParkedOrder(CThostFtdcRemoveParkedOrderField *pRemoveParkedOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspRemoveParkedOrderAction(CThostFtdcRemoveParkedOrderActionField *pRemoveParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspExecOrderAction(CThostFtdcInputExecOrderActionField *pInputExecOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspForQuoteInsert(CThostFtdcInputForQuoteField *pInputForQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQuoteAction(CThostFtdcInputQuoteActionField *pInputQuoteAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspBatchOrderAction(CThostFtdcInputBatchOrderActionField *pInputBatchOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspOptionSelfCloseInsert(CThostFtdcInputOptionSelfCloseField *pInputOptionSelfClose, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspOptionSelfCloseAction(CThostFtdcInputOptionSelfCloseActionField *pInputOptionSelfCloseAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspCombActionInsert(CThostFtdcInputCombActionField *pInputCombAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryInvestor(CThostFtdcInvestorField *pInvestor, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryTradingCode(CThostFtdcTradingCodeField *pTradingCode, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryProduct(CThostFtdcProductField *pProduct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryTraderOffer(CThostFtdcTraderOfferField *pTraderOffer, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryTransferBank(CThostFtdcTransferBankField *pTransferBank, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryNotice(CThostFtdcNoticeField *pNotice, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryInvestorPositionCombineDetail(CThostFtdcInvestorPositionCombineDetailField *pInvestorPositionCombineDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryCFMMCTradingAccountKey(CThostFtdcCFMMCTradingAccountKeyField *pCFMMCTradingAccountKey, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryEWarrantOffset(CThostFtdcEWarrantOffsetField *pEWarrantOffset, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryInvestorProductGroupMargin(CThostFtdcInvestorProductGroupMarginField *pInvestorProductGroupMargin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryExchangeMarginRate(CThostFtdcExchangeMarginRateField *pExchangeMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryExchangeMarginRateAdjust(CThostFtdcExchangeMarginRateAdjustField *pExchangeMarginRateAdjust, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryExchangeRate(CThostFtdcExchangeRateField *pExchangeRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQrySecAgentACIDMap(CThostFtdcSecAgentACIDMapField *pSecAgentACIDMap, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryProductExchRate(CThostFtdcProductExchRateField *pProductExchRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryProductGroup(CThostFtdcProductGroupField *pProductGroup, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryMMInstrumentCommissionRate(CThostFtdcMMInstrumentCommissionRateField *pMMInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryMMOptionInstrCommRate(CThostFtdcMMOptionInstrCommRateField *pMMOptionInstrCommRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryInstrumentOrderCommRate(CThostFtdcInstrumentOrderCommRateField *pInstrumentOrderCommRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQrySecAgentTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQrySecAgentCheckMode(CThostFtdcSecAgentCheckModeField *pSecAgentCheckMode, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQrySecAgentTradeInfo(CThostFtdcSecAgentTradeInfoField *pSecAgentTradeInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryOptionInstrTradeCost(CThostFtdcOptionInstrTradeCostField *pOptionInstrTradeCost, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryOptionInstrCommRate(CThostFtdcOptionInstrCommRateField *pOptionInstrCommRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryExecOrder(CThostFtdcExecOrderField *pExecOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryForQuote(CThostFtdcForQuoteField *pForQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryQuote(CThostFtdcQuoteField *pQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryOptionSelfClose(CThostFtdcOptionSelfCloseField *pOptionSelfClose, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryInvestUnit(CThostFtdcInvestUnitField *pInvestUnit, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQrySPBMFutureParameter(CThostFtdcSPBMFutureParameterField *pSPBMFutureParameter, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQrySPBMOptionParameter(CThostFtdcSPBMOptionParameterField *pSPBMOptionParameter, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQrySPBMIntraParameter(CThostFtdcSPBMIntraParameterField *pSPBMIntraParameter, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQrySPBMInterParameter(CThostFtdcSPBMInterParameterField *pSPBMInterParameter, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQrySPBMPortfDefinition(CThostFtdcSPBMPortfDefinitionField *pSPBMPortfDefinition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQrySPBMInvestorPortfDef(CThostFtdcSPBMInvestorPortfDefField *pSPBMInvestorPortfDef, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryInvestorPortfMarginRatio(CThostFtdcInvestorPortfMarginRatioField *pInvestorPortfMarginRatio, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryInvestorProdSPBMDetail(CThostFtdcInvestorProdSPBMDetailField *pInvestorProdSPBMDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryInvestorCommoditySPMMMargin(CThostFtdcInvestorCommoditySPMMMarginField *pInvestorCommoditySPMMMargin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryInvestorCommodityGroupSPMMMargin(CThostFtdcInvestorCommodityGroupSPMMMarginField *pInvestorCommodityGroupSPMMMargin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQrySPMMInstParam(CThostFtdcSPMMInstParamField *pSPMMInstParam, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQrySPMMProductParam(CThostFtdcSPMMProductParamField *pSPMMProductParam, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQrySPBMAddOnInterParameter(CThostFtdcSPBMAddOnInterParameterField *pSPBMAddOnInterParameter, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryRCAMSCombProductInfo(CThostFtdcRCAMSCombProductInfoField *pRCAMSCombProductInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryRCAMSInstrParameter(CThostFtdcRCAMSInstrParameterField *pRCAMSInstrParameter, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryRCAMSIntraParameter(CThostFtdcRCAMSIntraParameterField *pRCAMSIntraParameter, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryRCAMSInterParameter(CThostFtdcRCAMSInterParameterField *pRCAMSInterParameter, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryRCAMSShortOptAdjustParam(CThostFtdcRCAMSShortOptAdjustParamField *pRCAMSShortOptAdjustParam, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryRCAMSInvestorCombPosition(CThostFtdcRCAMSInvestorCombPositionField *pRCAMSInvestorCombPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryInvestorProdRCAMSMargin(CThostFtdcInvestorProdRCAMSMarginField *pInvestorProdRCAMSMargin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryRULEInstrParameter(CThostFtdcRULEInstrParameterField *pRULEInstrParameter, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryRULEIntraParameter(CThostFtdcRULEIntraParameterField *pRULEIntraParameter, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryRULEInterParameter(CThostFtdcRULEInterParameterField *pRULEInterParameter, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryInvestorProdRULEMargin(CThostFtdcInvestorProdRULEMarginField *pInvestorProdRULEMargin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryCombInstrumentGuard(CThostFtdcCombInstrumentGuardField *pCombInstrumentGuard, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryCombAction(CThostFtdcCombActionField *pCombAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryTransferSerial(CThostFtdcTransferSerialField *pTransferSerial, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryAccountregister(CThostFtdcAccountregisterField *pAccountregister, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
// void lueing::CtpTxHandler::OnRtnOrder(CThostFtdcOrderField *pOrder) {}
// void lueing::CtpTxHandler::OnRtnTrade(CThostFtdcTradeField *pTrade) {}
void lueing::CtpTxHandler::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo) {}
void lueing::CtpTxHandler::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo) {}
void lueing::CtpTxHandler::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus) {}
void lueing::CtpTxHandler::OnRtnBulletin(CThostFtdcBulletinField *pBulletin) {}
void lueing::CtpTxHandler::OnRtnTradingNotice(CThostFtdcTradingNoticeInfoField *pTradingNoticeInfo) {}
void lueing::CtpTxHandler::OnRtnErrorConditionalOrder(CThostFtdcErrorConditionalOrderField *pErrorConditionalOrder) {}
void lueing::CtpTxHandler::OnRtnExecOrder(CThostFtdcExecOrderField *pExecOrder) {}
void lueing::CtpTxHandler::OnErrRtnExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo) {}
void lueing::CtpTxHandler::OnErrRtnExecOrderAction(CThostFtdcExecOrderActionField *pExecOrderAction, CThostFtdcRspInfoField *pRspInfo) {}
void lueing::CtpTxHandler::OnErrRtnForQuoteInsert(CThostFtdcInputForQuoteField *pInputForQuote, CThostFtdcRspInfoField *pRspInfo) {}
void lueing::CtpTxHandler::OnRtnQuote(CThostFtdcQuoteField *pQuote) {}
void lueing::CtpTxHandler::OnErrRtnQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo) {}
void lueing::CtpTxHandler::OnErrRtnQuoteAction(CThostFtdcQuoteActionField *pQuoteAction, CThostFtdcRspInfoField *pRspInfo) {}
void lueing::CtpTxHandler::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp) {}
void lueing::CtpTxHandler::OnRtnCFMMCTradingAccountToken(CThostFtdcCFMMCTradingAccountTokenField *pCFMMCTradingAccountToken) {}
void lueing::CtpTxHandler::OnErrRtnBatchOrderAction(CThostFtdcBatchOrderActionField *pBatchOrderAction, CThostFtdcRspInfoField *pRspInfo) {}
void lueing::CtpTxHandler::OnRtnOptionSelfClose(CThostFtdcOptionSelfCloseField *pOptionSelfClose) {}
void lueing::CtpTxHandler::OnErrRtnOptionSelfCloseInsert(CThostFtdcInputOptionSelfCloseField *pInputOptionSelfClose, CThostFtdcRspInfoField *pRspInfo) {}
void lueing::CtpTxHandler::OnErrRtnOptionSelfCloseAction(CThostFtdcOptionSelfCloseActionField *pOptionSelfCloseAction, CThostFtdcRspInfoField *pRspInfo) {}
void lueing::CtpTxHandler::OnRtnCombAction(CThostFtdcCombActionField *pCombAction) {}
void lueing::CtpTxHandler::OnErrRtnCombActionInsert(CThostFtdcInputCombActionField *pInputCombAction, CThostFtdcRspInfoField *pRspInfo) {}
void lueing::CtpTxHandler::OnRspQryContractBank(CThostFtdcContractBankField *pContractBank, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryParkedOrder(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryTradingNotice(CThostFtdcTradingNoticeField *pTradingNotice, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryBrokerTradingParams(CThostFtdcBrokerTradingParamsField *pBrokerTradingParams, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryBrokerTradingAlgos(CThostFtdcBrokerTradingAlgosField *pBrokerTradingAlgos, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQueryCFMMCTradingAccountToken(CThostFtdcQueryCFMMCTradingAccountTokenField *pQueryCFMMCTradingAccountToken, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRtnFromBankToFutureByBank(CThostFtdcRspTransferField *pRspTransfer) {}
void lueing::CtpTxHandler::OnRtnFromFutureToBankByBank(CThostFtdcRspTransferField *pRspTransfer) {}
void lueing::CtpTxHandler::OnRtnRepealFromBankToFutureByBank(CThostFtdcRspRepealField *pRspRepeal) {}
void lueing::CtpTxHandler::OnRtnRepealFromFutureToBankByBank(CThostFtdcRspRepealField *pRspRepeal) {}
void lueing::CtpTxHandler::OnRtnFromBankToFutureByFuture(CThostFtdcRspTransferField *pRspTransfer) {}
void lueing::CtpTxHandler::OnRtnFromFutureToBankByFuture(CThostFtdcRspTransferField *pRspTransfer) {}
void lueing::CtpTxHandler::OnRtnRepealFromBankToFutureByFutureManual(CThostFtdcRspRepealField *pRspRepeal) {}
void lueing::CtpTxHandler::OnRtnRepealFromFutureToBankByFutureManual(CThostFtdcRspRepealField *pRspRepeal) {}
void lueing::CtpTxHandler::OnRtnQueryBankBalanceByFuture(CThostFtdcNotifyQueryAccountField *pNotifyQueryAccount) {}
void lueing::CtpTxHandler::OnErrRtnBankToFutureByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo) {}
void lueing::CtpTxHandler::OnErrRtnFutureToBankByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo) {}
void lueing::CtpTxHandler::OnErrRtnRepealBankToFutureByFutureManual(CThostFtdcReqRepealField *pReqRepeal, CThostFtdcRspInfoField *pRspInfo) {}
void lueing::CtpTxHandler::OnErrRtnRepealFutureToBankByFutureManual(CThostFtdcReqRepealField *pReqRepeal, CThostFtdcRspInfoField *pRspInfo) {}
void lueing::CtpTxHandler::OnErrRtnQueryBankBalanceByFuture(CThostFtdcReqQueryAccountField *pReqQueryAccount, CThostFtdcRspInfoField *pRspInfo) {}
void lueing::CtpTxHandler::OnRtnRepealFromBankToFutureByFuture(CThostFtdcRspRepealField *pRspRepeal) {}
void lueing::CtpTxHandler::OnRtnRepealFromFutureToBankByFuture(CThostFtdcRspRepealField *pRspRepeal) {}
void lueing::CtpTxHandler::OnRspFromBankToFutureByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspFromFutureToBankByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQueryBankAccountMoneyByFuture(CThostFtdcReqQueryAccountField *pReqQueryAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRtnOpenAccountByBank(CThostFtdcOpenAccountField *pOpenAccount) {}
void lueing::CtpTxHandler::OnRtnCancelAccountByBank(CThostFtdcCancelAccountField *pCancelAccount) {}
void lueing::CtpTxHandler::OnRtnChangeAccountByBank(CThostFtdcChangeAccountField *pChangeAccount) {}
void lueing::CtpTxHandler::OnRspQryClassifiedInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryCombPromotionParam(CThostFtdcCombPromotionParamField *pCombPromotionParam, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryRiskSettleInvstPosition(CThostFtdcRiskSettleInvstPositionField *pRiskSettleInvstPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
void lueing::CtpTxHandler::OnRspQryRiskSettleProductStatus(CThostFtdcRiskSettleProductStatusField *pRiskSettleProductStatus, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}
