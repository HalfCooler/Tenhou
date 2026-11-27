#include "room_settings_view_model.hpp"
#include <algorithm>
#include <charconv>

namespace tenhou {
    bool RoomSettingsViewModel::Initialize(const std::map<std::string, ServerRule> &rules,
                                           const std::map<std::string, std::string> &initial_settings) {
        available_.clear();
        values_ = initial_settings;
        selected_ = 0;
        page_ = 0;
        error_.clear();

        for (const auto &entry: rules) {
            if (entry.second.presentation != "riichi-v1" && entry.second.presentation != "mahjong-v1") continue;
            available_.push_back(&entry.second);
            if (values_.count("rule") && values_.at("rule") == entry.first)
                selected_ = available_.size() - 1;
        }
        if (available_.empty()) return false;

        const auto &schema = available_[selected_]->fields;
        for (auto it = values_.begin(); it != values_.end();) {
            if (it->first != "rule" &&
                std::none_of(schema.begin(), schema.end(), [&](const RuleField &f) { return f.key == it->first; })) {
                it = values_.erase(it);
            } else {
                ++it;
            }
        }
        values_["rule"] = available_[selected_]->id;
        return true;
    }

    bool RoomSettingsViewModel::SelectRule(const std::size_t rule_index) {
        if (rule_index >= available_.size()) return false;
        selected_ = rule_index;
        values_ = {{"rule", available_[selected_]->id}};
        page_ = 0;
        error_.clear();
        return true;
    }

    std::size_t RoomSettingsViewModel::PageEnd() const {
        if (available_.empty()) return 0;
        return std::min(available_[selected_]->fields.size(), (page_ + 1) * kPageSize);
    }

    bool RoomSettingsViewModel::CanNextPage() const {
        if (available_.empty()) return false;
        return (page_ + 1) * kPageSize < available_[selected_]->fields.size();
    }

    bool RoomSettingsViewModel::PrevPage() {
        if (!CanPrevPage()) return false;
        --page_;
        return true;
    }

    bool RoomSettingsViewModel::NextPage() {
        if (!CanNextPage()) return false;
        ++page_;
        return true;
    }

    std::string RoomSettingsViewModel::CurrentFieldValue(const std::size_t field_index) const {
        const auto &field = available_.at(selected_)->fields.at(field_index);
        const auto found = values_.find(field.key);
        return found == values_.end() ? field.initial : found->second;
    }

    bool RoomSettingsViewModel::SetChoiceField(const std::size_t field_index, const std::size_t choice_index) {
        const auto &field = available_.at(selected_)->fields.at(field_index);
        if (choice_index >= field.choices.size()) {
            error_ = field.label + "：请选择有效的选项";
            return false;
        }
        values_[field.key] = field.choices[choice_index].value;
        error_.clear();
        return true;
    }

    bool RoomSettingsViewModel::SetTextField(const std::size_t field_index, const std::string &raw_value) {
        const auto &field = available_.at(selected_)->fields.at(field_index);
        if (!field.minimum.empty() || !field.maximum.empty()) {
            int number = 0;
            const auto parsed = std::from_chars(raw_value.data(), raw_value.data() + raw_value.size(), number);
            const int low = field.minimum.empty() ? 0 : std::stoi(field.minimum);
            const int high = field.maximum.empty() ? 100000000 : std::stoi(field.maximum);
            if (parsed.ec != std::errc{} || parsed.ptr != raw_value.data() + raw_value.size() ||
                number < low || number > high ||
                (field.step && (number - low) % static_cast<int>(field.step))) {
                error_ = field.label + "：请输入服务器允许的数值";
                return false;
            }
        }
        values_[field.key] = raw_value;
        error_.clear();
        return true;
    }
} // namespace tenhou
