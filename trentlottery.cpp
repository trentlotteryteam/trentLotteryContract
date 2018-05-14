#include "trentlottery.hpp"

using namespace std;

//@abi action
void trentlottery::playerbet(uint64_t draw, account_name player, const asset& bet, const uint32_t buycnt, std::vector<uint16_t> bills) {
    eosio_assert( bet.symbol == S(4,EOS) , "only EOS token allowed" );
    eosio_assert( bet.is_valid(), "invalid bet" );
    eosio_assert( bet.amount > 0, "must bet positive quantity" );
    require_auth(player);

    // action(
    //     permission_level{player, N(active)},
    //     N(eosio.token), N(transfer),
    //     std::make_tuple(player, _self, bet, std::string(""))
    // ).send();

    auto bets_itr = offerbets.emplace(_self, [&](auto &offer) {
        offer.id = offerbets.available_primary_key();
        offer.player = player;
        offer.draw = draw;
        offer.bet = bet;
        offer.buycnt = buycnt;
        offer.buylottos = bills;
        offer.buytime = now();
    });
    eosio::print("end write", bets_itr->id);
}

void trentlottery::creategame(uint64_t draw, const asset &jackpot, uint16_t status, std::vector<uint16_t> hitnum) {
    games.emplace(_self, [&](auto &game) {
        game.draw       = draw;
        game.jackpot    = jackpot;
        game.status     = status;
        game.hitnum     = hitnum;
    });
}


EOSIO_ABI(trentlottery, (playerbet)(creategame))
