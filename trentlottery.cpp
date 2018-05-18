// #include <fc/crypto/rand.hpp>
#include "trentlottery.hpp"

using namespace std;
using eosio::action;
using eosio::permission_level;

//@abi action
void trentlottery::playerbet(uint64_t draw, account_name player, const uint32_t buycnt, std::vector<uint16_t> bills)
{
    require_auth(player);
    asset bet = ticketprice * buycnt;

    eosio_assert(games.begin() == games.end(), "game not started");
    auto lastgame_itr = games.rbegin();
    eosio_assert(lastgame_itr->status == BETTING, "game is not betting");
    eosio_assert(lastgame_itr->draw == draw, "must match game draw");

    eosio_assert(buycnt*7 == bills.size(), "maybe lost lottery tickets");

    action act(
        permission_level{player, N(active)},
        N(eosio.token), N(transfer),
        std::make_tuple(player, _self, bet, std::string("")));
    act.send();

    offerbets.emplace(_self, [&](auto &offer) {
        offer.id = offerbets.available_primary_key();
        offer.player = player;
        offer.draw = draw;
        offer.bet = bet;
        offer.buycnt = buycnt;
        offer.buylottos = bills;
        offer.buytime = now();
    });
}

/**
 * 创建一轮新游戏
 */
void trentlottery::creategame()
{
    uint64_t draw = 0;
    vector<uint16_t> initHitnum(7, 0);
    
    eosio_assert(games.begin() == games.end(), "game not started");

    auto lastgame_itr = games.rbegin();
    draw = lastgame_itr->draw;
    draw += 1;

    games.emplace(_self, [&](auto &game) {
        game.draw = draw;
        game.status = LOCKING;
        game.hitnum = initHitnum;
    });
}

void trentlottery::startgame()
{
    vector<uint16_t> initHitnum(7, 0);
    eosio_assert(games.begin() != games.end(), "already have started games!");
    require_auth(_self);

    games.emplace(_self, [&](auto &game) {
        game.draw = 1;
        game.status = LOCKING;
        game.hitnum = initHitnum;
    });
}

void trentlottery::setprice(const asset &price)
{
    eosio_assert(price.symbol == S(4, EOS), "only EOS token allowed");
    eosio_assert(price.is_valid(), "invalid bet");
    eosio_assert(price.amount > 0, "must positive bet amount");
    require_auth(_self);

    ticketprice = price;
}

void trentlottery::getprice()
{
    print("Ticket price is ", ticketprice.amount, " EOS");
}

void trentlottery::enablegame()
{
    require_auth(_self);
    auto lastgame_itr = games.rbegin();
    eosio_assert(games.begin() != games.end(), "already have started games!");
    eosio_assert(lastgame_itr->status == LOCKING, "game is not locking");

    auto mgame_itr = games.find(lastgame_itr->draw);
    games.modify(mgame_itr, 0, [&](auto &game) {
        game.status = BETTING;
    });
}

void trentlottery::jackpot()
{
    // asset balance = get_currency_balance(N(eosio.token), S(4, EOS), _self);
    // print("trentlottery balance: ", balance.amount);
}

std::vector<std::vector<uint16_t>> trentlottery::parseofferbet(uint32_t cnt, std::vector<uint16_t> tickets)
{
    eosio_assert(cnt * 7 == tickets.size(), "maybe lost lottery tickets");

    vector<vector<uint16_t>> betbills;

    size_t seg = 0;
    vector<uint16_t> subbill;
    for (auto iter = tickets.cbegin(); iter != tickets.cend(); iter++) 
    {
        subbill.push_back(*iter);
        seg++;

        if (seg % 7 == 0)
        {
            betbills.push_back(subbill);
            subbill.clear();
        }
    }
    return betbills;
}

EOSIO_ABI(trentlottery, (playerbet)(startgame)(enablegame)(setprice)(getprice))
