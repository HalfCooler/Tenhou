#include "network_protocol_internal.hpp"
#include "monotonic_clock.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace tenhou {
    using protocol_detail::Attribute;
    using protocol_detail::Escape;
    using protocol_detail::Fail;
    using protocol_detail::Integer;

    namespace {
        int ProfileInteger(std::string_view text) {
            int value = 0;
            const auto end = text.data() + text.size();
            const auto parsed = std::from_chars(text.data(), end, value);
            return parsed.ec == std::errc{} && parsed.ptr == end ? value : 0;
        }

        double ProfileDecimal(std::string_view text) {
            double value = 0;
            const auto end = text.data() + text.size();
            const auto parsed = std::from_chars(text.data(), end, value);
            if (parsed.ec != std::errc{} || parsed.ptr != end || !(value >= std::numeric_limits<int>::min() &&
                value <= std::numeric_limits<int>::max())) return 0;
            return value;
        }

        std::int64_t ProfileScoreTenths(std::string_view text) {
            return static_cast<std::int64_t>(10 * ProfileDecimal(text));
        }

        std::vector<std::string_view> ProfileFields(std::string_view text, std::size_t max_fields) {
            std::vector<std::string_view> fields;
            if (text.empty()) return fields;
            while (fields.size() < max_fields) {
                const auto comma = text.find(',');
                fields.push_back(text.substr(0, comma));
                if (comma == std::string_view::npos) return fields;
                text.remove_prefix(comma + 1);
            }
            return {};
        }

        std::int64_t FillPlacements(LobbyStatistics &stats, const std::array<int, 4> &places, double score_hundredths) {
            stats.place_counts = places;
            const auto games = std::accumulate(places.begin(), places.end(), std::int64_t{0});
            if (!games) return 0;
            constexpr auto max_int = std::numeric_limits<int>::max();
            stats.games = static_cast<int>(std::min<std::int64_t>(games, max_int));
            std::int64_t weighted_places = 0;
            for (std::size_t i = 0; i < places.size(); ++i) {
                weighted_places += static_cast<int>(i + 1) * static_cast<std::int64_t>(places[i]);
                stats.place_per_mille[i] = static_cast<int>(1000LL * places[i] / games);
            }
            stats.average_place_hundredths = static_cast<int>(100 * weighted_places / games);
            const auto score = score_hundredths / static_cast<double>(games);
            stats.average_score_hundredths = static_cast<int>(std::clamp(score, -double(max_int), double(max_int)));
            return games;
        }

        void FillLifetimeStatistics(LobbyStatistics &stats, const std::vector<std::string_view> &fields,
                                    std::size_t score_index) {
            const auto field = [&](std::size_t offset) {
                const auto index = score_index + offset;
                return index < fields.size() ? fields[index] : std::string_view{""};
            };
            const auto count = [&](std::size_t offset) { return std::max(0, ProfileInteger(field(offset))); };
            // Origin converts the wire score in points to integer tenths before averaging.
            const auto score_tenths = ProfileScoreTenths(field(0));
            stats.total_score = static_cast<double>(score_tenths) * 0.1;
            const auto games = FillPlacements(stats, {count(1), count(2), count(3), count(4)},
                                             10.0 * score_tenths);
            stats.bust_count = count(5);
            if (games)
                stats.bust_per_mille = static_cast<int>(1000LL * std::min<std::int64_t>(stats.bust_count, games) / games);
            stats.hands = count(6);
            stats.win_count = count(7);
            stats.deal_in_count = count(8);
            stats.call_count = count(9);
            stats.reach_count = count(10);
            if (stats.hands) {
                const auto rate = [&](int value) {
                    return static_cast<int>(1000LL * std::min(value, stats.hands) / stats.hands);
                };
                stats.win_per_mille = rate(stats.win_count);
                stats.deal_in_per_mille = rate(stats.deal_in_count);
                stats.call_per_mille = rate(stats.call_count);
                stats.reach_per_mille = rate(stats.reach_count);
            }
        }

        LobbyRank ReadOfficialLifetime(const std::string &text, LobbyStatistics &stats) {
            constexpr std::array targets{20, 20, 20, 20, 40, 60, 80, 100, 100, 100, 400, 800,
                                         1200, 1600, 2000, 2400, 2800, 3200, 3600, 4000};
            const auto fields = ProfileFields(text, 14);
            if (fields.size() < 3) return {};
            const int grade = std::max(0, ProfileInteger(fields[0]));
            stats.points = ProfileInteger(fields[1]);
            stats.rating = static_cast<int>(ProfileDecimal(fields[2]));
            if (static_cast<std::size_t>(grade) < targets.size()) stats.next_points = targets[grade];
            FillLifetimeStatistics(stats, fields, 3);
            return {grade, stats.rating};
        }

        std::optional<LobbyStatistics> ReadOfficialMonthly(const std::string &text) {
            const auto fields = ProfileFields(text, 21);
            if (fields.size() != 15 && fields.size() != 21) return std::nullopt;
            std::array<int, 4> places{};
            for (std::size_t i = 0; i < places.size(); ++i)
                places[i] = std::max(0, ProfileInteger(fields[i]));
            LobbyStatistics stats{};
            stats.total_score = ProfileDecimal(fields[4]);
            FillPlacements(stats, places, 100 * stats.total_score);
            stats.has_monthly_raw = true;
            stats.monthly_ranks.resize(fields.size() == 21 ? 13 : 9, 0);
            for (std::size_t i = 0; i < 9; ++i)
                stats.monthly_ranks[i] = std::max(0, ProfileInteger(fields[6 + i]));
            if (fields.size() == 21) {
                stats.has_amber_monthly = true;
                stats.total_income = ProfileDecimal(fields[15]);
                stats.total_chips = ProfileInteger(fields[16]);
                for (std::size_t i = 0; i < 4; ++i)
                    stats.monthly_ranks[9 + i] = std::max(0, ProfileInteger(fields[17 + i]));
            }
            return stats;
        }

        std::vector<int> DecimalList(const std::string &text, std::size_t limit) {
            std::vector<int> values;
            if (text.empty()) return values;
            std::istringstream fields(text);
            for (std::string field; std::getline(fields, field, ',');) {
                const auto value = Integer(field);
                if (value > 100000000 || values.size() == limit) Fail("Invalid self-hosted lobby list");
                values.push_back(static_cast<int>(value));
            }
            if (text.back() == ',') Fail("Incomplete self-hosted lobby list");
            return values;
        }

        OfflineLobbyProfile ServiceProfile(const NetworkMessage &message) {
            OfflineLobbyProfile profile{};
            profile.name = Attribute(message, "nickname");
            profile.membership_expiry = Attribute(message, "expire");
            profile.ranked_allowed = !Attribute(message, "account").empty();
            const auto number = [&](const std::string &key) {
                const auto value = Attribute(message, key);
                return value.empty() ? 0 : static_cast<int>(Integer(value.substr(0, value.find('.'))));
            };
            profile.membership_days = number("expiredays");
            profile.light_options = number("opt");
            for (int i = 0; i < 2; ++i) {
                const std::string n = i ? "3" : "4";
                auto &rank = i ? profile.three : profile.four;
                auto &stats = profile.lifetime[i];
                rank = {number("dan" + n), number("rating" + n)};
                stats.games = number("games" + n);
                stats.points = number("points" + n);
                stats.next_points = number("target" + n);
                const auto places = DecimalList(Attribute(message, "places" + n), 4);
                if (!stats.games || places.size() != 4) continue;
                std::int64_t total_places = 0;
                for (int p = 0; p < 4; ++p) {
                    total_places += (p + 1) * static_cast<std::int64_t>(places[p]);
                    stats.place_per_mille[p] = static_cast<int>(1000LL * places[p] / stats.games);
                }
                stats.average_place_hundredths = static_cast<int>(100 * total_places / stats.games);
            }
            return profile;
        }
    }

    LobbyStatistics ReadPrivateLobbyStatistics(std::string_view text) {
        LobbyStatistics stats{};
        FillLifetimeStatistics(stats, ProfileFields(text, 11), 0);
        return stats;
    }

    std::string LobbySession::RequestOfficialProfile(int type) {
        official_profile_type_ = type;
        if (snapshot.origin_lobby) snapshot.origin_lobby->profile.monthly.clear();
        return "<PXR V=\"" + std::to_string(type) + "\" />";
    }

    std::string LobbySession::OfficialProfilePoll() const {
        return "<PXR v=\"" + std::to_string(official_profile_type_) + "\" />";
    }

    void LobbySession::ReturnToOriginLobby() {
        const auto revision = snapshot.actions.revision + 1;
        game_ = {};
        actions_ = {};
        actions_.official = official;
        actions_.snapshot.enabled = interactive_games;
        actions_.snapshot.revision = revision;
        snapshot.game = {};
        snapshot.actions = actions_.snapshot;
        snapshot.exit_table.clear();
        snapshot.exit_allowed = snapshot.table_paused = snapshot.recovering = false;
        rejoin_received_ = recovery_go_ = official_start_ready_ = false;
        snapshot.native_window = 0;
        snapshot.target_lobby.clear();
        snapshot.phase = LobbyPhase::Lobby;
        ++snapshot.lobby_revision;
    }

    void LobbySession::FinishOfficialRecovery() {
        resume_connection = snapshot.recovering = snapshot.silent_recovery = false;
    }

    bool LobbySession::ApplyOfficialLobby(const NetworkMessage &message) {
        if (snapshot.phase == LobbyPhase::AwaitHelo) return false;
        // The official service uses RANKING for both the personal v2 list and contest rankings.
        if ((message.tag == "RANKING" || message.tag == "RANK") && message.attributes.count("v2")) {
            if (snapshot.origin_lobby && official_profile_type_ >= 0) {
                const auto stats = ReadOfficialMonthly(Attribute(message, "v2"));
                if (stats) snapshot.origin_lobby->profile.monthly[official_profile_type_] = *stats;
            }
            return true;
        }
        if (message.tag == "LN") {
            if (!snapshot.origin_lobby || official_profile_dirty_) {
                OfflineLobbyProfile profile{};
                profile.name = snapshot.identity;
                profile.ranked_allowed = snapshot.identity != "NoName";
                const auto field = [&](const std::string &key) {
                    const auto it = snapshot.server.find(key);
                    return it == snapshot.server.end() ? std::string{} : it->second;
                };
                profile.membership_expiry = field("expire");
                profile.membership_days = field("expiredays").empty() ? !profile.membership_expiry.empty() :
                                          ProfileInteger(field("expiredays"));
                profile.light_options = std::max(0, ProfileInteger(field("opt")));
                profile.four = ReadOfficialLifetime(field("PF4"), profile.lifetime[0]);
                profile.three = ReadOfficialLifetime(field("PF3"), profile.lifetime[1]);
                constexpr const char *rating_keys[]{"PF01C", "PF02C", "PF03C", "PF11C",
                                                    "PF12C", "PF13C", "PF23C", "PF33C"};
                for (std::size_t i = 0; i < kAmberLifetimeProfiles.size(); ++i) {
                    const auto type = kAmberLifetimeProfiles[i];
                    const auto record = field("PF" + std::to_string(type));
                    const auto rating = field(rating_keys[i]);
                    if (record.empty() && rating.empty()) continue;
                    LobbyStatistics stats{};
                    const auto results = ProfileFields(record, 12);
                    const auto rating_fields = ProfileFields(rating, 3);
                    if (rating_fields.size() >= 2)
                        stats.rating = static_cast<int>(ProfileDecimal(rating_fields[1]));
                    FillLifetimeStatistics(stats, results, 1);
                    if (results.size() >= 2 && stats.games) {
                        constexpr int chip_values[]{0, 2, 5, 0, 2, 5, 5, 5};
                        const auto raw_chips = ProfileInteger(results[0]);
                        stats.total_chips = raw_chips;
                        const auto score_tenths = ProfileScoreTenths(results[1]);
                        const auto score = score_tenths / 10;
                        const auto chips = static_cast<std::int64_t>(raw_chips) * chip_values[i];
                        constexpr auto max_int = static_cast<std::int64_t>(std::numeric_limits<int>::max());
                        stats.average_g_hundredths = static_cast<int>(
                            std::clamp(10000LL * (score + chips) / stats.games, -max_int, max_int));
                        if (chip_values[i] > 0) {
                            stats.average_income = (static_cast<double>(score_tenths) * 0.1 +
                                                    static_cast<double>(chips)) / static_cast<double>(stats.games);
                            stats.average_chips = static_cast<double>(raw_chips) / static_cast<double>(stats.games);
                        }
                    }
                    profile.other_lifetime.emplace(type, stats);
                }
                if (!snapshot.origin_lobby) snapshot.origin_lobby.emplace(std::move(profile));
                else {
                    snapshot.origin_lobby->profile = std::move(profile);
                    snapshot.origin_lobby->Enter(kRankedLobby);
                }
                snapshot.lobby = "L0000";
                official_profile_dirty_ = false;
            }
            auto &lobby = *snapshot.origin_lobby;
            auto counts = lobby.counts();
            const auto decode = [&](const char *key, auto &array) {
                const auto value = Attribute(message, key);
                const auto values = value.empty() ? std::vector<int>{} : DecodeLobbyNumbers(value);
                array.fill(0);
                for (std::size_t i = 0; i < std::min(values.size(), array.size()); ++i)
                    array[i] = static_cast<unsigned>(std::max(values[i], 0));
            };
            decode("n", counts.totals);
            decode("j", counts.waiting);
            decode("g", counts.playing);
            lobby.SetCounts(lobby.room(), counts);
            if (snapshot.phase == LobbyPhase::AwaitLobby) snapshot.phase = LobbyPhase::Lobby;
            return true;
        }
        if (message.tag == "CHAT" && message.attributes.count("lobby") && snapshot.origin_lobby &&
            !game_.state.started) {
            const auto room = LobbyNumber(Attribute(message, "lobby"));
            if (room) {
                snapshot.origin_lobby->Enter(*room);
                snapshot.lobby = *room;
                snapshot.target_lobby.clear();
                snapshot.origin_pending.clear();
                snapshot.phase = LobbyPhase::Lobby;
                ++snapshot.lobby_revision;
            }
            return true;
        }
        return false;
    }

    bool LobbySession::ApplyOriginLobby(const NetworkMessage &message) {
        const auto &tag = message.tag;
        if (tag == "PROFILE") {
            snapshot.profile = message.attributes;
            if (snapshot.origin_lobby) snapshot.origin_lobby->profile = ServiceProfile(message);
            return true;
        }
        if (tag != "CHAT" && tag != "CS" && tag != "LN" && tag != "RESERVATIONS") return false;
        if (!message.attributes.count("epoch")) return true;
        const auto epoch = Integer(Attribute(message, "epoch"));
        const auto id = Attribute(message, "lobby");
        const auto rid = Attribute(message, "rid");
        const auto phase = snapshot.phase;
        const bool receipt = !snapshot.origin_pending.empty() && rid == std::to_string(origin_request_);
        if (tag == "CHAT") {
            const auto room = LobbyNumber(id);
            if (!room || *room != id) Fail("Invalid self-hosted lobby identifier");
            if (phase == LobbyPhase::AwaitLobby && origin_epoch_ == 0 && epoch == 1 && rid.empty()) {
                snapshot.origin_lobby.emplace(ServiceProfile({"PROFILE", snapshot.profile}));
                snapshot.recovering = Attribute(message, "seated") == "1";
                if (!snapshot.recovering) resume_connection = false;
            } else if (phase == LobbyPhase::ChangingLobby && receipt && epoch == origin_epoch_ + 1 &&
                       id == snapshot.target_lobby) {
                snapshot.target_lobby.clear();
            } else if (phase == LobbyPhase::AwaitReturn && epoch == origin_epoch_ && id == snapshot.target_lobby && rid.empty()) {
                ReturnToOriginLobby();
            } else if (phase == LobbyPhase::Lobby && epoch == origin_epoch_ && id == snapshot.lobby && rid.empty()) {
                return true;
            } else return true;
            auto profile = snapshot.origin_lobby->profile;
            snapshot.origin_lobby.emplace(std::move(profile));
            snapshot.origin_lobby->Enter(*room);
            snapshot.lobby = id;
            origin_epoch_ = epoch;
            snapshot.origin_pending.clear();
            snapshot.origin_error.clear();
            snapshot.origin_clock = snapshot.origin_clock_at = 0;
            snapshot.phase = LobbyPhase::Lobby;
            ++snapshot.lobby_revision;
            return true;
        }
        if (epoch != origin_epoch_ || id != snapshot.lobby || !snapshot.origin_lobby ||
            (phase != LobbyPhase::Lobby && phase != LobbyPhase::Matching && phase != LobbyPhase::Cancelling &&
             phase != LobbyPhase::ChangingLobby)) return true;
        auto &lobby = *snapshot.origin_lobby;
        if (tag == "CS") {
            if (!lobby.contest()) return true;
            const auto number = [&](const char *key) { return Integer(Attribute(message, key)); };
            const auto players = number("players"), type = number("rule"), start = number("start"), end = number("end");
            if ((players != 3 && players != 4) || end <= start) Fail("Invalid self-hosted contest");
            lobby.SetContest(lobby.room(), {Attribute(message, "title"), players, type, number("joinfee"),
                number("premiumonly") == 1, number("registered") == 1, number("blocked") == 1, start, end});
            snapshot.origin_clock = number("now");
            snapshot.origin_clock_at = MonotonicMs();
        } else if (tag == "LN") {
            const auto n = DecimalList(Attribute(message, "n"), 4);
            const auto j = DecimalList(Attribute(message, "j"), 113);
            const auto g = DecimalList(Attribute(message, "g"), 113);
            OfflineLobbyCounts counts;
            if (n.size() != counts.totals.size() || j.size() != counts.waiting.size() ||
                g.size() != counts.playing.size()) Fail("Invalid self-hosted lobby counts");
            std::copy(n.begin(), n.end(), counts.totals.begin());
            std::copy(j.begin(), j.end(), counts.waiting.begin());
            std::copy(g.begin(), g.end(), counts.playing.begin());
            lobby.SetCounts(lobby.room(), counts);
        } else if (rid.empty() || receipt) {
            const auto types = DecimalList(Attribute(message, "types"), 48);
            lobby.SetReservations(types);
            snapshot.origin_error = Attribute(message, "code");
            if (receipt) {
                lobby.DeclineMove();
                snapshot.origin_pending.clear();
                snapshot.target_lobby.clear();
            }
            if (snapshot.origin_pending.empty())
                snapshot.phase = lobby.reservations() ? LobbyPhase::Matching : LobbyPhase::Lobby;
        }
        return true;
    }

    std::vector<std::string> LobbySession::SubmitOriginLobby(const std::string &command, const std::string &value) {
        if (official && command == "monthly-profile") {
            const auto type = Integer(value);
            if (!ValidMonthlyProfile(type)) return {};
            preferred_monthly_profile = type;
            const auto phase = snapshot.phase;
            if (!snapshot.origin_lobby || snapshot.recovering ||
                (phase != LobbyPhase::Lobby && phase != LobbyPhase::Matching) ||
                static_cast<int>(type) == official_profile_type_) return {};
            return {RequestOfficialProfile(static_cast<int>(type))};
        }
        if (!snapshot.origin_lobby || !snapshot.origin_pending.empty() ||
            (snapshot.phase != LobbyPhase::Lobby && snapshot.phase != LobbyPhase::Matching) || snapshot.recovering)
            return {};
        auto &lobby = *snapshot.origin_lobby;
        if (official) {
            if (command == "join") {
                const auto type = Integer(value);
                if ((type & 0x810) == 0x810 || lobby.contest() || lobby.Reserved(type)) return {};
                const bool verified_practice = lobby.ranked() && (type == 1024 || type == 1552);
                bool reserved = verified_practice;
                for (unsigned tier = 0; tier < 4 && !reserved; ++tier)
                    for (unsigned slot = 0; slot < kRankedSlots && !reserved; ++slot)
                        if (lobby.Candidate(static_cast<RankedTier>(tier), slot).type == type)
                            reserved = lobby.Reserve(static_cast<RankedTier>(tier), slot);
                if (!reserved) return {};
                snapshot.match_type = type;
                snapshot.phase = LobbyPhase::Matching;
                return {"<JOIN t=\"" + std::to_string(Integer(snapshot.lobby.substr(1))) + "," +
                        std::to_string(type) + "\" />"};
            }
            if (command == "cancel" && lobby.reservations()) {
                lobby.Cancel();
                snapshot.phase = LobbyPhase::Lobby;
                return {"<JOIN />"};
            }
            if (command != "change" || !lobby.RequestMove(value)) return {};
            snapshot.target_lobby = *lobby.pending();
            const auto &target = snapshot.target_lobby;
            snapshot.phase = LobbyPhase::ChangingLobby;
            return {target.front() == 'C' ? "<CS lobby=\"" + target + "\" />" :
                "<CHAT text=\"%2Flobby%20" + std::to_string(Integer(target.substr(1))) + "\" />"};
        }
        std::string wire;
        if (command == "change") {
            if (!lobby.RequestMove(value)) return {};
            snapshot.target_lobby = *lobby.pending();
            const auto &target = snapshot.target_lobby;
            wire = target.front() == 'C' ? "<CS target=\"" + target + "\"" :
                "<CHAT text=\"%2Flobby%20" + std::to_string(Integer(target.substr(1))) + "\"";
            snapshot.phase = LobbyPhase::ChangingLobby;
        } else if (command == "join") {
            const auto type = Integer(value);
            wire = lobby.contest() ? "<JOIN type=\"" + std::to_string(type) + "\"" :
                "<JOIN t=\"" + std::to_string(Integer(snapshot.lobby.substr(1))) + "," + std::to_string(type) + "\"";
            snapshot.phase = LobbyPhase::Matching;
        } else if (command == "cancel" && lobby.reservations()) {
            wire = "<JOIN";
            snapshot.phase = LobbyPhase::Cancelling;
        } else if (command == "register" && lobby.contest()) {
            wire = "<CSREG";
        } else return {};
        snapshot.origin_pending = command;
        snapshot.origin_error.clear();
        return {wire + " lobby=\"" + snapshot.lobby + "\" epoch=\"" + std::to_string(origin_epoch_) +
            "\" rid=\"" + std::to_string(++origin_request_) + "\" />"};
    }

    std::vector<std::string> LobbySession::SubmitLobby(std::uint64_t revision, const std::string &command,
                                                       unsigned value) {
        if (!match_games || !interactive_games || observe_games) Fail("Local matchmaking is disabled");
        if (revision != snapshot.lobby_revision) Fail("Stale lobby revision");
        if (official && command == "return" && snapshot.phase == LobbyPhase::MatchComplete) {
            ReturnToOriginLobby();
            official_game_id.clear();
            official_profile_type_ = -1;
            if (!official_identity_.empty()) {
                snapshot.phase = LobbyPhase::AwaitReturn;
                official_profile_dirty_ = true;
                return {"<HELO name=\"" + Escape(official_identity_) + "\" sx=\"" + official_gender_ + "\" tid=\"w0\" />"};
            }
            return {};
        }
        const auto roomName = [](unsigned room) {
            std::ostringstream out;
            out << 'L' << std::setw(4) << std::setfill('0') << room;
            return out.str();
        };
        const auto change = [](unsigned room) { return "<CHAT text=\"%2Flobby%20" + std::to_string(room) + "\" />"; };
        std::vector<std::string> replies;
        if (command == "change") {
            if (snapshot.phase != LobbyPhase::Lobby) Fail("Lobby change requires ready lobby");
            if (value != 0 && (value < 1000 || value > 9999)) Fail("Supported room is 0 or 1000..9999");
            replies = {change(value)};
            snapshot.room_settings.clear();
            room_ = value;
            snapshot.target_lobby = roomName(value);
            snapshot.phase = LobbyPhase::ChangingLobby;
        } else if (command == "match") {
            if (snapshot.phase != LobbyPhase::Lobby || snapshot.lobby != roomName(room_)) Fail(
                "Confirm a supported room before matching");
            auto queue = std::find_if(snapshot.queues.begin(), snapshot.queues.end(), [&](const ServerQueue &q) { return q.id == value; });
            if (!snapshot.catalog_ready || (!room_ && queue == snapshot.queues.end())) Fail("Select a server queue");
            if (!room_) {
                const auto rule = snapshot.rules.find(queue->rule);
                if (rule == snapshot.rules.end() || (rule->second.presentation != "riichi-v1" && rule->second.presentation != "mahjong-v1")) Fail("This rule needs a newer table renderer");
            } else {
                const auto rule = snapshot.rules.find(snapshot.room_settings.at("rule"));
                if (rule == snapshot.rules.end() || (rule->second.presentation != "riichi-v1" && rule->second.presentation != "mahjong-v1")) Fail("This rule needs a newer table renderer");
            }
            replies = {"<JOIN t=\"" + std::to_string(room_) + "," + std::to_string(value) + "\" />"};
            if (snapshot.native_service && room_) {
                if (!snapshot.room_settings.count("revision")) Fail("Confirm room settings before joining queue");
                replies = {
                    "<JOIN t=\"" + std::to_string(room_) + "," + std::to_string(value) + "\" revision=\"" + Escape(
                        snapshot.room_settings.at("revision")) + "\" />"
                };
            }
            snapshot.match_type = room_ ? Integer(snapshot.room_settings.at("type")) : queue->type;
            snapshot.match_result.clear();
            snapshot.phase = LobbyPhase::Matching;
            // JOIN selects family/length; local ROOM supplies the complete GO flags.
            if (!snapshot.native_service && !snapshot.room_settings.empty()) {
                const auto &room = snapshot.room_settings;
                snapshot.match_type |= (room.at("red_count") == "0" ? 2u : 0u) |
                        (room.at("open_tanyao") == "0" ? 4u : 0u) | (room.at("fast") == "1" ? 64u : 0u);
            }
            snapshot.last_settlement = {};
            snapshot.last_result_names = {};
            snapshot.queue_confirmed = false;
            game_.state.settlement = {};
            snapshot.game.settlement = {};
        } else if (command == "cancel") {
            if (snapshot.phase != LobbyPhase::Matching) Fail("No match request to cancel");
            replies = {"<JOIN />", change(room_)};
            snapshot.target_lobby = roomName(room_);
            snapshot.phase = LobbyPhase::Cancelling;
        } else if (command == "return") {
            if (snapshot.phase != LobbyPhase::MatchComplete || !snapshot.game.complete) Fail("Match is not complete");
            if (snapshot.origin_lobby) {
                replies = {"<BYE />"};
                snapshot.target_lobby = snapshot.lobby;
            } else {
                if (snapshot.lobby != roomName(room_)) Fail("Return requires a confirmed supported lobby");
                replies = {"<BYE />", change(room_)};
                snapshot.target_lobby = roomName(room_);
            }
            snapshot.phase = LobbyPhase::AwaitReturn;
            snapshot.variant_active = false;
            snapshot.variant_state.clear();
        } else Fail("Unknown lobby command");
        ++snapshot.lobby_revision;
        snapshot.native_error.clear();
        return replies;
    }

    std::vector<std::string> LobbySession::NativeRequest(const std::string &command, unsigned value,
                                                         const std::map<std::string, std::string> &settings) {
        if (!snapshot.native_service || snapshot.phase != LobbyPhase::Lobby || !snapshot.native_pending.empty()) Fail(
            "Native room request is not available");
        std::string tag;
        if (command == "create") tag = "ROOMCREATE";
        else if (command == "join") tag = "ROOMJOIN";
        else if (command == "leave") tag = "ROOMLEAVE";
        else if (command == "close") tag = "ROOMCLOSE";
        else if (command == "settings") tag = "SETROOM";
        else Fail("Unknown native request");
        std::string wire = "<" + tag + " rid=\"" + std::to_string(++snapshot.native_request) + "\"";
        if (command == "join") {
            if (value < 1000 || value > 9999) Fail("Invalid room number");
            wire += " room=\"" + std::to_string(value) + "\"";
        }
        if (command == "settings") {
            if (!snapshot.room_settings.count("revision")) Fail("Room revision missing");
            wire += " revision=\"" + Escape(snapshot.room_settings.at("revision")) + "\"";
        }
        if (command == "create" || command == "settings")
            for (const auto &item: settings) {
                if (item.first == "room" || item.first == "revision" || item.first == "owner" || item.first == "rid" || item.first == "type")
                    continue;
                if (item.first.empty() || !std::all_of(item.first.begin(), item.first.end(),
                                                       [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'; })) Fail(
                    "Invalid setting key");
                wire += " " + item.first + "=\"" + Escape(item.second) + "\"";
            }
        snapshot.native_error.clear();
        snapshot.native_pending = command;
        return {wire + " />"};
    }

} // namespace tenhou
