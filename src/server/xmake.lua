target("server")
    set_kind("static")
    add_files("*.cc")
    add_deps("http", "log", "thread", "timer")