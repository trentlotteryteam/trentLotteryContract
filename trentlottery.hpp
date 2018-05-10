#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <string>

namespace eosio {
    class trentlottery : public eosio::contract {
        private:
            //@abi table game i64
            struct game {
                uint64_t                period;         // 双色球期数
                asset                   jackpot;        // 资金池总量
                uint16_t                status;         // 此轮双色球游戏的状态，未开奖、正在开奖、已开奖
                vector<uint16_t>        winning_number; // 当期开奖号码，红球、蓝球都放在一个数组中，前面6个表示红球，最后一个表示蓝球

                uint64_t        primary_key()const { return period; }

                EOSLIB_SERIALIZE( game, (period)(jackpot)(status)(winning_number) )
            };

            //@abi table offer_bet i64
            struct offer_bet {
                uint64_t                    id;             // 主键，使用available_primary_key生成
                account_name                player;         // 下注的用户帐号
                uint64_t                    periods;        // The periods of bets
                asset                       bet;            // 下注金额
                uint32_t                    buyCnt;         // 购买的彩票数量，单位为张
                vector<uint16_t>            buyLottos;      // 记录每次下单购买的彩票数字组合，每7个数字表示一张彩票，所有彩票连续记录在此字段
                time                        buyTime;        // 购买的时间

                uint64_t        primary_key()const { return id; }
                account_name    by_player()const { return player; }
                uint64_t        by_period() const { return periods; }

                EOSLIB_SERIALIZE( offer_bet, (id)(player)(periods)(bet)(buyLottos)(buyTime) )
            };
            
            //@abi table winning_record i64
            struct winning_record {
                uint64_t                    id;             // 主键，使用available_primary_key生成
                account_name                winner;         // 中奖账户
                uint64_t                    period;         // 中奖期数
                asset                       win_currency;   // 中奖金额
                vector<uint16_t>            offer_number;   // 中奖账户购买的彩票号码
                uint16_t                    prize;          // 表示账户中的是几等奖

                uint64_t        primary_key()const { return id; }
                uint64_t        by_period() const { return period; }

                EOSLIB_SERIALIZE( winning_record, (id)(winner)(period)(win_currency)(offer_number)(prize) )
            };

        private:

            typedef eosio::multi_index<N(game), game> game_index;

            typedef eosio::multi_index<N(offer_bets), offer_bet,
                indexed_by<N(periods), const_mem_fun<offer_bet, uint64_t, &offer_bet::by_period > >,
                indexed_by<N(player), const_mem_fun<offer_bet, account_name, &offer_bet::by_player> >
            > offer_bets_index;

            typedef eosio::multi_index<N(winning_rec), winning_record,
                indexed_by<N(period), const_mem_fun<winning_record, uint64_t, &winning_record::by_period>>
            > winning_record_index;

            game_index              games;
            offer_bets_index        offer_bets;
            winning_record_index    winning_recs;

        public:
            trentlottery(account_name self)
                :eosio::contract(self),
                games(_self, _self),
                offer_bets(_self, _self),
                winning_recs(_self, _self)
                {}

            void playerbet(uint64_t period, account_name player, const asset& bet, const uint32_t buyCnt, vector<uint16_t> &bills);
    };
}
