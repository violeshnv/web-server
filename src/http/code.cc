#include "http.hh"

const std::unordered_map<int, std::string_view> HttpCode::code_name {
    {Unknown,     "Unknown"    },
    {OK,          "OK"         },
    {Bad_Request, "Bad Request"},
    {Forbidden,   "Forbidden"  },
    {Not_Found,   "Not Found"  },
};