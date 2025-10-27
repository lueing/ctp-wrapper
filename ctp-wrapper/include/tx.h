#ifndef LUEING_CTP_TX_H
#define LUEING_CTP_TX_H

#include "config.h"
#include "lueing_iconv.h"
#include "events.h"
#include "ThostFtdcTraderApi.h"

namespace lueing {

#define EVENT_TX_LOGIN "EVENT_TX_LOGIN"
#define TX_PUT '1'
#define TX_CALL '0'

    typedef char TxDirection;

    class CtpTxHandler : public CThostFtdcTraderSpi {
    private:
        CtpConfigPtr config_;
        LueingIconv gbk_to_utf8_converter_;
        Events events_;
        absl::node_hash_map<std::string, std::vector<CThostFtdcTradeField>> trade_data_;
        absl::node_hash_set<std::string> trade_finish_;
        CThostFtdcTraderApi *user_tx_api_ = nullptr;

    public:
        explicit CtpTxHandler(CtpConfigPtr config);

        ~CtpTxHandler();

    public:
        void CreateTxContext();

        double Order(const std::string &contract, TxDirection direction, int amt);       

    public:
        /// 当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
        void OnFrontConnected() override;

        /// 当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
        ///@param nReason 错误原因
        ///         0x1001 网络读失败
        ///         0x1002 网络写失败
        ///         0x2001 接收心跳超时
        ///         0x2002 发送心跳失败
        ///         0x2003 收到错误报文
        void OnFrontDisconnected(int nReason) override;

        /// 心跳超时警告。当长时间未收到报文时，该方法被调用。
        ///@param nTimeLapse 距离上次接收报文的时间
        void OnHeartBeatWarning(int nTimeLapse) override;

        /// 客户端认证响应
        void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo,
                               int nRequestID, bool bIsLast) override;

        /// 登录请求响应
        void
        OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                       bool bIsLast) override;

        /// 登出请求响应
        void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                             bool bIsLast) override;

        /// 用户口令更新请求响应
        void
        OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField *pUserPasswordUpdate,
                                CThostFtdcRspInfoField *pRspInfo,
                                int nRequestID, bool bIsLast) override;

        /// 资金账户口令更新请求响应
        void
        OnRspTradingAccountPasswordUpdate(CThostFtdcTradingAccountPasswordUpdateField *pTradingAccountPasswordUpdate,
                                          CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 查询用户当前支持的认证模式的回复
        void OnRspUserAuthMethod(CThostFtdcRspUserAuthMethodField *pRspUserAuthMethod, CThostFtdcRspInfoField *pRspInfo,
                                 int nRequestID, bool bIsLast) override;

        /// 获取图形验证码请求的回复
        void OnRspGenUserCaptcha(CThostFtdcRspGenUserCaptchaField *pRspGenUserCaptcha, CThostFtdcRspInfoField *pRspInfo,
                                 int nRequestID, bool bIsLast) override;

        /// 获取短信验证码请求的回复
        void
        OnRspGenUserText(CThostFtdcRspGenUserTextField *pRspGenUserText, CThostFtdcRspInfoField *pRspInfo,
                         int nRequestID,
                         bool bIsLast) override;

        /// 报单录入请求响应
        void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                              bool bIsLast) override;

        /// 预埋单录入请求响应
        void
        OnRspParkedOrderInsert(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo,
                               int nRequestID,
                               bool bIsLast) override;

        /// 预埋撤单录入请求响应
        void
        OnRspParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction, CThostFtdcRspInfoField *pRspInfo,
                               int nRequestID, bool bIsLast) override;

        /// 报单操作请求响应
        void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo,
                              int nRequestID, bool bIsLast) override;

        /// 查询最大报单数量响应
        void
        OnRspQryMaxOrderVolume(CThostFtdcQryMaxOrderVolumeField *pQryMaxOrderVolume, CThostFtdcRspInfoField *pRspInfo,
                               int nRequestID, bool bIsLast) override;

        /// 投资者结算结果确认响应
        void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
                                        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 删除预埋单响应
        void
        OnRspRemoveParkedOrder(CThostFtdcRemoveParkedOrderField *pRemoveParkedOrder, CThostFtdcRspInfoField *pRspInfo,
                               int nRequestID, bool bIsLast) override;

        /// 删除预埋撤单响应
        void OnRspRemoveParkedOrderAction(CThostFtdcRemoveParkedOrderActionField *pRemoveParkedOrderAction,
                                          CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 执行宣告录入请求响应
        void OnRspExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) override;

        /// 执行宣告操作请求响应
        void
        OnRspExecOrderAction(CThostFtdcInputExecOrderActionField *pInputExecOrderAction,
                             CThostFtdcRspInfoField *pRspInfo,
                             int nRequestID, bool bIsLast) override;

        /// 询价录入请求响应
        void
        OnRspForQuoteInsert(CThostFtdcInputForQuoteField *pInputForQuote, CThostFtdcRspInfoField *pRspInfo,
                            int nRequestID,
                            bool bIsLast) override;

        /// 报价录入请求响应
        void OnRspQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                              bool bIsLast) override;

        /// 报价操作请求响应
        void OnRspQuoteAction(CThostFtdcInputQuoteActionField *pInputQuoteAction, CThostFtdcRspInfoField *pRspInfo,
                              int nRequestID, bool bIsLast) override;

        /// 批量报单操作请求响应
        void OnRspBatchOrderAction(CThostFtdcInputBatchOrderActionField *pInputBatchOrderAction,
                                   CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 期权自对冲录入请求响应
        void OnRspOptionSelfCloseInsert(CThostFtdcInputOptionSelfCloseField *pInputOptionSelfClose,
                                        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 期权自对冲操作请求响应
        void OnRspOptionSelfCloseAction(CThostFtdcInputOptionSelfCloseActionField *pInputOptionSelfCloseAction,
                                        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 申请组合录入请求响应
        void OnRspCombActionInsert(CThostFtdcInputCombActionField *pInputCombAction, CThostFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast) override;

        /// 请求查询报单响应
        void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                           bool bIsLast) override;

        /// 请求查询成交响应
        void OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                           bool bIsLast) override;

        /// 请求查询投资者持仓响应
        void
        OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo,
                                 int nRequestID, bool bIsLast) override;

        /// 请求查询资金账户响应
        void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo,
                                    int nRequestID, bool bIsLast) override;

        /// 请求查询投资者响应
        void OnRspQryInvestor(CThostFtdcInvestorField *pInvestor, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                              bool bIsLast) override;

        /// 请求查询交易编码响应
        void
        OnRspQryTradingCode(CThostFtdcTradingCodeField *pTradingCode, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                            bool bIsLast) override;

        /// 请求查询合约保证金率响应
        void OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate,
                                          CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 请求查询合约手续费率响应
        void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate,
                                              CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 请求查询交易所响应
        void OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                              bool bIsLast) override;

        /// 请求查询产品响应
        void OnRspQryProduct(CThostFtdcProductField *pProduct, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                             bool bIsLast) override;

        /// 请求查询合约响应
        void
        OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                           bool bIsLast) override;

        /// 请求查询行情响应
        void OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo,
                                     int nRequestID, bool bIsLast) override;

        /// 请求查询交易员报盘机响应
        void
        OnRspQryTraderOffer(CThostFtdcTraderOfferField *pTraderOffer, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                            bool bIsLast) override;

        /// 请求查询投资者结算结果响应
        void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo,
                                    int nRequestID, bool bIsLast) override;

        /// 请求查询转帐银行响应
        void
        OnRspQryTransferBank(CThostFtdcTransferBankField *pTransferBank, CThostFtdcRspInfoField *pRspInfo,
                             int nRequestID,
                             bool bIsLast) override;

        /// 请求查询投资者持仓明细响应
        void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail,
                                            CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 请求查询客户通知响应
        void OnRspQryNotice(CThostFtdcNoticeField *pNotice, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                            bool bIsLast) override;

        /// 请求查询结算信息确认响应
        void OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
                                           CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 请求查询投资者持仓明细响应
        void
        OnRspQryInvestorPositionCombineDetail(
                CThostFtdcInvestorPositionCombineDetailField *pInvestorPositionCombineDetail,
                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 查询保证金监管系统经纪公司资金账户密钥响应
        void OnRspQryCFMMCTradingAccountKey(CThostFtdcCFMMCTradingAccountKeyField *pCFMMCTradingAccountKey,
                                            CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 请求查询仓单折抵信息响应
        void OnRspQryEWarrantOffset(CThostFtdcEWarrantOffsetField *pEWarrantOffset, CThostFtdcRspInfoField *pRspInfo,
                                    int nRequestID, bool bIsLast) override;

        /// 请求查询投资者品种/跨品种保证金响应
        void OnRspQryInvestorProductGroupMargin(CThostFtdcInvestorProductGroupMarginField *pInvestorProductGroupMargin,
                                                CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                                bool bIsLast) override;

        /// 请求查询交易所保证金率响应
        void
        OnRspQryExchangeMarginRate(CThostFtdcExchangeMarginRateField *pExchangeMarginRate,
                                   CThostFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast) override;

        /// 请求查询交易所调整保证金率响应
        void OnRspQryExchangeMarginRateAdjust(CThostFtdcExchangeMarginRateAdjustField *pExchangeMarginRateAdjust,
                                              CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 请求查询汇率响应
        void
        OnRspQryExchangeRate(CThostFtdcExchangeRateField *pExchangeRate, CThostFtdcRspInfoField *pRspInfo,
                             int nRequestID,
                             bool bIsLast) override;

        /// 请求查询二级代理操作员银期权限响应
        void OnRspQrySecAgentACIDMap(CThostFtdcSecAgentACIDMapField *pSecAgentACIDMap, CThostFtdcRspInfoField *pRspInfo,
                                     int nRequestID, bool bIsLast) override;

        /// 请求查询产品报价汇率
        void OnRspQryProductExchRate(CThostFtdcProductExchRateField *pProductExchRate, CThostFtdcRspInfoField *pRspInfo,
                                     int nRequestID, bool bIsLast) override;

        /// 请求查询产品组
        void
        OnRspQryProductGroup(CThostFtdcProductGroupField *pProductGroup, CThostFtdcRspInfoField *pRspInfo,
                             int nRequestID,
                             bool bIsLast) override;

        /// 请求查询做市商合约手续费率响应
        void OnRspQryMMInstrumentCommissionRate(CThostFtdcMMInstrumentCommissionRateField *pMMInstrumentCommissionRate,
                                                CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                                bool bIsLast) override;

        /// 请求查询做市商期权合约手续费响应
        void OnRspQryMMOptionInstrCommRate(CThostFtdcMMOptionInstrCommRateField *pMMOptionInstrCommRate,
                                           CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 请求查询报单手续费响应
        void OnRspQryInstrumentOrderCommRate(CThostFtdcInstrumentOrderCommRateField *pInstrumentOrderCommRate,
                                             CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 请求查询资金账户响应
        void
        OnRspQrySecAgentTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo,
                                       int nRequestID, bool bIsLast) override;

        /// 请求查询二级代理商资金校验模式响应
        void
        OnRspQrySecAgentCheckMode(CThostFtdcSecAgentCheckModeField *pSecAgentCheckMode,
                                  CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) override;

        /// 请求查询二级代理商信息响应
        void
        OnRspQrySecAgentTradeInfo(CThostFtdcSecAgentTradeInfoField *pSecAgentTradeInfo,
                                  CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) override;

        /// 请求查询期权交易成本响应
        void OnRspQryOptionInstrTradeCost(CThostFtdcOptionInstrTradeCostField *pOptionInstrTradeCost,
                                          CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 请求查询期权合约手续费响应
        void OnRspQryOptionInstrCommRate(CThostFtdcOptionInstrCommRateField *pOptionInstrCommRate,
                                         CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 请求查询执行宣告响应
        void OnRspQryExecOrder(CThostFtdcExecOrderField *pExecOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                               bool bIsLast) override;

        /// 请求查询询价响应
        void OnRspQryForQuote(CThostFtdcForQuoteField *pForQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                              bool bIsLast) override;

        /// 请求查询报价响应
        void OnRspQryQuote(CThostFtdcQuoteField *pQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                           bool bIsLast) override;

        /// 请求查询期权自对冲响应
        void OnRspQryOptionSelfClose(CThostFtdcOptionSelfCloseField *pOptionSelfClose, CThostFtdcRspInfoField *pRspInfo,
                                     int nRequestID, bool bIsLast) override;

        /// 请求查询投资单元响应
        void
        OnRspQryInvestUnit(CThostFtdcInvestUnitField *pInvestUnit, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                           bool bIsLast) override;

        void OnRspQrySPBMFutureParameter(CThostFtdcSPBMFutureParameterField *pSPBMFutureParameter,
                                         CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        void OnRspQrySPBMOptionParameter(CThostFtdcSPBMOptionParameterField *pSPBMOptionParameter,
                                         CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        void
        OnRspQrySPBMIntraParameter(CThostFtdcSPBMIntraParameterField *pSPBMIntraParameter,
                                   CThostFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast) override;

        void
        OnRspQrySPBMInterParameter(CThostFtdcSPBMInterParameterField *pSPBMInterParameter,
                                   CThostFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast) override;

        void OnRspQrySPBMPortfDefinition(CThostFtdcSPBMPortfDefinitionField *pSPBMPortfDefinition,
                                         CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        void OnRspQrySPBMInvestorPortfDef(CThostFtdcSPBMInvestorPortfDefField *pSPBMInvestorPortfDef,
                                          CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        void OnRspQryInvestorPortfMarginRatio(CThostFtdcInvestorPortfMarginRatioField *pInvestorPortfMarginRatio,
                                              CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        void OnRspQryInvestorProdSPBMDetail(CThostFtdcInvestorProdSPBMDetailField *pInvestorProdSPBMDetail,
                                            CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        void
        OnRspQryInvestorCommoditySPMMMargin(CThostFtdcInvestorCommoditySPMMMarginField *pInvestorCommoditySPMMMargin,
                                            CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        void OnRspQryInvestorCommodityGroupSPMMMargin(
                CThostFtdcInvestorCommodityGroupSPMMMarginField *pInvestorCommodityGroupSPMMMargin,
                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        void OnRspQrySPMMInstParam(CThostFtdcSPMMInstParamField *pSPMMInstParam, CThostFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast) override;

        void
        OnRspQrySPMMProductParam(CThostFtdcSPMMProductParamField *pSPMMProductParam, CThostFtdcRspInfoField *pRspInfo,
                                 int nRequestID, bool bIsLast) override;

        void OnRspQrySPBMAddOnInterParameter(CThostFtdcSPBMAddOnInterParameterField *pSPBMAddOnInterParameter,
                                             CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        void OnRspQryRCAMSCombProductInfo(CThostFtdcRCAMSCombProductInfoField *pRCAMSCombProductInfo,
                                          CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        void OnRspQryRCAMSInstrParameter(CThostFtdcRCAMSInstrParameterField *pRCAMSInstrParameter,
                                         CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        void OnRspQryRCAMSIntraParameter(CThostFtdcRCAMSIntraParameterField *pRCAMSIntraParameter,
                                         CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        void OnRspQryRCAMSInterParameter(CThostFtdcRCAMSInterParameterField *pRCAMSInterParameter,
                                         CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        void OnRspQryRCAMSShortOptAdjustParam(CThostFtdcRCAMSShortOptAdjustParamField *pRCAMSShortOptAdjustParam,
                                              CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        void OnRspQryRCAMSInvestorCombPosition(CThostFtdcRCAMSInvestorCombPositionField *pRCAMSInvestorCombPosition,
                                               CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        void OnRspQryInvestorProdRCAMSMargin(CThostFtdcInvestorProdRCAMSMarginField *pInvestorProdRCAMSMargin,
                                             CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        void
        OnRspQryRULEInstrParameter(CThostFtdcRULEInstrParameterField *pRULEInstrParameter,
                                   CThostFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast) override;

        void
        OnRspQryRULEIntraParameter(CThostFtdcRULEIntraParameterField *pRULEIntraParameter,
                                   CThostFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast) override;

        void
        OnRspQryRULEInterParameter(CThostFtdcRULEInterParameterField *pRULEInterParameter,
                                   CThostFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast) override;

        void OnRspQryInvestorProdRULEMargin(CThostFtdcInvestorProdRULEMarginField *pInvestorProdRULEMargin,
                                            CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 请求查询组合合约安全系数响应
        void OnRspQryCombInstrumentGuard(CThostFtdcCombInstrumentGuardField *pCombInstrumentGuard,
                                         CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 请求查询申请组合响应
        void
        OnRspQryCombAction(CThostFtdcCombActionField *pCombAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                           bool bIsLast) override;

        /// 请求查询转帐流水响应
        void OnRspQryTransferSerial(CThostFtdcTransferSerialField *pTransferSerial, CThostFtdcRspInfoField *pRspInfo,
                                    int nRequestID, bool bIsLast) override;

        /// 请求查询银期签约关系响应
        void OnRspQryAccountregister(CThostFtdcAccountregisterField *pAccountregister, CThostFtdcRspInfoField *pRspInfo,
                                     int nRequestID, bool bIsLast) override;

        /// 错误应答
        void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 报单通知
        void OnRtnOrder(CThostFtdcOrderField *pOrder) override;

        /// 成交通知
        void OnRtnTrade(CThostFtdcTradeField *pTrade) override;

        /// 报单录入错误回报
        void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo) override;

        /// 报单操作错误回报
        void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo) override;

        /// 合约交易状态通知
        void OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus) override;

        /// 交易所公告通知
        void OnRtnBulletin(CThostFtdcBulletinField *pBulletin) override;

        /// 交易通知
        void OnRtnTradingNotice(CThostFtdcTradingNoticeInfoField *pTradingNoticeInfo) override;

        /// 提示条件单校验错误
        void OnRtnErrorConditionalOrder(CThostFtdcErrorConditionalOrderField *pErrorConditionalOrder) override;

        /// 执行宣告通知
        void OnRtnExecOrder(CThostFtdcExecOrderField *pExecOrder) override;

        /// 执行宣告录入错误回报
        void
        OnErrRtnExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder,
                                CThostFtdcRspInfoField *pRspInfo) override;

        /// 执行宣告操作错误回报
        void OnErrRtnExecOrderAction(CThostFtdcExecOrderActionField *pExecOrderAction,
                                     CThostFtdcRspInfoField *pRspInfo) override;

        /// 询价录入错误回报
        void
        OnErrRtnForQuoteInsert(CThostFtdcInputForQuoteField *pInputForQuote, CThostFtdcRspInfoField *pRspInfo) override;

        /// 报价通知
        void OnRtnQuote(CThostFtdcQuoteField *pQuote) override;

        /// 报价录入错误回报
        void OnErrRtnQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo) override;

        /// 报价操作错误回报
        void OnErrRtnQuoteAction(CThostFtdcQuoteActionField *pQuoteAction, CThostFtdcRspInfoField *pRspInfo) override;

        /// 询价通知
        void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp) override;

        /// 保证金监控中心用户令牌
        void OnRtnCFMMCTradingAccountToken(CThostFtdcCFMMCTradingAccountTokenField *pCFMMCTradingAccountToken) override;

        /// 批量报单操作错误回报
        void OnErrRtnBatchOrderAction(CThostFtdcBatchOrderActionField *pBatchOrderAction,
                                      CThostFtdcRspInfoField *pRspInfo) override;

        /// 期权自对冲通知
        void OnRtnOptionSelfClose(CThostFtdcOptionSelfCloseField *pOptionSelfClose) override;

        /// 期权自对冲录入错误回报
        void OnErrRtnOptionSelfCloseInsert(CThostFtdcInputOptionSelfCloseField *pInputOptionSelfClose,
                                           CThostFtdcRspInfoField *pRspInfo) override;

        /// 期权自对冲操作错误回报
        void OnErrRtnOptionSelfCloseAction(CThostFtdcOptionSelfCloseActionField *pOptionSelfCloseAction,
                                           CThostFtdcRspInfoField *pRspInfo) override;

        /// 申请组合通知
        void OnRtnCombAction(CThostFtdcCombActionField *pCombAction) override;

        /// 申请组合录入错误回报
        void OnErrRtnCombActionInsert(CThostFtdcInputCombActionField *pInputCombAction,
                                      CThostFtdcRspInfoField *pRspInfo) override;

        /// 请求查询签约银行响应
        void
        OnRspQryContractBank(CThostFtdcContractBankField *pContractBank, CThostFtdcRspInfoField *pRspInfo,
                             int nRequestID,
                             bool bIsLast) override;

        /// 请求查询预埋单响应
        void
        OnRspQryParkedOrder(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                            bool bIsLast) override;

        /// 请求查询预埋撤单响应
        void
        OnRspQryParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction,
                                  CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) override;

        /// 请求查询交易通知响应
        void OnRspQryTradingNotice(CThostFtdcTradingNoticeField *pTradingNotice, CThostFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast) override;

        /// 请求查询经纪公司交易参数响应
        void OnRspQryBrokerTradingParams(CThostFtdcBrokerTradingParamsField *pBrokerTradingParams,
                                         CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 请求查询经纪公司交易算法响应
        void
        OnRspQryBrokerTradingAlgos(CThostFtdcBrokerTradingAlgosField *pBrokerTradingAlgos,
                                   CThostFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast) override;

        /// 请求查询监控中心用户令牌
        void
        OnRspQueryCFMMCTradingAccountToken(CThostFtdcQueryCFMMCTradingAccountTokenField *pQueryCFMMCTradingAccountToken,
                                           CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 银行发起银行资金转期货通知
        void OnRtnFromBankToFutureByBank(CThostFtdcRspTransferField *pRspTransfer) override;

        /// 银行发起期货资金转银行通知
        void OnRtnFromFutureToBankByBank(CThostFtdcRspTransferField *pRspTransfer) override;

        /// 银行发起冲正银行转期货通知
        void OnRtnRepealFromBankToFutureByBank(CThostFtdcRspRepealField *pRspRepeal) override;

        /// 银行发起冲正期货转银行通知
        void OnRtnRepealFromFutureToBankByBank(CThostFtdcRspRepealField *pRspRepeal) override;

        /// 期货发起银行资金转期货通知
        void OnRtnFromBankToFutureByFuture(CThostFtdcRspTransferField *pRspTransfer) override;

        /// 期货发起期货资金转银行通知
        void OnRtnFromFutureToBankByFuture(CThostFtdcRspTransferField *pRspTransfer) override;

        /// 系统运行时期货端手工发起冲正银行转期货请求，银行处理完毕后报盘发回的通知
        void OnRtnRepealFromBankToFutureByFutureManual(CThostFtdcRspRepealField *pRspRepeal) override;

        /// 系统运行时期货端手工发起冲正期货转银行请求，银行处理完毕后报盘发回的通知
        void OnRtnRepealFromFutureToBankByFutureManual(CThostFtdcRspRepealField *pRspRepeal) override;

        /// 期货发起查询银行余额通知
        void OnRtnQueryBankBalanceByFuture(CThostFtdcNotifyQueryAccountField *pNotifyQueryAccount) override;

        /// 期货发起银行资金转期货错误回报
        void
        OnErrRtnBankToFutureByFuture(CThostFtdcReqTransferField *pReqTransfer,
                                     CThostFtdcRspInfoField *pRspInfo) override;

        /// 期货发起期货资金转银行错误回报
        void
        OnErrRtnFutureToBankByFuture(CThostFtdcReqTransferField *pReqTransfer,
                                     CThostFtdcRspInfoField *pRspInfo) override;

        /// 系统运行时期货端手工发起冲正银行转期货错误回报
        void OnErrRtnRepealBankToFutureByFutureManual(CThostFtdcReqRepealField *pReqRepeal,
                                                      CThostFtdcRspInfoField *pRspInfo) override;

        /// 系统运行时期货端手工发起冲正期货转银行错误回报
        void OnErrRtnRepealFutureToBankByFutureManual(CThostFtdcReqRepealField *pReqRepeal,
                                                      CThostFtdcRspInfoField *pRspInfo) override;

        /// 期货发起查询银行余额错误回报
        void OnErrRtnQueryBankBalanceByFuture(CThostFtdcReqQueryAccountField *pReqQueryAccount,
                                              CThostFtdcRspInfoField *pRspInfo) override;

        /// 期货发起冲正银行转期货请求，银行处理完毕后报盘发回的通知
        void OnRtnRepealFromBankToFutureByFuture(CThostFtdcRspRepealField *pRspRepeal) override;

        /// 期货发起冲正期货转银行请求，银行处理完毕后报盘发回的通知
        void OnRtnRepealFromFutureToBankByFuture(CThostFtdcRspRepealField *pRspRepeal) override;

        /// 期货发起银行资金转期货应答
        void OnRspFromBankToFutureByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo,
                                           int nRequestID, bool bIsLast) override;

        /// 期货发起期货资金转银行应答
        void OnRspFromFutureToBankByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo,
                                           int nRequestID, bool bIsLast) override;

        /// 期货发起查询银行余额应答
        void OnRspQueryBankAccountMoneyByFuture(CThostFtdcReqQueryAccountField *pReqQueryAccount,
                                                CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                                bool bIsLast) override;

        /// 银行发起银期开户通知
        void OnRtnOpenAccountByBank(CThostFtdcOpenAccountField *pOpenAccount) override;

        /// 银行发起银期销户通知
        void OnRtnCancelAccountByBank(CThostFtdcCancelAccountField *pCancelAccount) override;

        /// 银行发起变更银行账号通知
        void OnRtnChangeAccountByBank(CThostFtdcChangeAccountField *pChangeAccount) override;

        /// 请求查询分类合约响应
        void OnRspQryClassifiedInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo,
                                          int nRequestID, bool bIsLast) override;

        /// 请求组合优惠比例响应
        void
        OnRspQryCombPromotionParam(CThostFtdcCombPromotionParamField *pCombPromotionParam,
                                   CThostFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast) override;

        /// 投资者风险结算持仓查询响应
        void OnRspQryRiskSettleInvstPosition(CThostFtdcRiskSettleInvstPositionField *pRiskSettleInvstPosition,
                                             CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

        /// 风险结算产品查询响应
        void OnRspQryRiskSettleProductStatus(CThostFtdcRiskSettleProductStatusField *pRiskSettleProductStatus,
                                             CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    };
    
    struct TxUnit {
        std::string contract;
        int position;
        double price;
        double last_price;
        bool open_signal;
        bool close_signal;
        long long check_timestamp;
    };

    class CtpTx {
    private:
        CtpTxHandler tx_handler_;

    public:
        explicit CtpTx(CtpConfigPtr config);

        ~CtpTx();

    public:
        double Order(const std::string &contract, TxDirection direction, int amt);
    };
}

#endif // LUEING_CTP_TX_H