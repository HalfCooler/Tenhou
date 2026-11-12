#pragma once
#include "common.hpp"
namespace authority {
struct Rank {
    int level=0,points=0,games=0;
    double rating=1500;
    std::array<int,4> places{};
};
struct Account {
    std::string id,nickname,salt,password_hash;
    std::array<Rank,2> ranks; // four-player, three-player
    int played=0;
};
std::string account_id(std::string value);
Account new_account(const std::string& id,const std::string& nickname,const std::string& password);
bool password_matches(const Account& account,const std::string& password);
const char* rank_name(int level);
int rank_target(int level);
void update_rank(Rank& rank,int place,int players,bool south,double average);
}
