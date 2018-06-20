#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <string>

using eosio::action;
using eosio::asset;
using eosio::const_mem_fun;
using eosio::indexed_by;
using eosio::key256;
using eosio::name;
using eosio::permission_level;
using eosio::print;

class trentlottery : public eosio::contract
{
  private:
    typedef enum game_status
    {
        LOCKING = 0,        // 锁定状态，不允许下注
        BETTING,            // 正在进行
        REVEAL,             // 正在开奖
        OVER                // 已开奖，本轮游戏已结束
    } game_status;

    //@abi table globalgame i64
    struct globalgame
    {
        uint64_t id = 0;                        // 主键，使用available_primary_key生成
        bool maintained = false;                // 是否处于系统维护状态

        uint64_t primary_key()const { return id; }

        EOSLIB_SERIALIZE( globalgame, (id)(maintained) )
    };

    //@abi table ticketprice i64
    struct ticketprice
    {
        uint64_t id = 0; // 主键，使用available_primary_key生成
        asset price;

        uint64_t primary_key() const { return id; }

        EOSLIB_SERIALIZE(ticketprice, (id)(price))
    };

    //@abi table bonusgrade i64
    struct bonusgrade
    {
        uint64_t id = 0; // 主键，使用available_primary_key生成

        double firstrate;
        double secondrate;

        asset maxfirst;
        asset maxsecond;
        asset third;
        asset fourth;
        asset fifth;
        asset sixth;

        uint64_t primary_key() const { return id; }

        EOSLIB_SERIALIZE(bonusgrade, (id)(firstrate)(secondrate)(maxfirst)(maxsecond)(third)(fourth)(fifth)(sixth))
    };

    //@abi table games i64
    struct game
    {
        uint64_t draw;                // 双色球期数
        uint16_t status;              // 此轮双色球游戏的状态，未开奖、正在开奖、已开奖
        std::vector<uint16_t> hitnum; // 当期开奖号码，红球、蓝球都放在一个数组中，前面6个表示红球，最后一个表示蓝球

        uint64_t primary_key() const { return draw; }

        EOSLIB_SERIALIZE(game, (draw)(status)(hitnum))
    };

    //@abi table offerbets i64
    struct offerbet
    {
        uint64_t id;                     // 主键，使用available_primary_key生成
        account_name player;             // 下注的用户帐号
        uint64_t draw;                   // 双色球期数
        asset bet;                       // 下注金额
        uint32_t buycnt;                 // 购买的彩票数量，单位为张
        std::vector<uint16_t> buylottos; // 记录每次下单购买的彩票数字组合，每7个数字表示一张彩票，所有彩票连续记录在此字段
        time buytime;                    // 购买的时间

        uint64_t primary_key() const { return id; }
        account_name by_player() const { return player; }
        uint64_t by_draw() const { return draw; }

        EOSLIB_SERIALIZE(offerbet, (id)(player)(draw)(bet)(buycnt)(buylottos)(buytime))
    };

    //@abi table winnings i64
    struct winning
    {
        uint64_t id;                    // 主键，使用available_primary_key生成
        account_name winner;            // 中奖账户
        uint64_t draw;                  // 中奖期数
        asset currency;                 // 中奖金额
        std::vector<uint16_t> offernum; // 中奖账户购买的彩票号码
        uint16_t prize;                 // 表示账户中的是几等奖

        uint64_t primary_key() const { return id; }
        uint64_t by_draw() const { return draw; }

        EOSLIB_SERIALIZE(winning, (id)(winner)(draw)(currency)(offernum)(prize))
    };

  private:
    typedef eosio::multi_index<N(globalgame), globalgame> globalgame_index;

    typedef eosio::multi_index<N(ticketprice), ticketprice> ticketprice_index;

    typedef eosio::multi_index<N(bonusgrade), bonusgrade> bonusgrade_index;

    typedef eosio::multi_index<N(games), game> game_index;

    typedef eosio::multi_index<N(offerbets), offerbet,
                               indexed_by<N(draw), const_mem_fun<offerbet, uint64_t, &offerbet::by_draw>>,
                               indexed_by<N(player), const_mem_fun<offerbet, account_name, &offerbet::by_player>>>
        offer_bets_index;

    typedef eosio::multi_index<N(winnings), winning,
                               indexed_by<N(draw), const_mem_fun<winning, uint64_t, &winning::by_draw>>>
        winning_record_index;

    globalgame_index globalgames;
    game_index games;
    offer_bets_index offerbets;
    winning_record_index winnings;

    ticketprice_index ticket_price;
    bonusgrade_index gamebonusgrade;

  public:
    trentlottery(account_name self)
        : eosio::contract(self),
          globalgames(_self, _self),
          games(_self, _self),
          offerbets(_self, _self),
          winnings(_self, _self),
          ticket_price(_self, _self),
          gamebonusgrade(_self, _self)
    {
    }

    void playerbet(uint64_t draw, const name player, const uint32_t buycnt, std::vector<uint16_t> bills);
    void startgame();
    void enablegame();
    void jackpot();
    void setprice(const asset& price);
    void getprice();
    void setmaintain(bool maintenance);
    void getmaintain();
    void setbonus(const double firstrate, const double secondrate, const asset &maxfirst, const asset &maxsecond, const asset &third, const asset &fourth, const asset &fifth, const asset &sixth);
    void lockgame();
    void drawlottery();

  private:
    bool isInMaintain();
    void creategame();
    std::vector<std::vector<uint16_t>> parseofferbet(uint32_t cnt, std::vector<uint16_t> tickets, std::vector<std::vector<uint16_t>> &betbills);
    bool isTicketValid(std::vector<uint16_t> ticket);
    uint16_t judgeprice(std::vector<uint16_t> hitnum, std::vector<uint16_t> offernum);
    asset contractbalance();
   
    void drawhighlottery(uint64_t draw, std::vector<winning> firstwinnings, std::vector<winning> secondwinnings);
    void sendbonus(asset bonus, account_name player,uint64_t draw,uint16_t prize,std::vector<uint16_t> offernum);
    std::vector<uint16_t> generatehitnum();
    asset getticketprice();
    void setlastgamestatus(game_status status);
};
