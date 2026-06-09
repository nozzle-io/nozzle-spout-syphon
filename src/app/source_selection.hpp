#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace nozzle_spout_syphon {

enum class external_source_backend {
    syphon,
    spout,
};

enum class source_selector_kind {
    default_source,
    name,
    app,
};

enum class source_selection_status {
    selected,
    not_found,
    ambiguous,
    unsupported_selector,
};

struct external_source_info {
    external_source_backend backend{external_source_backend::syphon};
    std::string display_name{};
    std::string server_name{};
    std::string app_name{};
    std::string stable_id{};
    std::size_t platform_index{0};
};

struct source_selector {
    source_selector_kind kind{source_selector_kind::default_source};
    std::string value{};
    std::string raw{"default"};
    bool unsupported{false};
};

struct source_selection_result {
    source_selection_status status{source_selection_status::not_found};
    source_selector selector{};
    std::size_t selected_index{0};
    std::vector<std::size_t> matching_indices{};
    std::string message{};
};

const char *external_source_backend_name(external_source_backend backend) noexcept;
const char *source_selection_status_name(source_selection_status status) noexcept;
source_selector parse_source_selector(const std::string &raw_selector);
bool backend_supports_selector(external_source_backend backend, source_selector_kind kind) noexcept;
source_selection_result select_external_source(
    external_source_backend backend,
    const std::string &raw_selector,
    const std::vector<external_source_info> &sources
);
std::string format_source_candidate(const external_source_info &source);
std::string format_source_selection_error(const source_selection_result &result, const std::vector<external_source_info> &sources);
std::string format_external_source_list_line(
    const external_source_info &source,
    const std::vector<external_source_info> &sources
);

} // namespace nozzle_spout_syphon
