#include "http.hh"

const std::unordered_set<std::string_view> HttpRequest::default_html {
    "/index",
    // "/register",
    // "/login",
    "/welcome",
    "/video",
    "/picture",
};

void HttpRequest::Clear()
{
    raw_data_.clear();
    state_ = ParseState::REQUEST_LINE;
    path_.clear();
    header_.clear();
}

auto HttpRequest::IsKeepAlive() const -> bool
{
    if (version_ != "HTTP/1.1") return false;
    if (auto find = header_.find("Connection"); find != header_.end()) {
        return find->second == "keep-alive";
    }
    return false;
}

auto HttpRequest::Parse_() -> bool
{
    auto lines = SliceLines_(raw_data_.view());
    for (auto& line : lines) {
        switch (state_) {
        case ParseState::REQUEST_LINE:
            if (!ParseRequestLine_(line))
                return false;
            ParsePath_();
            break;
        case ParseState::HEADERS:
            ParseHeader_(line);
            break;
        case ParseState::BODY:
            ParseBody_(line);
            break;
        default:
            break;
        }
        if (state_ == ParseState::FINISH)
            break;
    }
    return true;
}

auto HttpRequest::SliceLines_(std::string_view view) -> std::vector<std::string_view>
{
    constexpr std::string_view CRLF {"\r\n"};

    std::vector<std::string_view> lines;
    while (true) {
        if (view.starts_with(CRLF)) {
            lines.emplace_back("");
            view.remove_prefix(CRLF.size());
            continue;
        }
        auto found = std::ranges::search(view, CRLF);
        if (found.empty()) {
            if (!view.empty())
                lines.emplace_back(view);
            break;
        }
        size_t len = std::distance(view.begin(), found.begin());
        if (len != 0)
            lines.emplace_back(view.begin(), len);
        view.remove_prefix(len + CRLF.size());
    }
    return lines;
}

auto HttpRequest::ParseRequestLine_(std::string_view& line) -> bool
{
    do {
        size_t pos;
        if ((pos = line.find(' ')) == std::string_view::npos)
            break;
        method_ = line.substr(0, pos++);
        line.remove_prefix(pos);

        pos = line.find(' ');
        if ((pos = line.find(' ')) == std::string_view::npos)
            break;
        path_ = line.substr(0, pos++);
        line.remove_prefix(pos);

        version_ = line;

        state_ = ParseState::HEADERS;

        LOG_DEBUG("[method: " + std::string(method_) + "] " +
                  "[path: " + path_ + "] " +
                  "[version: " + std::string(version_) + "] ");
        return true;

    } while (0);

    LOG_ERROR("fail to parse request line:" + std::string(line));
    return false;
}

void HttpRequest::ParseHeader_(std::string_view& line)
{
    size_t pos = line.find(':');
    if (pos == std::string_view::npos) {
        state_ = ParseState::BODY;
        return;
    }

    auto key = line.substr(0, pos++);
    assert(pos <= line.size());
    auto value = (pos == line.size() || line[pos] != ' ')
                   ? line.substr(pos)
                   : line.substr(pos + 1);
    header_[key] = value;
}

void HttpRequest::ParseBody_(std::string_view& line)
{
    body_ = line;
    ParsePost_();
    state_ = ParseState::FINISH;
    LOG_DEBUG("BODY: " + std::string(body_));
}

void HttpRequest::ParsePath_()
{
    if (path_ == "/")
        path_ = "/index.html";
    else if (default_html.contains(path_)) {
        path_ += ".html";
    }
}
