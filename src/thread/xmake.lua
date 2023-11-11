target("thread")
    set_kind("static")
    add_files("*.cc")
    add_syslinks("pthread")