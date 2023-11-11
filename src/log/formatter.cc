#include "log.hh"

// -------
//  Items
// -------

struct MessageItem : LogFormatter::Item {
    virtual void Format(
        std::ostream& os,
        LogInfo const& info,
        LogEvent::ptr event)
        override { os << event->Content(); }
};

struct LevelItem : LogFormatter::Item {
    virtual void Format(
        std::ostream& os,
        LogInfo const& info,
        LogEvent::ptr event)
        override { os << std::string(info.Level()); }
};

struct NameItem : LogFormatter::Item {
    virtual void Format(
        std::ostream& os,
        LogInfo const& info,
        LogEvent::ptr event)
        override { os << info.Name(); }
};

struct FuncNameItem : LogFormatter::Item {
    virtual void Format(
        std::ostream& os,
        LogInfo const& info,
        LogEvent::ptr event)
        override { os << event->FuncName(); }
};

struct ElapseTimeItem : LogFormatter::Item {
    virtual void Format(
        std::ostream& os,
        LogInfo const& info,
        LogEvent::ptr event)
        override
    {
        auto time = event->ElapsedTime();
        std::tm tm = *std::localtime(&time);
        os << std::put_time(&tm, default_format);
    }

private:
    static constexpr char const* default_format = "%Y:%m:%d %H:%M:%S";
};

struct ThreadIdItem : LogFormatter::Item {
    virtual void Format(
        std::ostream& os,
        LogInfo const& info,
        LogEvent::ptr event)
        override { os << event->ThreadId(); }
};

struct FiberIdItem : LogFormatter::Item {
    virtual void Format(
        std::ostream& os,
        LogInfo const& info,
        LogEvent::ptr event)
        override { os << event->FiberId(); }
};

struct TimeItem : LogFormatter::Item {
    TimeItem(std::string const& format = default_format) :
        format_(format)
    {
        if (format.empty()) format_ = default_format;
    }
    virtual void Format(
        std::ostream& os,
        LogInfo const& info,
        LogEvent::ptr event)
        override
    {
        auto time = event->Time();
        std::tm tm = *std::localtime(&time);
        os << std::put_time(&tm, format_.c_str());
    }

private:
    static constexpr char const* default_format = "%Y:%m:%d %H:%M:%S";

    std::string format_;
};

struct FileItem : LogFormatter::Item {
    virtual void Format(
        std::ostream& os,
        LogInfo const& info,
        LogEvent::ptr event)
        override { os << event->File(); }
};

struct LineItem : LogFormatter::Item {
    virtual void Format(
        std::ostream& os,
        LogInfo const& info,
        LogEvent::ptr event)
        override { os << event->Line(); }
};

struct PlainTextItem : LogFormatter::Item {
    PlainTextItem(std::string const& text) :
        text_(text) { }

    virtual void Format(
        std::ostream& os,
        LogInfo const& info,
        LogEvent::ptr event)
        override { os << text_; }

private:
    std::string text_;
};

// --------------
//  LogFormatter
// --------------

LogFormatter::LogFormatter(std::string_view pattern) :
    pattern_(pattern),
    items_()
{
    ItemGen(ParsePattern());
}

auto LogFormatter::Format(LogInfo const& info, LogEvent::ptr event)
    -> std::string
{
    std::stringstream ss;
    for (auto& item : items_) {
        item->Format(ss, info, event);
    }
    return ss.str();
}

// %c log name
// %m message
// %p level
// %r elapsed time
// %t thread id
// %d time
// %n line feed
// %f file name
// %l file line number
auto LogFormatter::ParsePattern()
    -> std::vector<std::pair<std::string, std::optional<std::string>>>
{
    // text:format
    // null format  : plain text
    // empty format : placeholder
    // normal format: formatted placeholder
    std::vector<std::pair<std::string, std::optional<std::string>>> v;

    std::istringstream ss(pattern_);
    std::string str;

    while (std::getline(ss, str, '%')) {
        if (!str.empty()) {
            v.emplace_back(std::move(str), std::nullopt);
            str.clear();
        }
        // use eof() because not yet read anything,
        // so fail() and bool(ss) don't work
        if (ss.eof()) break;

        // size_t pos = ss.view().find_first_of(" {%[", ss.tellg() + 1);
        // if (pos == std::string_view::npos) pos = ss.view().size();
        // pos -= ss.tellg();
        auto b = ss.view().begin() + static_cast<size_t>(ss.tellg()) + 1,
             e = ss.view().end();
        assert(b <= e);
        auto p = std::ranges::find_if_not(b, e, isalnum);
        size_t pos = p == e ? ss.view().size() : p - b + 1;

        std::string placeholder {ss.view().substr(ss.tellg(), pos)};
        ss.seekg(pos, ss.cur);

        if (placeholder.empty()) break;

        std::string format;
        if (ss.get() == '{') {
            std::getline(ss, format, '}');
        } else ss.unget();

        v.emplace_back(std::move(placeholder), std::move(format));
    }

    return v;
}

void LogFormatter::ItemGen(
    std::vector<std::pair<std::string, std::optional<std::string>>>&& v)
{
    typedef std::string const& format_t;
#define HELPER(str, name)                                \
    {                                                    \
        #str, [](format_t format) { return new name(); } \
    }
#define HELPER_FMT(str, name)                                  \
    {                                                          \
        #str, [](format_t format) { return new name(format); } \
    }
#define HELPER_PLAIN(str, text)                                        \
    {                                                                  \
        #str, [](format_t format) { return new PlainTextItem(#text); } \
    }

    static const std::map<
        std::string,
        std::function<Item*(format_t format)>>
        map {
            HELPER(m, MessageItem),
            HELPER(p, LevelItem),
            HELPER(c, NameItem),
            HELPER(x, FuncNameItem),
            HELPER(r, ElapseTimeItem),
            HELPER(t, ThreadIdItem),
            HELPER(f, FileItem),
            HELPER(l, LineItem),

            HELPER_FMT(d, TimeItem),

            HELPER_PLAIN(%, %),
            HELPER_PLAIN(n, \n),
            HELPER_PLAIN(T, \t),
        };

#undef HELPER
#undef HELPER_FMT
#undef HELPER_PLAIN

    for (auto const& [str, opt_format] : v) {
        Item* p;
        if (!opt_format) p = new PlainTextItem(str);
        else if (auto find = map.find(str); find != map.end()) {
            p = find->second(opt_format.value());
        } else p = new PlainTextItem("<<Invalid Format: " + str + ">>");

        items_.emplace_back(p);
    }
}
