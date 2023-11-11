add_rules("module.test")

add_includedirs("../src")

for _, file in ipairs(os.files("*.cc")) do
    local name = path.basename(file)
    target("test." .. name)
        before_run(function (target)
            print("================== [launch ".. name .."] ====================")
        end)
        after_run(function (target)
            print("================== [finish ".. name .."] ====================")
        end)
        set_kind("binary")
        add_files(file)
        add_deps("log", "config", "timer", "http", "buffer")
    target_end()
end