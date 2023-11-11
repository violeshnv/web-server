rule("module.test")
    on_load(function (target)
        if not has_config("test") then
            target:set("enabled", false)
            return
        end

        target:set("rundir", os.projectdir() .. "/test")
        target:set("group", "test")
    end)