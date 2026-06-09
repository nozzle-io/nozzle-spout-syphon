#include <app/source_selection.hpp>

#include <algorithm>
#include <sstream>

namespace nozzle_spout_syphon {

namespace {

bool has_prefix(const std::string &value, const char *prefix) {
    const std::string prefix_text{prefix};
    return value.size() >= prefix_text.size() && value.compare(0, prefix_text.size(), prefix_text) == 0;
}

bool source_less(const external_source_info &lhs, const external_source_info &rhs) {
    if (lhs.server_name != rhs.server_name) {
        return lhs.server_name < rhs.server_name;
    }
    if (lhs.app_name != rhs.app_name) {
        return lhs.app_name < rhs.app_name;
    }
    if (lhs.stable_id != rhs.stable_id) {
        return lhs.stable_id < rhs.stable_id;
    }
    return lhs.platform_index < rhs.platform_index;
}

bool selector_matches(const source_selector &selector, const external_source_info &source) {
    switch (selector.kind) {
    case source_selector_kind::default_source:
        return true;
    case source_selector_kind::name:
        return source.server_name == selector.value;
    case source_selector_kind::app:
        return source.app_name == selector.value;
    }
    return false;
}

std::string selector_text(const source_selector &selector) {
    switch (selector.kind) {
    case source_selector_kind::default_source: return "default";
    case source_selector_kind::name: return "name:" + selector.value;
    case source_selector_kind::app: return "app:" + selector.value;
    }
    return selector.raw;
}

std::string quote_value(const std::string &value) {
    std::string quoted{"\""};
    for (char character : value) {
        if (character == '\\' || character == '"') {
            quoted.push_back('\\');
            quoted.push_back(character);
        } else if (character == '\n') {
            quoted += "\\n";
        } else if (character == '\t') {
            quoted += "\\t";
        } else if (character == '\r') {
            quoted += "\\r";
        } else {
            quoted.push_back(character);
        }
    }
    quoted.push_back('"');
    return quoted;
}

std::size_t count_selector_matches(
    const std::vector<external_source_info> &sources,
    source_selector_kind kind,
    const std::string &value
) {
    if (value.empty()) {
        return 0;
    }
    source_selector selector{};
    selector.kind = kind;
    selector.value = value;
    std::size_t count = 0;
    for (const external_source_info &source : sources) {
        if (selector_matches(selector, source)) {
            count = count + 1;
        }
    }
    return count;
}

} // namespace

const char *external_source_backend_name(external_source_backend backend) noexcept {
    switch (backend) {
    case external_source_backend::syphon: return "Syphon";
    case external_source_backend::spout: return "Spout";
    }
    return "external";
}

const char *source_selection_status_name(source_selection_status status) noexcept {
    switch (status) {
    case source_selection_status::selected: return "selected";
    case source_selection_status::not_found: return "not_found";
    case source_selection_status::ambiguous: return "ambiguous";
    case source_selection_status::unsupported_selector: return "unsupported_selector";
    }
    return "unknown";
}

source_selector parse_source_selector(const std::string &raw_selector) {
    source_selector selector{};
    selector.raw = raw_selector.empty() ? "default" : raw_selector;
    if (raw_selector.empty() || raw_selector == "default") {
        selector.kind = source_selector_kind::default_source;
        selector.value.clear();
        return selector;
    }
    if (has_prefix(raw_selector, "name:")) {
        selector.kind = source_selector_kind::name;
        selector.value = raw_selector.substr(5);
        selector.unsupported = selector.value.empty();
        return selector;
    }
    if (has_prefix(raw_selector, "app:")) {
        selector.kind = source_selector_kind::app;
        selector.value = raw_selector.substr(4);
        selector.unsupported = selector.value.empty();
        return selector;
    }

    const std::size_t colon = raw_selector.find(':');
    if (colon != std::string::npos) {
        selector.kind = source_selector_kind::name;
        selector.value = raw_selector;
        selector.unsupported = true;
        return selector;
    }

    selector.kind = source_selector_kind::name;
    selector.value = raw_selector;
    return selector;
}

bool backend_supports_selector(external_source_backend backend, source_selector_kind kind) noexcept {
    if (kind == source_selector_kind::default_source || kind == source_selector_kind::name) {
        return true;
    }
    if (kind == source_selector_kind::app) {
        return backend == external_source_backend::syphon;
    }
    return false;
}

source_selection_result select_external_source(
    external_source_backend backend,
    const std::string &raw_selector,
    const std::vector<external_source_info> &sources
) {
    source_selection_result result{};
    result.selector = parse_source_selector(raw_selector);

    if (result.selector.unsupported) {
        result.status = source_selection_status::unsupported_selector;
        result.message = std::string(external_source_backend_name(backend)) + " does not support selector " + result.selector.raw;
        return result;
    }

    if (!backend_supports_selector(backend, result.selector.kind)) {
        result.status = source_selection_status::unsupported_selector;
        result.message = std::string(external_source_backend_name(backend)) + " does not support selector " + selector_text(result.selector);
        return result;
    }

    if (result.selector.kind == source_selector_kind::default_source) {
        if (sources.empty()) {
            result.status = source_selection_status::not_found;
            result.message = std::string(external_source_backend_name(backend)) + " source not found for selector default";
            return result;
        }
        std::vector<std::size_t> sorted_indices{};
        sorted_indices.reserve(sources.size());
        for (std::size_t index = 0; index < sources.size(); ++index) {
            sorted_indices.push_back(index);
        }
        std::sort(sorted_indices.begin(), sorted_indices.end(), [&sources](std::size_t lhs, std::size_t rhs) {
            return source_less(sources[lhs], sources[rhs]);
        });
        result.status = source_selection_status::selected;
        result.selected_index = sorted_indices.front();
        result.matching_indices = sorted_indices;
        result.message = std::string("default selected first sorted ") + external_source_backend_name(backend) + " source";
        return result;
    }

    for (std::size_t index = 0; index < sources.size(); ++index) {
        if (selector_matches(result.selector, sources[index])) {
            result.matching_indices.push_back(index);
        }
    }

    if (result.matching_indices.empty()) {
        result.status = source_selection_status::not_found;
        result.message = std::string(external_source_backend_name(backend)) + " source not found for selector " + selector_text(result.selector);
        return result;
    }
    if (result.matching_indices.size() > 1) {
        result.status = source_selection_status::ambiguous;
        result.message = std::string(external_source_backend_name(backend)) + " selector is ambiguous: " + selector_text(result.selector);
        return result;
    }

    result.status = source_selection_status::selected;
    result.selected_index = result.matching_indices.front();
    result.message = std::string(external_source_backend_name(backend)) + " source selected by " + selector_text(result.selector);
    return result;
}

std::string format_source_candidate(const external_source_info &source) {
    std::ostringstream out;
    out << external_source_backend_name(source.backend);
    if (!source.app_name.empty()) {
        out << " app=" << quote_value(source.app_name);
    }
    if (!source.server_name.empty()) {
        out << " name=" << quote_value(source.server_name);
    }
    if (!source.stable_id.empty()) {
        out << " id=" << quote_value(source.stable_id);
    }
    out << " index=" << source.platform_index;
    return out.str();
}

std::string format_source_selection_error(const source_selection_result &result, const std::vector<external_source_info> &sources) {
    std::ostringstream out;
    out << result.message;
    if (!result.matching_indices.empty()) {
        out << "; candidates:";
        for (std::size_t index : result.matching_indices) {
            if (index < sources.size()) {
                out << " [" << format_source_candidate(sources[index]) << "]";
            }
        }
    }
    return out.str();
}

std::string format_external_source_list_line(
    const external_source_info &source,
    const std::vector<external_source_info> &sources
) {
    std::ostringstream out;
    out << format_source_candidate(source);
    if (!source.server_name.empty()) {
        out << " selector=" << quote_value("name:" + source.server_name);
        out << " name_ambiguous=" << (count_selector_matches(sources, source_selector_kind::name, source.server_name) > 1 ? "yes" : "no");
    }
    if (!source.app_name.empty() && backend_supports_selector(source.backend, source_selector_kind::app)) {
        out << " selector=" << quote_value("app:" + source.app_name);
        out << " app_ambiguous=" << (count_selector_matches(sources, source_selector_kind::app, source.app_name) > 1 ? "yes" : "no");
    }
    return out.str();
}

} // namespace nozzle_spout_syphon
