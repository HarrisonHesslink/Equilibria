#include <vector>
#define CURL_STATICLIB
#include <curl/curl.h>
#include <iostream>
#include <math.h>

#include "int-util.h"
#include "rapidjson/document.h"
#include "cryptonote_core.h"
#include "ribbon.h"

namespace service_nodes {


size_t curl_write_callback(char *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// TODO: use http_client from net tools
std::string make_curl_http_get(std::string url)
{
  std::string read_buffer;
  CURL* curl = curl_easy_init(); 
  curl_global_init(CURL_GLOBAL_ALL); 
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); //Fix this before launching...Should verify tradeogre
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); 
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
  CURLcode res = curl_easy_perform(curl); 
  curl_easy_cleanup(curl);
  return read_buffer;
}

ribbon_protocol::ribbon_protocol(cryptonote::core& core) : m_core(core){};

std::vector<exchange_trade> ribbon_protocol::trades_during_latest_1_block()
{
  std::vector<exchange_trade> trades = get_recent_trades();
  uint64_t top_block_height = m_core.get_current_blockchain_height() - 2;
  crypto::hash top_block_hash = m_core.get_block_id_by_height(top_block_height);
  cryptonote::block top_blk;
  m_core.get_block_by_hash(top_block_hash, top_blk);
  uint64_t top_block_timestamp = top_blk.timestamp;

  std::vector<exchange_trade> result;
  for (size_t i = 0; i < trades.size(); i++)
  {
    if (trades[i].date >= top_block_timestamp){
      result.push_back(trades[i]);
    }
  }
  return result;
}

bool get_trades_from_ogre(std::vector<exchange_trade> *trades)
{
  std::string data = make_curl_http_get(std::string(TRADE_OGRE_API) + std::string("/history/BTC-XEQ"));

  rapidjson::Document document;
  document.Parse(data.c_str());
  for (size_t i = 0; i < document.Size(); i++)
  {
    exchange_trade trade;
    trade.date = document[i]["date"].GetUint64();
    trade.type = document[i]["type"].GetString();
    trade.price = std::stod(document[i]["price"].GetString()); // trade ogre gives this info as a string
    trade.quantity = std::stod(document[i]["quantity"].GetString());
    trades->push_back(trade);
  }
  
  return true;
}

bool get_trades_from_tritonex(std::vector<exchange_trade> *trades)
{
  std::string data = make_curl_http_get(std::string(TRITON_EX) + std::string("/get_trades"));
    
  rapidjson::Document document;
  document.Parse(data.c_str());
  for (size_t i = 0; i < document.Size(); i++)
  {
    exchange_trade trade;
    trade.date = std::stoull(document[i]["TimeStamp"].GetString());
    trade.type = document[i]["TradeType"].GetString();
    trade.price = std::stod(document[i]["Price"].GetString()); // tritonex gives this info as a string
    trade.quantity = std::stod(document[i]["Amount"].GetString());
    trades->push_back(trade);
  }
  
  return true;
}

bool get_orders_from_ogre(std::vector<exchange_order> *orders)
{
  std::string data = make_curl_http_get(std::string(TRADE_OGRE_API) + std::string("/orders/BTC-XEQ"));
    
  rapidjson::Document document;
  document.Parse(data.c_str());

  if(document.HasMember("buy")){
    const rapidjson::Value& buyJson = document["buy"];
    size_t get_top_25_orders = buyJson.Size() - 25;
    for (rapidjson::Value::ConstMemberIterator iter = buyJson.MemberBegin() + get_top_25_orders; iter != buyJson.MemberEnd(); ++iter)
    {
      exchange_order order;
      order.price = std::stod(iter->name.GetString());
      order.quantity = std::stod(iter->value.GetString());
      orders->push_back(order);
    }
  }  

  return true;
}

std::pair<double, double> get_coinbase_pro_btc_usd()
{
  std::string data = make_curl_http_get(std::string(COINBASE_PRO) + std::string("/products/BTC-USD/ticker"));
  rapidjson::Document document;
  document.Parse(data.c_str());
  double btc_usd = 0;
  double vol_usd = 0;
  for (size_t i = 0; i < document.Size(); i++)
  {  
    btc_usd = std::stod(document["price"].GetString());
    vol_usd = std::stod(document["volume"].GetString());
  }
  return {btc_usd, vol_usd};
}

std::pair<double, double> get_gemini_btc_usd()
{
  std::string data = make_curl_http_get(std::string(GEMINI_API) + std::string("/pubticker/btcusd"));
  rapidjson::Document document;
  document.Parse(data.c_str());
  double btc_usd = 0;
  double vol_usd = 0;
  for (size_t i = 0; i < document.Size(); i++)
  {
    btc_usd = std::stod(document["last"].GetString());;
    vol_usd = std::stod(document["volume"]["USD"].GetString());;
  }
  return {btc_usd, vol_usd};
}

std::pair<double, double> get_bitfinex_btc_usd()
{
  std::string data = make_curl_http_get(std::string(BITFINEX_API) + std::string("/pubticker/btcusd"));
  rapidjson::Document document;
  document.Parse(data.c_str());
  double btc_usd = 0;
  double vol_usd = 0;
  for (size_t i = 0; i < document.Size(); i++)
  {
    btc_usd = std::stod(document["last_price"].GetString());
    vol_usd = std::stod(document["volume"].GetString());
  }
  return {btc_usd, vol_usd};
}

std::pair<double, double> get_nance_btc_usd()
{
  std::string data = make_curl_http_get(std::string(NANCE_API) + std::string("/ticker/24hr?symbol=BTCUSDT"));
  rapidjson::Document document;
  document.Parse(data.c_str());
  double btc_usd = 0;
  double vol_usd = 0;
  for (size_t i = 0; i < document.Size(); i++)
  {
    btc_usd = std::stod(document["lastPrice"].GetString());
    vol_usd = std::stod(document["quoteVolume"].GetString());
  }
  return {btc_usd, vol_usd};
}

std::pair<double, double> get_stamp_btc_usd()
{
  std::string data = make_curl_http_get(std::string(STAMP_API) + std::string("/ticker/"));
  rapidjson::Document document;
  document.Parse(data.c_str());
  double btc_usd = 0;
  double vol_usd = 0;
  for (size_t i = 0; i < document.Size(); i++)
  {
    btc_usd = std::stod(document["last"].GetString());
    vol_usd = std::stod(document["volume"].GetString());
  }
  return {btc_usd, vol_usd};
}

uint64_t create_bitcoin_a(){
  std::pair<double, double> gemini_usd = get_gemini_btc_usd();
  std::pair<double, double> coinbase_pro_usd = get_coinbase_pro_btc_usd();
  std::pair<double, double> bitfinex_usd = get_bitfinex_btc_usd();
  std::pair<double, double> nance_usd = get_nance_btc_usd();
  std::pair<double, double> stamp_usd = get_stamp_btc_usd();

  double t_vol = gemini_usd.second + coinbase_pro_usd.second + bitfinex_usd.second + nance_usd.second + stamp_usd.second;
  double weighted_values = (gemini_usd.first * gemini_usd.second) + (coinbase_pro_usd.first * coinbase_pro_usd.second) + (bitfinex_usd.first * bitfinex_usd.second) + (nance_usd.first * nance_usd.second) + (stamp_usd.first * stamp_usd.second);
  
  return static_cast<uint64_t>(weighted_values / t_vol);
}

double get_usd_average(){
  std::pair<double, double> gemini_usd = get_gemini_btc_usd();
  std::pair<double, double> coinbase_pro_usd = get_coinbase_pro_btc_usd();
  std::pair<double, double> bitfinex_usd = get_bitfinex_btc_usd();
  std::pair<double, double> nance_usd = get_nance_btc_usd();
  std::pair<double, double> stamp_usd = get_stamp_btc_usd();

  //Sometimes coinbase pro returns 0? Need to look into this.
  if(coinbase_pro_usd.first == 0)
    return (gemini_usd.first + bitfinex_usd.first + nance_usd.first + stamp_usd.first) / 4;

  return (gemini_usd.first + coinbase_pro_usd.first + bitfinex_usd.first + nance_usd.first + stamp_usd.first) / 5;
}

double ribbon_protocol::get_btc_b(){
  
  crypto::hash block_hash = m_core.get_block_id_by_height(m_core.get_current_blockchain_height() - 1);
  cryptonote::block blk;
  m_core.get_block_by_hash(block_hash, blk);
  return static_cast<double>(blk.btc_b);
}

uint64_t ribbon_protocol::convert_btc_to_usd(double btc)
{
  double usd_average = get_btc_b();

  if(usd_average == 0)
    usd_average = get_usd_average();
  
	double usd = usd_average * btc;
	return static_cast<uint64_t>(usd * 100000); // remove "cents" decimal place and convert to integer
}

uint64_t convert_btc_to_usd(double btc)
{
	double usd = get_usd_average() * btc;
	return static_cast<uint64_t>(usd * 100000); // remove "cents" decimal place and convert to integer
}

uint64_t create_ribbon_blue(std::vector<exchange_trade> trades)
{
  double filtered_mean = filter_trades_by_deviation(trades);
  return convert_btc_to_usd(filtered_mean);
}

//Volume Weighted Average
uint64_t create_ribbon_green(std::vector<exchange_trade> trades){
  double weighted_mean = trades_weighted_mean(trades);
  return convert_btc_to_usd(weighted_mean);
}

uint64_t get_volume_for_block(std::vector<exchange_trade> trades){
  double volume = 0;
  if(trades.size() == 0)
    return 0;

  for(size_t i = 0; i < trades.size();i++){
    volume += (trades[i].price * trades[i].quantity);
  }
  return convert_btc_to_usd(volume);
}


//Volume Weighted Average with 2 STDEV trades removed
double filter_trades_by_deviation(std::vector<exchange_trade> trades)
{
  double weighted_mean = trades_weighted_mean(trades);
  int n = trades.size();
  double sum = 0;
  
  for (size_t i = 0; i < trades.size(); i++)
  {
    sum += pow((trades[i].price - weighted_mean), 2.0);
  }
  
  double deviation = sqrt(sum / (double)n);
  
  double max = weighted_mean + (2 * deviation);
  double min = weighted_mean - (2 * deviation);
  
  for (size_t i = 0; i < trades.size(); i++)
  {
    if (trades[i].price > max || trades[i].price < min)
      trades.erase(trades.begin() + i);
  }

  return trades_weighted_mean(trades);
}

double trades_weighted_mean(std::vector<exchange_trade> trades)
{
  double XEQ_volume_sum = 0;
  double weighted_sum = 0;
  for (size_t i = 0; i < trades.size(); i++)
  {
    XEQ_volume_sum += trades[i].quantity;
    weighted_sum += (trades[i].price * trades[i].quantity);
  }
  
  return weighted_sum / XEQ_volume_sum;
}

std::vector<exchange_trade> get_recent_trades()
{
  std::vector<service_nodes::exchange_trade> trades;
  if(!service_nodes::get_trades_from_ogre(&trades))
    MERROR("Error getting trades from Ogre");

  if(!service_nodes::get_trades_from_tritonex(&trades))
    MERROR("Error getting trades from TritonEX");

  return trades;
}


std::vector<adjusted_liquidity> get_recent_liquids(double blue)
{
  std::vector<exchange_order> orders;
  if(!get_orders_from_ogre(&orders))
    MERROR("Error getting orders from TradeOgre");
  //more exchanges below
  std::vector<adjusted_liquidity> adj_liquid = create_adjusted_liqudities(orders, blue);
  return adj_liquid;
}


std::vector<adjusted_liquidity> create_adjusted_liqudities(std::vector<exchange_order> orders, double spot){
  std::vector<adjusted_liquidity> al;

  for(size_t i = 0; i < orders.size();i++){
      adjusted_liquidity this_al;
      if(orders[i].price != spot){
        this_al.price = orders[i].price;
        double denom = (1 - std::abs(this_al.price - spot));
        this_al.liquid = (orders[i].quantity) * (1 / denom);
        al.push_back(this_al);
      }
  }

  return al;
}

double create_mac(std::vector<adjusted_liquidity> adj_liquids){
  double adj_liquid_sum = 0;
  double price_weighted = 0;

  for(size_t i = 0; i < adj_liquids.size(); i++){
    adj_liquid_sum += adj_liquids[i].liquid;
    price_weighted += (adj_liquids[i].liquid * adj_liquids[i].price);
  }
  return price_weighted * adj_liquid_sum;
}

}
