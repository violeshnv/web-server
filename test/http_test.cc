#include "config/config.hh"
#include "http/http.hh"

int main()
{
    Config& c = Config::Instance();
    c.LoadFile("test.yaml");
    c.Initialize();

    HttpRequest req;
    req.Parse(
        "GET /cpp/string/basic_string/operator%22%22s HTTP/2\r\n"
        "expires: Thu, 01 Jan 1970 00:00:00 GMT\r\n"
        "content-language: en\r\n"
        "tt-server: t=1699541436127354 D=24057\r\n"
        "content-encoding: gzip\r\n"
        "content-length: 10919\r\n"
        "content-type: text/html; charset=UTF-8\r\n");

    std::cout << "method: " << req.Method() << '\n'
              << "path: " << req.Path() << '\n'
              << "version: " << req.Version() << '\n'
              << "Body: " << req.Body() << '\n';

    for (auto const& [k, v] : req.Header()) {
        std::cout << k << ": " << v << '\n';
    }

    std::cout << "\n\n";

    HttpResponse res;
    res.Init(".", "http_test.cc");
    res.Compose();
    std::cout << res.Response()
              << res.FileView() << '\n';

    res.Init(".", "http_test.jpg");
    res.Compose();
    std::cout << res.Response()
              << res.FileView() << '\n';
}