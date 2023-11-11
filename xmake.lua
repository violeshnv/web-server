set_project("web-server")
set_version("0.0.1")
set_xmakever("2.8.3")

set_allowedplats("linux")

set_warnings("all", "error")
set_languages("c++20")
set_optimize("fastest")

add_requires("yaml-cpp", {system = true})
add_requires("pqxx", {system = true})
add_requires("pq", {system = true})

add_requires("boost", {configs = {fiber = true}})

add_defines("NDEBUG")

add_includedirs("inc", "src")

add_rules("mode.debug", "mode.release")

includes("src", "test", "xmake")

task("test")
    on_run(function ()
        os.exec("xmake f -m debug --test=y")
        os.exec("xmake build -g test")
        os.exec("xmake run -g test")
    end)

    set_menu{}