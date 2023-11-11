target("http")
    set_kind("static")
    add_files("*.cc")
    add_deps("log", "buffer")