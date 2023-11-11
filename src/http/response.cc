#include "http.hh"

const std::unordered_map<std::string_view, std::string_view> HttpResponse::suffix_type = {
    {".html",  "text/html"            },
    {".xml",   "text/xml"             },
    {".xhtml", "application/xhtml+xml"},
    {".txt",   "text/plain"           },
    {".rtf",   "application/rtf"      },
    {".pdf",   "application/pdf"      },
    {".word",  "application/nsword"   },
    {".png",   "image/png"            },
    {".gif",   "image/gif"            },
    {".jpg",   "image/jpeg"           },
    {".jpeg",  "image/jpeg"           },
    {".au",    "audio/basic"          },
    {".mpeg",  "video/mpeg"           },
    {".mpg",   "video/mpeg"           },
    {".avi",   "video/x-msvideo"      },
    {".gz",    "application/x-gzip"   },
    {".tar",   "application/x-tar"    },
    {".css",   "text/css "            },
    {".js",    "text/javascript "     },
};

const std::unordered_map<int, std::string_view> HttpResponse::code_path = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

void HttpResponse::Init(std::string_view base, std::string_view path, HttpCode code, bool keep_alive)
{
    full_path_ = base;
    full_path_ /= std::filesystem::path(path).relative_path();

    code_ = code;
    keep_alive_ = keep_alive;
}

void HttpResponse::Compose()
{
    res_lst_.clear();
    slurp_ = Slurp(full_path_.native());

    ComposeCode_();
    Redirect_();
    ComposeState_();
    ComposeHeader_();
    ComposeContent_();

    size_t size = 0;
    for (auto& sv : res_lst_) size += sv.size();
    response_.reserve(size + 1);
    for (auto& sv : res_lst_) response_.append(sv);
}

void HttpResponse::ComposeCode_()
{
    if (slurp_.error_message()) {
        LOG_INFO(std::string(slurp_.state_message()) + ": " + slurp_.error_message().value());
        if (slurp_.state() <= Slurp::State::OPEN) {
            code_ = HttpCode::Not_Found;
        } else if (slurp_.state() <= Slurp::State::READ) {
            code_ = HttpCode::Forbidden;
        }
    } else if (code_ == HttpCode::Unknown) {
        code_ = HttpCode::OK;
    }
}

void HttpResponse::Redirect_()
{
    if (auto find = code_path.find(code_); find != code_path.end()) {
        slurp_ = Slurp(find->second);
        if (slurp_.error_message()) {
            LOG_ERROR(std::string(slurp_.state_message()) +
                      ": " + slurp_.error_message().value() +
                      " (" + std::string(find->second) + ")");
        }
    }
}

void HttpResponse::ComposeState_()
{
    temp_.emplace_back(std::to_string(int(code_)));
    res_lst_.insert(res_lst_.end(),
                    {"HTTP/1.1 ", std::string_view(temp_.back()),
                     " ", code_, "\r\n"});
}

void HttpResponse::ComposeHeader_()
{
    constexpr std::string_view keep_alive_header {
        "Connection: keep-alive\r\n"
        "keep-alive: max=6, timeout=120\r\n"};
    constexpr std::string_view close_header {
        "Connection: close\r\n"};
    temp_.emplace_back(slurp_.error_message()
                           ? std::to_string(ErrorHtml_().size())
                           : std::to_string(slurp_.size()));

    res_lst_.insert(res_lst_.end(),
                    {(keep_alive_ ? keep_alive_header : close_header),
                     "Content-type: ", FileType_(), "\r\n",
                     "Content-Length: ", temp_.back(), "\r\n\r\n"});
}

void HttpResponse::ComposeContent_()
{
    if (slurp_.error_message()) {
        res_lst_.emplace_back(ErrorHtml_());
    }
}

constexpr auto HttpResponse::ErrorHtml_() -> std::string_view
{
    constexpr std::string_view error_html {
        "<html><title>Error</title>"
        "<body bgcolor=\"ffffff\">"
        "Error HTML"
        "<p>File Not Found</p>"
        "<hr><em>WebServer</em></body></html>"};
    return error_html;
}

auto HttpResponse::FileType_() -> std::string_view
{
    constexpr std::string_view default_type {
        "text/plain"};
    auto ext = full_path_.extension();
    if (ext.empty()) return default_type;
    if (auto find = suffix_type.find(ext.native());
        find != suffix_type.end()) {
        return find->second;
    }
    return default_type;
}
