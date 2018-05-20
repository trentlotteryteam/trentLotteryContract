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

    auto tickets = parseofferbet(buycnt, bills);
    for (uint32_t i = 0; i < buycnt; i++)
    {
        eosio_assert(isTicketValid(tickets.at(i)), "lottery tickets invalid");
    }

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

bool trentlottery::isTicketValid(std::vector<uint16_t> ticket)
{
    if (ticket.size() != 7) {return false;}

    uint16_t blueball = ticket.at(6);
    if (blueball < 1 || blueball > 16) {return false;}

    for (uint16_t i = 0; i < ticket.size()-1; i++)
    {
        if (ticket.at(i) < 1 || ticket.at(i) > 33)
        {
            return false;
        }
    }

    for (uint16_t i = 0; i < ticket.size()-2; i++)
    {
        uint16_t redball = ticket.at(i);
        for (uint16_t j = i+1; j < ticket.size()-1; j++)
        {
            if (redball == ticket.at(j)) {return false;}
        }
    }

    return true;
}

uint16_t trentlottery::judgeprice(std::vector<uint16_t> hitnum, std::vector<uint16_t> offernum)
{
    uint16_t price = 0;
    bool hitblue = false;
    uint16_t hitrednum = 0;

    uint16_t blueball = hitnum.at(6);
    if (blueball == offernum.at(6))
    {
        hitblue = true;
    }

    for (uint16_t i = 0; i < hitnum.size()-1; i++)
    {
        for (uint16_t j = 0; j < offernum.size()-1; j++)
        {
            if (hitnum.at(i) == offernum.at(j))
            {
                hitrednum++;
                break;
            }
        }
    }

    if (hitblue && hitrednum == 6)
    {
        return 1;
    }
    if (!hitblue && hitrednum == 6)
    {
        return 2;
    }
    if (hitblue && hitrednum == 5)
    {
        return 3;
    }
    if (((!hitblue && hitrednum == 5) || (hitblue && hitrednum == 4))
    {
        return 4;
    }
    if (((!hitblue && hitrednum == 4) || (hitblue && hitrednum == 3))
    {
        return 5;
    }
    if ((hitblue && hitrednum == 0) || (hitblue && hitrednum == 1) || (hitblue && hitrednum == 2))
    {
        return 6;
    }

    return price;
}

EOSIO_ABI(trentlottery, (playerbet)(startgame)(enablegame)(setprice)(getprice))
