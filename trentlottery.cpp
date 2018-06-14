// #include <fc/crypto/rand.hpp>
#include "trentlottery.hpp"
#include <eosio.token/eosio.token.hpp>

using namespace std;
using eosio::action;
using eosio::permission_level;

//@abi action
void trentlottery::playerbet(uint64_t draw, const name player, const uint32_t buycnt, std::vector<uint16_t> bills)
{
    require_auth(player);
    auto ticketprice = getticketprice();
    asset bet = ticketprice * buycnt;

    eosio_assert(isInMaintain() == false, "The game is under maintenance");
    eosio_assert(games.begin() != games.end(), "game not started");
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
    eosio_assert(games.begin() == games.end(), "already have started games!");
    require_auth(_self);

    globalgames.emplace(_self, [&](auto &ggame) {
        ggame.maintained = false;
    });

    ticket_price.emplace(_self, [&](auto &tprice) {
        tprice.price = asset{10, CORE_SYMBOL};
    });

    games.emplace(_self, [&](auto &game) {
        game.draw = 1;
        game.status = LOCKING;
        game.hitnum = initHitnum;
    });
}

void trentlottery::setbonus(const double first, const double second, const asset& third, const asset& fourth, const asset& fifth, const asset& sixth)
{

    eosio_assert(third.symbol == CORE_SYMBOL, "only core token allowed");
    eosio_assert(third.is_valid(), "invalid third");
    eosio_assert(third.amount > 0, "must positive third amount");
    eosio_assert(fourth.symbol == CORE_SYMBOL, "only core token allowed");
    eosio_assert(fourth.is_valid(), "invalid bet");
    eosio_assert(fourth.amount > 0, "must positive bet amount");
    eosio_assert(fifth.symbol == CORE_SYMBOL, "only core token allowed");
    eosio_assert(fifth.is_valid(), "invalid bet");
    eosio_assert(fifth.amount > 0, "must positive bet amount");
    eosio_assert(sixth.symbol == CORE_SYMBOL, "only core token allowed");
    eosio_assert(sixth.is_valid(), "invalid bet");
    eosio_assert(sixth.amount > 0, "must positive bet amount");
    eosio_assert(first>0 && first<1, "first is less than 1 and more than 0");
    eosio_assert(second>0 && second<1, "second is less than 1 and more than 0");
    require_auth(_self);
    gamebonusgrade = {first, second, third, fourth, fifth, sixth};
}

void trentlottery::setprice(const asset &price)
{
    eosio_assert(isInMaintain() == false, "The game is under maintenance");
    eosio_assert(price.symbol == CORE_SYMBOL, "only core token allowed");
    eosio_assert(price.is_valid(), "invalid bet");
    eosio_assert(price.amount > 0, "must positive bet amount");
    require_auth(_self);

    auto price_itr = ticket_price.begin();
    eosio_assert(price_itr != ticket_price.end(), "The game not started!");
    ticket_price.modify(price_itr, 0, [&](auto &tprice) {
        tprice.price = price;
    });
}

void trentlottery::getprice()
{
    auto price = getticketprice();
    print("Ticket price is ", price, "\n");
}

void trentlottery::setlastgamestatus(game_status status)
{
    eosio_assert(isInMaintain() == false, "The game is under maintenance");
    eosio_assert(games.begin() != games.end(), "not started games!");
    auto lastgame_itr = games.rbegin();
    eosio_assert(lastgame_itr->status == LOCKING, "game is not locking");

    auto mgame_itr = games.find(lastgame_itr->draw);
    games.modify(mgame_itr, 0, [&](auto &game) {
        game.status = status;
    });
}

void trentlottery::enablegame()
{
    require_auth(_self);
    setlastgamestatus(BETTING);
}

void trentlottery::lockgame()
{
    require_auth(_self);
    setlastgamestatus(LOCKING);
}

uint64_t trentlottery::getnewestgame()
{
    require_auth(_self);
    eosio_assert(games.begin() != games.end(), "not started games!");
    auto lastgame_itr = games.rbegin();
    eosio_assert(lastgame_itr->status == LOCKING, "game is not LOCKING");
    return lastgame_itr->draw;
}

asset trentlottery::contractbalance()
{
    return eosio::token(N(eosio.token)).get_balance(_self, eosio::symbol_type(CORE_SYMBOL).name());
    // return asset{0, CORE_SYMBOL};
}

void trentlottery::jackpot()
{
    const auto balance = contractbalance();
    print("trentlottery balance: ", balance, "\n");
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
    if ((!hitblue && hitrednum == 5) || (hitblue && hitrednum == 4))
    {
        return 4;
    }
    if ((!hitblue && hitrednum == 4) || (hitblue && hitrednum == 3))
    {
        return 5;
    }
    if ((hitblue && hitrednum == 0) || (hitblue && hitrednum == 1) || (hitblue && hitrednum == 2))
    {
        return 6;
    }

    return price;
}

void trentlottery::setgamestate(bool maintenance)
{
    require_auth(_self);
    auto game_itr = globalgames.begin();
    if ( game_itr == globalgames.end() ) 
    {
        game_itr = globalgames.emplace(_self, [&](auto& ggame){
            ggame.maintained = false;
        });
    }

    globalgames.modify(game_itr, 0, [&](auto& ggame){
        ggame.maintained = maintenance;
    });
}

void trentlottery::getgamestate()
{
    auto game_itr = globalgames.begin();
    if ( game_itr == globalgames.end() ) 
    {
        print("game maintenance is false");
        return;
    }
    print("game maintenance is ", game_itr->maintained);
}

bool trentlottery::isInMaintain()
{
    auto game_itr = globalgames.begin();
    if ( game_itr == globalgames.end() ) 
    {
        return false;
    }
    return game_itr->maintained;
}

asset trentlottery::getticketprice()
{
    auto price_itr = ticket_price.begin();
    eosio_assert(price_itr != ticket_price.end(), "The game not started!");
    return price_itr->price;
}

void trentlottery::drawhighlottery(uint64_t draw, std::vector<winning> firstwinnings, std::vector<winning> secondwinnings)
{
    const auto balance = contractbalance();
    if(firstwinnings.size()>0){
        int64_t firstuint = int64_t(gamebonusgrade.first * 10);
        asset firstpot = balance * firstuint;
        asset firstbonus = firstpot/firstwinnings.size();
        for(uint16_t i = 0; i < secondwinnings.size(); i++){
        sendbonus(firstbonus,firstwinnings.at(i).winner,draw,firstwinnings.at(i).prize,firstwinnings.at(i).offernum);
        }
    }
   
    if(secondwinnings.size()>0){
        int64_t seconduint = int64_t(gamebonusgrade.second * 10);
        asset secondpot = (balance * seconduint)/10;
        asset secondbonus = secondpot/secondwinnings.size();
        for(uint16_t i = 0; i < secondwinnings.size(); i++){
        sendbonus(secondbonus,secondwinnings.at(i).winner,draw,secondwinnings.at(i).prize,secondwinnings.at(i).offernum);
        }
    }
    
}

void trentlottery::drawlottery()
{
    require_auth(_self);
    auto draw = getnewestgame();
    auto hitnum = generatehitnum();
    auto draw_index = offerbets.template get_index<N(draw)>();
    auto bet = draw_index.find(draw);
    vector<winning> firstwinnings;
    vector<winning> secondwinnings;

    for(; bet != draw_index.end(); bet++)
    {
        auto tickets = parseofferbet(bet->buycnt,bet->buylottos);
        for(uint16_t i = 0; i < tickets.size()-1; i++){
            auto prize = judgeprice(hitnum, tickets.at(i));
            winning winprize;
            switch(prize){
                case 1:
                winprize = {0,bet->player,draw,asset(0, CORE_SYMBOL), tickets.at(i), prize};
                firstwinnings.push_back(winprize);
                break;
                case 2:
                winprize = {0,bet->player,draw,asset(0, CORE_SYMBOL), tickets.at(i), prize};
                secondwinnings.push_back(winprize);
                break;
                case 3:
                sendbonus(gamebonusgrade.third, bet->player, draw, prize, tickets.at(i));
                break;
                case 4:
                sendbonus(gamebonusgrade.fourth, bet->player, draw, prize, tickets.at(i));
                break;
                case 5:
                sendbonus(gamebonusgrade.fifth, bet->player, draw, prize, tickets.at(i));
                break;
                case 6:
                sendbonus(gamebonusgrade.sixth, bet->player, draw, prize, tickets.at(i));
                break;
            }
        }
    }

    drawhighlottery(draw, firstwinnings, secondwinnings);

    creategame();
}

void trentlottery::sendbonus(asset bonus, account_name player, uint64_t draw, uint16_t prize, std::vector<uint16_t> offernum){
    require_auth(_self);
    action act(
        permission_level{_self, N(active)},
        N(eosio.token), N(transfer),
        std::make_tuple(_self, player, bonus, std::string("")));
    act.send();
    winnings.emplace(_self, [&](auto &winning) {
        winning.draw = draw;
        winning.prize = prize;
        winning.winner = player;
        winning.currency = bonus;
        winning.offernum = offernum;
    });
}

std::vector<uint16_t> trentlottery::generatehitnum(){
    vector<uint16_t> initHitnum(7, 0);
    return initHitnum;
}

EOSIO_ABI(trentlottery, (playerbet)(startgame)(enablegame)(setprice)(getprice)(jackpot)(getgamestate)(setgamestate)(lockgame)(drawlottery))
