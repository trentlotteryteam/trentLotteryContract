#include "trentlottery.hpp"

using namespace eosio;

void trentlottery::player_bet(uint64_t period, account_name player, const asset& bet, vector<vector<uint16_t>> &bills) {
    eosio_assert( bet.symbol == S(4,EOS) , "only EOS token allowed" );
    eosio_assert( bet.amount > 0, "must bet positive quantity" );
    require_auth(player);

    offer_bets.emplace(_self, [&](auto& offer_bet) {
        offer_bet.id        = offer_bets.available_primary_key();
        offer_bet.player    = player;
        offer_bet.periods   = period;
        offer_bet.bet       = bet;
        offer_bet.buyLottos = bills;
        offer_bet.buyTime   = now();
    });
}

EOSIO_ABI(trentlottery, (player_bet))
