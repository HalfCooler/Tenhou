#pragma once
#include "matchmaking.hpp"
#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace tenhou {
    // Pure C++ ViewModel for friend-room rule editing (zero Win32/HWND dependencies).
    class RoomSettingsViewModel {
    public:
        static constexpr std::size_t kPageSize = 8;

        bool Initialize(const std::map<std::string, ServerRule> &rules,
                        const std::map<std::string, std::string> &initial_settings);

        bool SelectRule(std::size_t rule_index);

        std::size_t RuleCount() const { return available_.size(); }
        std::size_t selected_rule() const { return selected_; }
        const ServerRule &CurrentRule() const { return *available_.at(selected_); }
        const std::vector<const ServerRule *> &AvailableRules() const { return available_; }

        std::size_t page() const { return page_; }
        std::size_t PageBegin() const { return page_ * kPageSize; }
        std::size_t PageEnd() const;
        bool CanPrevPage() const { return page_ > 0; }
        bool CanNextPage() const;
        bool PrevPage();
        bool NextPage();

        std::string CurrentFieldValue(std::size_t field_index) const;
        bool SetChoiceField(std::size_t field_index, std::size_t choice_index);
        bool SetTextField(std::size_t field_index, const std::string &raw_value);

        const std::map<std::string, std::string> &values() const { return values_; }
        const std::string &error() const { return error_; }

    private:
        std::vector<const ServerRule *> available_;
        std::map<std::string, std::string> values_;
        std::size_t selected_ = 0;
        std::size_t page_ = 0;
        std::string error_;
    };
} // namespace tenhou
