#pragma once
#include "cryptonote_protocol/cryptonote_protocol_handler_common.h"

namespace cryptonote
{
	class core;
};

namespace service_nodes {

#define TRADE_OGRE_API "https://tradeogre.com/api/v1"
#define BITLIBER_API "https://bitliber.com/api/v1"
#define COINBASE_PRO "https://api.pro.coinbase.com"
#define GEMINI_API "https://api.gemini.com/v1"
#define TRITON_EX "http://exchange.equilibria.network/api"
#define BITFINEX_API "https://api.bitfinex.com/v1"
#define NANCE_API "https://api.binance.com/api/v1"
#define STAMP_API "https://www.bitstamp.net/api"

struct exchange_trade {
  uint64_t date;
  std::string type;
  double price;
  double quantity;
};

struct exchange_order {
  double price;
  double quantity;
};

struct adjusted_liquidity {
  double liquid;
  double price;
};

//Ribbon Protocol
class ribbon_protocol {
  public:
    explicit ribbon_protocol(cryptonote::core& core);
    double get_btc_b();
    uint64_t convert_btc_to_usd(double btc);
    std::vector<exchange_trade> trades_during_latest_1_block();
  private:
  	cryptonote::core& m_core;
};


//Trade API functions
//--XTRI--
bool get_trades_from_ogre(std::vector<exchange_trade> *trades);
bool get_orders_from_ogre(std::vector<exchange_order> *orders);

std::vector<exchange_trade> get_recent_trades();
std::vector<adjusted_liquidity> get_recent_liquids(double blue);


//--BITCOIN USD--
uint64_t get_bittrex_price(std::pair<std::string, std::string> pair);
std::pair<double, double> get_nance_btc_usd();

uint64_t create_bitcoin_a();

//Price Functions
double get_usd_average();
uint64_t convert_btc_to_usd(double btc);
uint64_t create_ribbon_green(std::vector<exchange_trade> trades);
uint64_t create_ribbon_blue(std::vector<exchange_trade> trades);
double filter_trades_by_deviation(std::vector<exchange_trade> trades);
double trades_weighted_mean(std::vector<exchange_trade> trades);

//Liquid Functions
uint64_t get_volume_for_block(std::vector<exchange_trade> trades);
std::vector<adjusted_liquidity> create_adjusted_liqudities(std::vector<exchange_order> orders, double spot);
double create_mac(std::vector<adjusted_liquidity> adj_liquids);

}
