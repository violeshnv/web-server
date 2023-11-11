target("sql")
    set_kind("static")
    add_files("*.cc")
    add_packages("pqxx", "pq")