#include "matchmaking.hpp"
#include <algorithm>

namespace tenhou {
const std::array<MatchRule,12>& GeneralMatchRules() {
    // Original 0xfb1/2, 0xfb3/4, 0xfb7/8, 0xfb9/a; 0xfaf is no-red/no-kuitan.
    // User explicitly requests the corresponding no-kuitan sanma and fast forms.
    static const std::array<MatchRule,12> rules{{
        {1,L"四人 东"},{9,L"四人 南"},{65,L"速 四人 东"},{73,L"速 四人 南"},
        {17,L"三人 东"},{25,L"三人 南"},{81,L"速 三人 东"},{89,L"速 三人 南"},
        {7,L"四人 东 无食断"},{71,L"速 四人 东 无食断"},
        {23,L"三人 东 无食断"},{87,L"速 三人 东 无食断"}
    }};
    return rules;
}
bool MatchmakingDraft::CreateRoom() {
    if (request_ || room_ || intent_!=RoomIntent::None) return false;
    // Only the service allocates room numbers. This is an unnumbered form.
    room_=0; intent_=RoomIntent::Create;
    settings_={{"match_type","standard4"},{"length","east"},{"time","5+20"},{"fast","0"},{"advanced","0"},
        {"starting_points","25000"},{"target_points","30000"},{"bankruptcy","1"},{"ancient","0"},
        {"red_count","3"},{"open_tanyao","1"},{"minimum","1"},{"open_hands","0"}};
    return true;
}
bool MatchmakingDraft::JoinRoom(std::string_view number) {
    if (request_ || room_ || number.size()!=4 || number[0]<'1' || number[0]>'9' ||
        !std::all_of(number.begin(),number.end(),[](char c) { return c>='0' && c<='9'; })) return false;
    room_=0; for (char c:number) room_=room_*10+static_cast<unsigned>(c-'0');
    intent_=RoomIntent::Join; settings_.clear(); return true;
}
bool MatchmakingDraft::SetSettings(std::map<std::string,std::string> settings) {
    if (request_ || intent_!=RoomIntent::Create) return false;
    // Preserve a complete room draft; detailed domain validation remains in the editor/server.
    for (const auto& entry:settings_) if (!settings.count(entry.first)) return false;
    if ((settings.at("match_type")!="standard3" && settings.at("match_type")!="standard4") ||
        (settings.at("length")!="single" && settings.at("length")!="east" && settings.at("length")!="south")) return false;
    settings_=std::move(settings); return true;
}
bool MatchmakingDraft::Prepare(unsigned type) {
    if (request_) return false;
    if (intent_==RoomIntent::None) {
        const auto& rules=GeneralMatchRules();
        if (std::none_of(rules.begin(),rules.end(),[&](const MatchRule& rule) { return rule.type==type; })) return false;
    } else {
        // A joined room's rules must come from the future server, never invented locally.
        if (intent_==RoomIntent::Join) type=0;
        else type=1u | (settings_.at("match_type")=="standard3"?16u:0u) |
            (settings_.at("length")=="south"?8u:0u) | (settings_.at("fast")=="1"?64u:0u) |
            (settings_.at("open_tanyao")=="0"?4u:0u) | (settings_.at("red_count")=="0"?2u:0u);
    }
    request_=MatchRequest{type,room_,intent_,settings_}; return true;
}
void MatchmakingDraft::LeaveRoom() { if (!request_) { room_=0; intent_=RoomIntent::None; settings_.clear(); } }
void MatchmakingDraft::Reset() { Cancel(); LeaveRoom(); }
void MatchmakingDraft::Confirm(unsigned room,bool owner,std::map<std::string,std::string> settings) {
    request_.reset(); room_=room; intent_=room?(owner?RoomIntent::Create:RoomIntent::Join):RoomIntent::None;
    settings_=std::move(settings);
}
}
