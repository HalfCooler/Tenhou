#include "accounts.hpp"
#include "storage.hpp"
#include <windows.h>
#include <bcrypt.h>
#include <cmath>
namespace authority {
namespace {
std::string derive(const std::string& password,const std::string& salt) {
    BCRYPT_ALG_HANDLE algorithm=nullptr;
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,BCRYPT_ALG_HANDLE_HMAC_FLAG)<0) throw std::runtime_error("Password provider failed");
    std::array<UCHAR,32> output{};
    auto status=BCryptDeriveKeyPBKDF2(algorithm,reinterpret_cast<PUCHAR>(const_cast<char*>(password.data())),static_cast<ULONG>(password.size()),
        reinterpret_cast<PUCHAR>(const_cast<char*>(salt.data())),static_cast<ULONG>(salt.size()),210000,output.data(),static_cast<ULONG>(output.size()),0);
    BCryptCloseAlgorithmProvider(algorithm,0);
    if(status<0) throw std::runtime_error("Password derivation failed");
    std::string result; for(auto b:output) { result+="0123456789abcdef"[b>>4]; result+="0123456789abcdef"[b&15]; } return result;
}
}
std::string account_id(std::string value) {
    if(value.size()<3 || value.size()>32) throw std::runtime_error("账号须为 3～32 位英文字母、数字、下划线或短横线");
    for(auto& c:value) {
        if(c>='A' && c<='Z') c=static_cast<char>(c-'A'+'a');
        if(!((c>='a' && c<='z') || (c>='0' && c<='9') || c=='_' || c=='-')) throw std::runtime_error("账号含有无效字符");
    }
    if(value=="noname") throw std::runtime_error("NoName 是保留的访客账号"); return value;
}
Account new_account(const std::string& id,const std::string& nickname,const std::string& password) {
    Account a; a.id=account_id(id);
    if(nickname.empty() || nickname.size()>48 || !MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,nickname.data(),static_cast<int>(nickname.size()),nullptr,0) ||
        std::any_of(nickname.begin(),nickname.end(),[](unsigned char c){return c<32 || c==127;})) throw std::runtime_error("昵称不能为空，最多 16 个汉字且不能含控制字符");
    if(password.size()<8 || password.size()>128) throw std::runtime_error("密码须为 8～128 字节");
    a.nickname=nickname; a.salt=secret(); a.password_hash=derive(password,a.salt); return a;
}
bool password_matches(const Account& account,const std::string& password) {
    if(password.size()<8 || password.size()>128) return false;
    const auto expected=derive(password,account.salt); unsigned different=static_cast<unsigned>(expected.size()^account.password_hash.size());
    for(std::size_t i=0;i<expected.size();++i) different|=static_cast<unsigned char>(expected[i])^(i<account.password_hash.size()?static_cast<unsigned char>(account.password_hash[i]):0u);
    return different==0;
}
const char* rank_name(int level) {
    static constexpr const char* names[]{"新人","9级","8级","7级","6级","5级","4级","3级","2级","1级","初段","二段","三段","四段","五段","六段","七段","八段","九段"};
    return names[std::clamp(level,0,18)];
}
int rank_target(int level) {
    static constexpr int targets[]{20,20,20,20,40,60,80,100,100,100,400,800,1200,1600,2000,2400,2800,3200,3600}; return targets[std::clamp(level,0,18)];
}
void update_rank(Rank& r,int place,int players,bool south,double average) {
    if(place<0 || place>=players || (players!=3 && players!=4)) throw std::runtime_error("Invalid ranked result");
    int delta=players==3?(place==0?30:0):(place==0?20:place==1?10:0);
    if(place==players-1) delta=r.level<8?0:r.level==8?-10:r.level==9?-20:-(r.level-7)*10;
    if(south) delta=delta*3/2;
    r.points+=delta;
    if(r.level<18 && r.points>=rank_target(r.level)) { ++r.level; r.points=r.level<10?0:(r.level-9)*200; }
    else if(r.level>=10 && r.points<0) { --r.level; r.points=r.level<10?0:(r.level-9)*200; }
    else r.points=std::clamp(r.points,0,rank_target(r.level));
    const int finish=players==3?(place==0?30:place==1?0:-30):30-20*place;
    const double factor=std::max(.2,1-r.games*.002);
    r.rating=std::ceil((r.rating+factor*(finish+(std::max(1500.0,average)-r.rating)/40))*100-1e-8)/100;
    ++r.games; ++r.places[place];
}
}
