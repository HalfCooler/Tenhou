#include "replay.hpp"
#include "zlib.h"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace tenhou {
    namespace {
        constexpr std::size_t kMaximumBytes = 16 * 1024 * 1024;
        constexpr std::size_t kMaximumEvents = 100000;
        [[noreturn]] void Fail(const std::string &message) { throw std::runtime_error(message); }
        bool Space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

        bool NameChar(char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                   (c >= '0' && c <= '9') || c == '_' || c == ':' || c == '-' || c == '.';
        }

        void ValidateUtf8(const std::string &text, const std::string &what) {
            for (std::size_t i = 0; i < text.size();) {
                const auto lead = static_cast<unsigned char>(text[i++]);
                if (lead < 128) {
                    if (lead < 32 && lead != 9 && lead != 10 && lead != 13)
                        Fail("Invalid control character in " + what);
                    continue;
                }
                unsigned code = 0, minimum = 0;
                std::size_t extra = 0;
                if (lead >= 0xc2 && lead <= 0xdf) {
                    code = lead & 31;
                    extra = 1;
                    minimum = 0x80;
                } else if (lead >= 0xe0 && lead <= 0xef) {
                    code = lead & 15;
                    extra = 2;
                    minimum = 0x800;
                } else if (lead >= 0xf0 && lead <= 0xf4) {
                    code = lead & 7;
                    extra = 3;
                    minimum = 0x10000;
                } else Fail("Invalid UTF-8 in " + what);
                if (extra > text.size() - i) Fail("Truncated UTF-8 in " + what);
                for (std::size_t j = 0; j < extra; ++j) {
                    const auto next = static_cast<unsigned char>(text[i++]);
                    if ((next & 0xc0) != 0x80) Fail("Invalid UTF-8 continuation in " + what);
                    code = (code << 6) | (next & 63);
                }
                if (code < minimum || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff) || code == 0xfffe || code ==
                    0xffff)
                    Fail("Invalid UTF-8 codepoint in " + what);
            }
        }

        int Number(const std::string &value, const std::string &what, int low, int high) {
            int result = 0;
            const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
            if (value.empty() || parsed.ec != std::errc() || parsed.ptr != value.data() + value.size() ||
                result < low || result > high)
                Fail("Invalid " + what + ": " + value);
            return result;
        }

        std::string Attribute(const ReplayEvent &event, const std::string &name, bool required = true) {
            const auto found = event.attributes.find(name);
            if (found != event.attributes.end()) return found->second;
            if (required) Fail("Missing " + name + " attribute");
            return {};
        }

        int Integer(const ReplayEvent &event, const std::string &name, int low, int high) {
            return Number(Attribute(event, name), name, low, high);
        }

        std::vector<int> Numbers(const std::string &text, const std::string &name, int low, int high) {
            std::vector<int> result;
            std::size_t start = 0;
            do {
                const auto comma = text.find(',', start);
                result.push_back(Number(text.substr(start, comma == std::string::npos ? comma : comma - start), name,
                                        low, high));
                if (comma == std::string::npos) break;
                start = comma + 1;
            } while (true);
            return result;
        }

        void Utf8Append(std::string &out, unsigned code) {
            if ((code < 32 && code != 9 && code != 10 && code != 13) || code > 0x10ffff ||
                (code >= 0xd800 && code <= 0xdfff) || code == 0xfffe || code == 0xffff)
                Fail("Invalid XML character entity");
            if (code <= 0x7f) out += static_cast<char>(code);
            else if (code <= 0x7ff) {
                out += static_cast<char>(0xc0 | (code >> 6));
                out += static_cast<char>(0x80 | (code & 63));
            } else if (code <= 0xffff) {
                out += static_cast<char>(0xe0 | (code >> 12));
                out += static_cast<char>(0x80 | ((code >> 6) & 63));
                out += static_cast<char>(0x80 | (code & 63));
            } else {
                out += static_cast<char>(0xf0 | (code >> 18));
                out += static_cast<char>(0x80 | ((code >> 12) & 63));
                out += static_cast<char>(0x80 | ((code >> 6) & 63));
                out += static_cast<char>(0x80 | (code & 63));
            }
        }

        std::string Entities(const std::string &text) {
            std::string result;
            for (std::size_t i = 0; i < text.size(); ++i) {
                if (text[i] != '&') {
                    result += text[i];
                    continue;
                }
                const auto end = text.find(';', i);
                if (end == std::string::npos) Fail("Unterminated XML entity");
                const auto entity = text.substr(i + 1, end - i - 1);
                if (entity == "amp") result += '&';
                else if (entity == "lt") result += '<';
                else if (entity == "gt") result += '>';
                else if (entity == "quot") result += '"';
                else if (entity == "apos") result += '\'';
                else if (!entity.empty() && entity[0] == '#') {
                    unsigned code = 0;
                    const bool hex = entity.size() > 1 && entity[1] == 'x';
                    const auto offset = hex ? 2 : 1;
                    const auto decoded = std::from_chars(entity.data() + offset, entity.data() + entity.size(), code,
                                                         hex ? 16 : 10);
                    if (decoded.ec != std::errc() || decoded.ptr != entity.data() + entity.size()) Fail(
                        "Invalid XML entity");
                    Utf8Append(result, code);
                } else Fail("Unsupported XML entity: " + entity);
                i = end;
            }
            return result;
        }

        std::string PercentDecode(const std::string &text) {
            std::string result;
            for (std::size_t i = 0; i < text.size(); ++i) {
                if (text[i] != '%') {
                    result += text[i];
                    continue;
                }
                if (i + 2 >= text.size()) Fail("Invalid percent-encoded player name");
                unsigned byte = 0;
                const auto decoded = std::from_chars(text.data() + i + 1, text.data() + i + 3, byte, 16);
                if (decoded.ec != std::errc() || decoded.ptr != text.data() + i + 3 || byte == 0) Fail(
                    "Invalid percent-encoded player name");
                result += static_cast<char>(byte);
                i += 2;
            }
            ValidateUtf8(result, "decoded player name");
            return result;
        }

        struct Tag {
            ReplayEvent event;
            bool closing = false, self_closing = false;
        };

        class XmlReader {
        public:
            explicit XmlReader(const std::string &xml) : xml_(xml) {
                if (xml.compare(0, 3, "\xef\xbb\xbf") == 0) position_ = 3;
            }

            bool Next(Tag &tag) {
                for (;;) {
                    while (position_ < xml_.size() && Space(xml_[position_])) ++position_;
                    if (position_ == xml_.size()) return false;
                    if (xml_.compare(position_, 4, "<!--") == 0) {
                        const auto end = xml_.find("-->", position_ + 4);
                        if (end == std::string::npos) Fail("Unterminated XML comment");
                        position_ = end + 3;
                        continue;
                    }
                    if (xml_.compare(position_, 5, "<?xml") == 0) {
                        if (declaration_ || root_seen_) Fail("Misplaced XML declaration");
                        const auto end = xml_.find("?>", position_ + 5);
                        if (end == std::string::npos) Fail("Unterminated XML declaration");
                        const auto declaration = xml_.substr(position_, end - position_);
                        if (declaration.find("UTF-16") != std::string::npos || declaration.find("utf-16") !=
                            std::string::npos)
                            Fail("Only UTF-8 XML is supported");
                        declaration_ = true;
                        position_ = end + 2;
                        continue;
                    }
                    break;
                }
                tag = {};
                tag.event.source_offset = position_;
                if (xml_[position_++] != '<') Fail("Expected XML tag at byte " + std::to_string(position_ - 1));
                if (position_ < xml_.size() && xml_[position_] == '/') {
                    tag.closing = true;
                    ++position_;
                }
                tag.event.tag = Name();
                for (;;) {
                    if (position_ == xml_.size()) Fail("Truncated XML tag");
                    if (xml_[position_] == '>') {
                        ++position_;
                        break;
                    }
                    if (xml_[position_] == '/' && !tag.closing) {
                        ++position_;
                        Expect('>');
                        tag.self_closing = true;
                        break;
                    }
                    if (!Space(xml_[position_])) Fail("Expected whitespace before attribute");
                    while (position_ < xml_.size() && Space(xml_[position_])) ++position_;
                    if (position_ < xml_.size() && (xml_[position_] == '>' || xml_[position_] == '/')) continue;
                    if (tag.closing) Fail("Attributes on closing tag");
                    const auto name = Name();
                    while (position_ < xml_.size() && Space(xml_[position_])) ++position_;
                    Expect('=');
                    while (position_ < xml_.size() && Space(xml_[position_])) ++position_;
                    if (position_ == xml_.size() || (xml_[position_] != '\'' && xml_[position_] != '"')) Fail(
                        "Expected quoted XML attribute");
                    const char quote = xml_[position_++];
                    const auto start = position_;
                    while (position_ < xml_.size() && xml_[position_] != quote) {
                        if (xml_[position_] == '<' || xml_[position_] == '\0') Fail("Invalid XML attribute character");
                        ++position_;
                    }
                    if (position_ == xml_.size()) Fail("Unterminated XML attribute");
                    if (!tag.event.attributes.emplace(name, Entities(xml_.substr(start, position_ - start))).second)
                        Fail("Duplicate attribute: " + name);
                    ++position_;
                }
                root_seen_ = true;
                return true;
            }

        private:
            std::string Name() {
                const auto start = position_;
                while (position_ < xml_.size() && NameChar(xml_[position_])) ++position_;
                if (position_ == start) Fail("Invalid XML tag or attribute name at byte " + std::to_string(start));
                return xml_.substr(start, position_ - start);
            }

            void Expect(char c) {
                if (position_ >= xml_.size() || xml_[position_++] != c) Fail(std::string("Expected '") + c + "'");
            }

            const std::string &xml_;
            std::size_t position_ = 0;
            bool declaration_ = false, root_seen_ = false;
        };

        void ValidateTile(int tile) { if (tile < 0 || tile >= 136) Fail("Tile ID outside 0..135"); }

        bool HasTile(const std::vector<int> &tiles, int tile) {
            return std::find(tiles.begin(), tiles.end(), tile) != tiles.end();
        }

        bool VisibleTile(const ReplayState &state, int tile) {
            if (HasTile(state.dora_indicators, tile)) return true;
            for (const auto &player: state.players) {
                if (HasTile(player.hand, tile)) return true;
                if (HasTile(player.nuki, tile)) return true;
                for (const auto &discard: player.discards) if (discard.tile == tile && !discard.claimed) return true;
                for (const auto &meld: player.melds) if (HasTile(meld.tiles, tile)) return true;
            }
            return false;
        }

        void RemoveTile(ReplayPlayer &player, int tile) {
            const auto found = std::find(player.hand.begin(), player.hand.end(), tile);
            if (found == player.hand.end()) Fail("Tile " + std::to_string(tile) + " is not in actor's hand");
            player.hand.erase(found);
        }

        void Scores(ReplayState &state, const ReplayEvent &event) {
            const auto values = Numbers(Attribute(event, "sc"), "sc", -1000000, 1000000);
            if (values.size() != 8) Fail("sc must have 8 values for four players");
            for (std::size_t i = 0; i < 4; ++i) state.players[i].score = 100 * (values[i * 2] + values[i * 2 + 1]);
        }

        void Apply(ReplayState &state, const ReplayEvent &event) {
            state.last_event = event.tag;
            if (event.kind == ReplayEventKind::Metadata) {
                if (event.tag == "GO") {
                    state.type = Integer(event, "type", 0, 65535);
                    if (event.attributes.count("red_tiles")) {
                        const auto text = Attribute(event, "red_tiles");
                        state.red_tiles = text.empty() ? std::vector<int>{} : Numbers(text, "red_tiles", 0, 135);
                        if (state.red_tiles.size() > 4) Fail("too many red tiles");
                        for (int t: state.red_tiles) if (t / 4 != 4 && t / 4 != 13 && t / 4 != 22) Fail(
                            "invalid red tile kind");
                    }
                    if ((state.type & 0x810) == 0x810) Fail("Special three-player rules are not restored");
                }
                if (event.tag == "FURITEN") {
                    if (!state.initialized || state.ended) Fail("FURITEN outside a live hand");
                    state.furiten = Integer(event, "show", 0, 1) != 0;
                }
                if (event.tag == "UN")
                    for (std::size_t i = 0; i < 4; ++i) {
                        const auto key = "n" + std::to_string(i);
                        if (event.attributes.count(key)) {
                            state.players[i].name = PercentDecode(Attribute(event, key));
                            state.players[i].connected = true;
                        }
                        if (event.attributes.count("sx")) state.players[i].gender = SoundGender(
                                                              Attribute(event, "sx"), static_cast<int>(i));
                    }
                if (event.tag == "BYE" && event.attributes.count("who"))
                    state.players[Integer(event, "who", 0, 3)].connected = false;
                return;
            }
            if (event.kind == ReplayEventKind::Init) {
                if (state.initialized && !state.ended) Fail("INIT before previous round ended");
                const auto seed = Numbers(Attribute(event, "seed"), "seed", 0, 1000000);
                const auto scores = Numbers(Attribute(event, "ten"), "ten", -1000000, 1000000);
                if (seed.size() != 6 || scores.size() != 4) Fail("INIT needs seed[6] and ten[4]");
                std::array<std::string, 4> names;
                std::array < char, 4 > genders;
                std::array < bool, 4 > connected;
                for (std::size_t i = 0; i < 4; ++i) names[i] = state.players[i].name;
                for (std::size_t i = 0; i < 4; ++i) genders[i] = state.players[i].gender;
                for (int i = 0; i < 4; ++i) connected[i] = state.players[i].connected;
                const auto type = state.type;
                const auto reds = state.red_tiles;
                state = {};
                state.initialized = true;
                state.type = type;
                state.red_tiles = reds;
                state.round = seed[0];
                state.honba = seed[1];
                state.sticks = seed[2];
                state.dealer = Integer(event, "oya", 0, 3);
                state.turn = state.dealer;
                if (type & 16) {
                    state.seat_mask = 0;
                    for (int i = 0; i < 4; ++i) if (!Attribute(event, "hai" + std::to_string(i), false).empty())
                        state.seat_mask |= 1u << i;
                    if (SeatCount(state.seat_mask) != 3 || !SeatPresent(state.seat_mask, state.dealer)) Fail(
                        "Three-player INIT needs three hands and an active dealer");
                }
                state.wall.Init(state.dealer, seed[3], seed[4], type);
                ValidateTile(seed[5]);
                state.dora_indicators.push_back(seed[5]);
                std::array < bool, 136 > used{};
                used[static_cast<std::size_t>(seed[5])] = true;
                for (std::size_t i = 0; i < 4; ++i) {
                    auto &player = state.players[i];
                    player.name = names[i];
                    player.score = scores[i] * 100;
                    player.gender = genders[i];
                    player.connected = connected[i];
                    if (!SeatPresent(state.seat_mask, static_cast<int>(i))) {
                        if (scores[i] != 0 || !names[i].empty()) Fail("Absent seat has a score or name");
                        player.connected = false;
                        continue;
                    }
                    player.hand = Numbers(Attribute(event, "hai" + std::to_string(i)), "hand tile", 0, 135);
                    if (player.hand.size() != 13) Fail("INIT requires 13 concealed tiles per player");
                    for (int tile: player.hand) {
                        if (used[static_cast<std::size_t>(tile)]) Fail(
                            "Duplicate physical tile in INIT: " + std::to_string(tile));
                        used[static_cast<std::size_t>(tile)] = true;
                    }
                    player.deal_hand = player.hand;
                    std::sort(player.hand.begin(), player.hand.end());
                }
                state.last_event = event.tag;
                return;
            }
            if (!state.initialized) Fail("State event before INIT");
            if (state.ended && event.kind != ReplayEventKind::Win) Fail("State event after round end");
            if (event.kind == ReplayEventKind::Dora) {
                const int tile = Integer(event, "hai", 0, 135);
                if (state.dora_indicators.size() >= 5 || VisibleTile(state, tile)) Fail(
                    "Invalid or duplicate dora indicator");
                state.dora_indicators.push_back(tile);
                return;
            }
            if (event.kind == ReplayEventKind::Win || event.kind == ReplayEventKind::DrawEnd) {
                state.settlement.red_tiles = state.red_tiles;
                AppendSettlement(state.settlement, event.tag, event.attributes, state.seat_mask);
                ValidateSettlementDisclosure(state.settlement.receipts.back(), state);
                if (event.kind == ReplayEventKind::Win) {
                    const int who = Integer(event, "who", 0, 3);
                    Integer(event, "fromWho", 0, 3);
                    Integer(event, "machi", 0, 135);
                    if (state.ended && state.result != "AGARI") Fail("AGARI after draw end");
                    if (HasTile(state.winners, who)) Fail("Duplicate winner in round");
                    state.winners.push_back(who);
                    state.result = "AGARI";
                    state.sticks = 0;
                } else {
                    state.result = "RYUUKYOKU";
                    const auto type = Attribute(event, "type", false);
                    if (!type.empty()) state.result += ":" + type;
                }
                Scores(state, event);
                state.ended = true;
                state.turn = -1;
                return;
            }
            const int who = event.who;
            if (!SeatPresent(state.seat_mask, who)) Fail("Invalid or absent actor seat");
            auto &player = state.players[static_cast<std::size_t>(who)];
            if (event.kind == ReplayEventKind::Draw) {
                if (state.turn != who || player.hand.size() % 3 != 1 || player.drawn_tile != -1)
                    Fail("Draw is out of turn or hand phase");
                if (VisibleTile(state, event.tile)) Fail("Duplicate drawn physical tile " + std::to_string(event.tile));
                player.hand.push_back(event.tile);
                std::sort(player.hand.begin(), player.hand.end());
                player.drawn_tile = event.tile;
                state.last_discard_seat = -1;
                state.wall.Draw();
            } else if (event.kind == ReplayEventKind::Discard) {
                if (state.turn != who || player.hand.size() % 3 != 2) Fail("Discard is out of turn or hand phase");
                const bool riichi = player.reach == 1 && std::none_of(player.discards.begin(), player.discards.end(),
                                                                      [](const ReplayDiscard &discard) {
                                                                          return discard.riichi;
                                                                      });
                ReplayDiscard discard{event.tile, false, riichi, player.drawn_tile == event.tile};
                RemoveTile(player, event.tile);
                player.discards.push_back(discard);
                player.drawn_tile = -1;
                state.last_discard_seat = who;
                state.turn = NextSeat(state.seat_mask, who);
            } else if (event.kind == ReplayEventKind::Reach) {
                const int step = Integer(event, "step", 1, 2);
                if (step == 1) {
                    if (state.turn != who || player.reach != 0 || player.hand.size() % 3 != 2) Fail(
                        "Invalid REACH declaration");
                    player.reach = 1;
                } else {
                    if (player.reach != 1 || player.discards.empty() || !player.discards.back().riichi) Fail(
                        "REACH acceptance without declaration discard");
                    player.reach = 2;
                    player.score -= 1000;
                    ++state.sticks;
                    if (event.attributes.count("ten")) {
                        const auto scores = Numbers(Attribute(event, "ten"), "REACH ten", -1000000, 1000000);
                        if (scores.size() != 4) Fail("REACH ten must have four values");
                        for (std::size_t i = 0; i < 4; ++i) state.players[i].score = scores[i] * 100;
                    }
                }
            } else if (event.kind == ReplayEventKind::Meld) {
                auto meld = DecodeReplayMeld(static_cast<std::uint32_t>(Integer(event, "m", 0, 65535)), who);
                if (meld.kind == MeldKind::Nuki) {
                    if (!(state.type & 16) || meld.tiles.front() / 4 != 30 || state.turn != who || player.hand.size() %
                        3 != 2)
                        Fail("Invalid north extraction mode, entity or phase");
                    RemoveTile(player, meld.tiles.front());
                    player.nuki.push_back(meld.tiles.front());
                    player.drawn_tile = -1;
                    state.last_discard_seat = -1;
                    state.wall.Kan();
                    return;
                }
                if (!SeatPresent(state.seat_mask, meld.from) || ((state.type & 16) && meld.kind == MeldKind::Chi)) Fail(
                    "Invalid three-player meld source/kind");
                if (meld.kind == MeldKind::OpenKan || meld.kind == MeldKind::ClosedKan || meld.kind ==
                    MeldKind::AddedKan) state.wall.Kan();
                if (meld.kind == MeldKind::AddedKan) {
                    if (state.turn != who || player.hand.size() % 3 != 2) Fail("Added kan is out of turn");
                    auto pon = std::find_if(player.melds.begin(), player.melds.end(), [&](const ReplayMeld &old) {
                        return old.kind == MeldKind::Pon && old.tiles.front() / 4 == meld.tiles.front() / 4;
                    });
                    if (pon == player.melds.end()) Fail("Added kan requires an existing pon");
                    if (pon->from != meld.from || HasTile(pon->tiles, meld.called_tile)) Fail(
                        "Added kan source or added tile disagrees with existing pon");
                    int added = -1;
                    for (int tile: meld.tiles) if (!HasTile(pon->tiles, tile)) added = tile;
                    RemoveTile(player, added);
                    pon->kind = MeldKind::AddedKan;
                    pon->tiles = meld.tiles;
                    pon->encoded = meld.encoded;
                } else {
                    if (meld.kind == MeldKind::ClosedKan) {
                        if (state.turn != who || player.hand.size() % 3 != 2) Fail("Closed kan is out of turn");
                    } else {
                        if (state.last_discard_seat != meld.from || meld.from == who) Fail(
                            "Meld source is not the latest discarder");
                        auto &river = state.players[static_cast<std::size_t>(meld.from)].discards;
                        if (river.empty() || river.back().claimed || river.back().tile != meld.called_tile) Fail(
                            "Meld tile does not match latest discard");
                        if (player.hand.size() % 3 != 1) Fail("Meld is in an invalid hand phase");
                        river.back().claimed = true;
                    }
                    for (int tile: meld.tiles) if (tile != meld.called_tile || meld.kind == MeldKind::ClosedKan)
                        RemoveTile(player, tile);
                    player.melds.push_back(meld);
                }
                player.drawn_tile = -1;
                state.turn = who;
                state.last_discard_seat = -1;
            }
        }

        std::string Quote(const std::string &text) {
            std::string result = "\"";
            const char *hex = "0123456789abcdef";
            for (unsigned char byte: text) {
                if (byte == '"' || byte == '\\') {
                    result += '\\';
                    result += static_cast<char>(byte);
                } else if (byte < 32) {
                    result += "\\u00";
                    result += hex[byte >> 4];
                    result += hex[byte & 15];
                } else result += static_cast<char>(byte);
            }
            return result + '"';
        }

        void Array(std::ostream &out, const std::vector<int> &values) {
            out << '[';
            bool comma = false;
            for (int value: values) {
                if (comma) out << ',';
                out << value;
                comma = true;
            }
            out << ']';
        }

        std::string InflateGzip(const std::string &bytes) {
            z_stream stream{};
            stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(bytes.data()));
            stream.avail_in = static_cast<uInt>(bytes.size());
            if (inflateInit2(&stream, 16 + MAX_WBITS) != Z_OK) Fail("Cannot initialize gzip decoder");
            std::string decoded;
            int status = Z_OK;
            std::array < char, 32768 > chunk{};
            do {
                stream.next_out = reinterpret_cast<Bytef *>(chunk.data());
                stream.avail_out = static_cast<uInt>(chunk.size());
                status = inflate(&stream, Z_NO_FLUSH);
                decoded.append(chunk.data(), chunk.size() - stream.avail_out);
                if (decoded.size() > kMaximumBytes) {
                    inflateEnd(&stream);
                    Fail("Decompressed replay exceeds 16 MiB limit");
                }
            } while (status == Z_OK);
            const auto remaining = stream.avail_in;
            inflateEnd(&stream);
            if (status != Z_STREAM_END || remaining != 0) Fail("Invalid, truncated, or concatenated gzip replay");
            return decoded;
        }
    } // namespace

    char SoundGender(const std::string &seats, int who) {
        // UN sx is normally comma separated; tolerate a compact four-seat string.
        if (who < 0 || who > 3) return '?';
        std::string value;
        if (seats.find(',') == std::string::npos) {
            if (seats.size() == 4) value = seats.substr(who, 1);
        } else {
            std::size_t start = 0;
            for (int seat = 0; seat <= who; ++seat) {
                const auto end = seats.find(',', start);
                if (seat == who) {
                    value = seats.substr(start, end == std::string::npos ? end : end - start);
                    break;
                }
                if (end == std::string::npos) return '?';
                start = end + 1;
            }
        }
        return value == "M" ? 'M' : value == "F" ? 'F' : '?';
    }

    ReplayMeld DecodeReplayMeld(std::uint32_t encoded, int who) {
        if (who < 0 || who > 3 || encoded > 65535) Fail("Invalid meld actor or 16-bit encoding");
        ReplayMeld result;
        result.encoded = encoded;
        const int relative = static_cast<int>(encoded & 3);
        result.from = (who + relative) % 4;
        if (encoded & 4) {
            if (relative != 3) Fail("Chi must be from the previous seat");
            const int packed = static_cast<int>(encoded >> 10);
            const int called = packed % 3;
            const int base = (packed / 3 / 7) * 9 + packed / 3 % 7;
            if (base >= 27) Fail("Invalid chi base tile");
            result.kind = MeldKind::Chi;
            result.tiles = {
                base * 4 + static_cast<int>((encoded >> 3) & 3),
                (base + 1) * 4 + static_cast<int>((encoded >> 5) & 3),
                (base + 2) * 4 + static_cast<int>((encoded >> 7) & 3)
            };
            result.called_tile = result.tiles[static_cast<std::size_t>(called)];
        } else if (encoded & 0x18) {
            if (relative == 0 || (encoded & 0x18) == 0x18) Fail("Invalid pon/added-kan encoding");
            const int packed = static_cast<int>(encoded >> 9);
            const int called = packed % 3;
            const int base = packed / 3 * 4;
            const int excluded = static_cast<int>((encoded >> 5) & 3);
            for (int copy = 0; copy < 4; ++copy) if (copy != excluded) result.tiles.push_back(base + copy);
            result.called_tile = result.tiles[static_cast<std::size_t>(called)];
            if (encoded & 8) result.kind = MeldKind::Pon;
            else {
                result.kind = MeldKind::AddedKan;
                result.called_tile = base + excluded;
                result.tiles.push_back(base + excluded);
            }
        } else if (encoded & 0x20) {
            // FUN_0044d8e0: no low nibble, 0x20 set, entity is high byte.
            if (relative) Fail("North extraction cannot have a source seat");
            result.kind = MeldKind::Nuki;
            result.from = who;
            result.tiles = {static_cast<int>(encoded >> 8)};
            result.called_tile = result.tiles.front();
        } else {
            const int tile = static_cast<int>(encoded >> 8);
            ValidateTile(tile);
            const int base = tile / 4 * 4;
            result.tiles = {base, base + 1, base + 2, base + 3};
            result.kind = relative == 0 ? MeldKind::ClosedKan : MeldKind::OpenKan;
            result.called_tile = relative == 0 ? -1 : tile;
        }
        for (int tile: result.tiles) ValidateTile(tile);
        std::sort(result.tiles.begin(), result.tiles.end());
        return result;
    }

    ReplayLog ReplayLog::Load(const std::filesystem::path &path) {
        std::ifstream input(path, std::ios::binary);
        if (!input) Fail("Cannot open replay: " + path.u8string());
        input.seekg(0, std::ios::end);
        const auto size = input.tellg();
        if (size <= 0 || size > static_cast<std::streamoff>(kMaximumBytes)) Fail(
            "Replay file is empty or exceeds 16 MiB limit");
        input.seekg(0);
        std::string bytes(static_cast<std::size_t>(size), '\0');
        if (!input.read(bytes.data(), size)) Fail("Cannot read replay: " + path.u8string());
        if (bytes.size() >= 2 && static_cast<unsigned char>(bytes[0]) == 0x1f && static_cast<unsigned char>(bytes[1]) ==
            0x8b)
            bytes = InflateGzip(bytes);
        return Parse(bytes, path.u8string());
    }

    ReplayLog ReplayLog::Parse(const std::string &xml, const std::string &source) {
        if (xml.empty() || xml.size() > kMaximumBytes || xml.find('\0') != std::string::npos) Fail(
            "Empty, oversized, or non-UTF8 replay XML");
        ValidateUtf8(xml, "replay XML");
        XmlReader reader(xml);
        Tag tag;
        if (!reader.Next(tag) || tag.event.tag != "mjloggm" || tag.closing || tag.self_closing || Attribute(
                tag.event, "ver") != "2.3")
            Fail("Expected mjloggm root with ver=2.3");
        ReplayLog result;
        result.source = source;
        bool closed = false, init = false;
        while (reader.Next(tag)) {
            auto &event = tag.event;
            if (tag.closing && event.tag == "mjloggm") {
                closed = true;
                break;
            }
            if (!tag.self_closing || tag.closing) Fail("Replay events must be self-closing XML elements");
            if (event.tag == "GO" || event.tag == "UN" || event.tag == "SHUFFLE" || event.tag == "TAIKYOKU" || event.tag
                == "BYE" || event.tag == "FURITEN")
                event.kind = ReplayEventKind::Metadata;
            else if (event.tag == "INIT") {
                event.kind = ReplayEventKind::Init;
                init = true;
            } else if (event.tag == "N") {
                event.kind = ReplayEventKind::Meld;
                event.who = Integer(event, "who", 0, 3);
            } else if (event.tag == "REACH") {
                event.kind = ReplayEventKind::Reach;
                event.who = Integer(event, "who", 0, 3);
            } else if (event.tag == "DORA") event.kind = ReplayEventKind::Dora;
            else if (event.tag == "AGARI") {
                event.kind = ReplayEventKind::Win;
                event.who = Integer(event, "who", 0, 3);
            } else if (event.tag == "RYUUKYOKU") event.kind = ReplayEventKind::DrawEnd;
            else if (event.tag.size() > 1 && (std::string("TUVWDEFGtuvwdefg").find(event.tag[0]) !=
                                              std::string::npos)) {
                const char c = event.tag[0] >= 'a' && event.tag[0] <= 'z'
                                   ? static_cast<char>(event.tag[0] - 'a' + 'A')
                                   : event.tag[0];
                const auto draw = std::string("TUVW").find(c);
                const auto discard = std::string("DEFG").find(c);
                event.kind = draw != std::string::npos ? ReplayEventKind::Draw : ReplayEventKind::Discard;
                event.who = static_cast<int>(draw != std::string::npos ? draw : discard);
                event.tile = Number(event.tag.substr(1), "tile tag", 0, 135);
                if (!event.attributes.empty()) Fail("Tile event attributes are unsupported");
            } else Fail("Unsupported replay tag: " + event.tag + " at byte " + std::to_string(event.source_offset));
            result.events.push_back(event);
            if (result.events.size() > kMaximumEvents) Fail("Replay exceeds 100000 event limit");
        }
        if (!closed || reader.Next(tag)) Fail("Missing root close or trailing XML content");
        if (!init) Fail("Replay has no INIT round");
        ReplaySession validate(result);
        while (validate.Step()) {
        } // Fail before exposing a partially invalid log.
        return result;
    }

    ReplaySession::ReplaySession(ReplayLog log) : log_(std::move(log)) {
    }

    bool ReplaySession::Step() {
        if (position_ == log_.events.size()) return false;
        auto next = state_;
        const auto &event = log_.events[position_];
        try { Apply(next, event); } catch (const std::exception &error) {
            Fail("Event " + std::to_string(position_) + " <" + event.tag + "> at byte " +
                 std::to_string(event.source_offset) + ": " + error.what());
        }
        state_ = std::move(next);
        ++position_;
        return true;
    }

    void ReplaySession::Reset() {
        state_ = {};
        position_ = 0;
    }

    void ReplaySession::Seek(std::size_t applied_events) {
        if (applied_events > log_.events.size()) Fail("Replay seek exceeds event count");
        ReplaySession next(log_);
        while (next.position() < applied_events) next.Step();
        state_ = std::move(next.state_);
        position_ = next.position_;
    }

    std::string TileLabel(int tile) {
        if (tile < 0 || tile >= 136) return "?";
        const int type = tile / 4;
        if (type < 27)
            return std::to_string(type % 9 + 1) + "mps"[type / 9] +
                   ((tile == 16 || tile == 52 || tile == 88) ? "*" : "");
        const char *honors[] = {"East", "South", "West", "North", "White", "Green", "Red"};
        return honors[type - 27];
    }

    const char *MeldKindName(MeldKind kind) {
        switch (kind) {
            case MeldKind::Chi: return "chi";
            case MeldKind::Pon: return "pon";
            case MeldKind::OpenKan: return "open_kan";
            case MeldKind::ClosedKan: return "closed_kan";
            case MeldKind::AddedKan: return "added_kan";
            case MeldKind::Nuki: return "nuki";
        }
        return "unknown";
    }

    std::string ReplayStateJson(const ReplayState &state) {
        std::ostringstream out;
        out << "{\"initialized\":" << (state.initialized ? "true" : "false") << ",\"ended\":" << (
                    state.ended ? "true" : "false")
                << ",\"type\":" << state.type << ",\"seat_mask\":" << state.seat_mask << ",\"furiten\":" << (
                    state.furiten ? "true" : "false")
                << ",\"round\":" << state.round << ",\"honba\":" << state.honba << ",\"sticks\":" << state.sticks
                << ",\"dealer\":" << state.dealer << ",\"turn\":" << state.turn << ",\"result\":" << Quote(state.result)
                << ",\"last_event\":" << Quote(state.last_event) << ",\"dora_indicators\":";
        Array(out, state.dora_indicators);
        out << ",\"winners\":";
        Array(out, state.winners);
        out << ",\"players\":[";
        for (std::size_t i = 0; i < 4; ++i) {
            if (i) out << ',';
            const auto &player = state.players[i];
            out << "{\"name\":" << Quote(player.name) << ",\"score\":" << player.score << ",\"reach\":" << player.reach
                    << ",\"connected\":" << (player.connected ? "true" : "false") << ",\"drawn_tile\":" << player.
                    drawn_tile << ",\"hand\":";
            Array(out, player.hand);
            out << ",\"nuki\":";
            Array(out, player.nuki);
            out << ",\"discards\":[";
            for (std::size_t j = 0; j < player.discards.size(); ++j) {
                if (j) out << ',';
                const auto &discard = player.discards[j];
                out << "{\"tile\":" << discard.tile << ",\"claimed\":" << (discard.claimed ? "true" : "false")
                        << ",\"riichi\":" << (discard.riichi ? "true" : "false") << ",\"tsumogiri\":" << (
                            discard.tsumogiri ? "true" : "false") << '}';
            }
            out << "],\"melds\":[";
            for (std::size_t j = 0; j < player.melds.size(); ++j) {
                if (j) out << ',';
                const auto &meld = player.melds[j];
                out << "{\"kind\":" << Quote(MeldKindName(meld.kind)) << ",\"tiles\":";
                Array(out, meld.tiles);
                out << ",\"called_tile\":" << meld.called_tile << ",\"from\":" << meld.from << ",\"encoded\":" << meld.
                        encoded << '}';
            }
            out << "]}";
        }
        out << "]}";
        return out.str();
    }
} // namespace tenhou
