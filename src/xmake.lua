includes("*/xmake.lua")

target("web-server")
    set_rundir(os.projectdir())
    set_kind("binary")
    add_files("main.cc")
    add_deps("http", "server", "config", "instance")
