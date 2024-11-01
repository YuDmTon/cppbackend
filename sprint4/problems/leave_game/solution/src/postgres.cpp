#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <iostream>
#include <string>

#include "postgres.h"

using pqxx::operator"" _zv;

namespace postgres {

Db::Db() {
    auto conn = conn_pool_.GetConnection();
    pqxx::work work{*conn};
    work.exec(R"(
        CREATE TABLE IF NOT EXISTS retired_players (
            id UUID CONSTRAINT retired_players_id_constraint PRIMARY KEY, 
            name varchar(100) NOT NULL, 
            score integer NOT NULL, 
            play_time_ms integer NOT NULL);
        )"_zv
    );
    work.exec(R"(CREATE INDEX IF NOT EXISTS retired_players_multi_idx ON retired_players (score DESC, play_time_ms, name);)"_zv);
    work.commit();
////std::cout << "postgres::Db::Db(): '" << db_url_ << "'" << std::endl;

}

void Db::SaveRetiredPlayers(std::vector<std::tuple<std::string, int, int>> retired_players) {
////std::cout << "--- Db::SaveRetiredPlayers(): retired_players.size = " << retired_players.size() << std::endl;
    auto conn = conn_pool_.GetConnection();
    pqxx::work work{*conn};
    for (const auto& retired_player : retired_players) {
        auto id = boost::uuids::random_generator()();
////std::cout << "write: " << id << ", '" << to_string(id) << "'" << std::endl;
////std::cout << "write: '" << std::get<0>(retired_player) << "', " << std::get<1>(retired_player) << ", " << std::get<2>(retired_player) << std::endl;
        work.exec_params(R"(
                INSERT INTO retired_players (id, name, score, play_time_ms) VALUES ($1, $2, $3, $4);
            )"_zv,
            to_string(id), std::get<0>(retired_player), std::get<1>(retired_player), std::get<2>(retired_player)
        );
    }
    work.commit();
}

std::vector<std::tuple<std::string, int, int>> Db::ReadRetiredPlayers() {
    auto conn = conn_pool_.GetConnection();
    pqxx::read_transaction rt(*conn);
    std::vector<std::tuple<std::string, int, int>> retired_players;
    for (auto& [name, score, play_time] : rt.query<std::string, int, int>("SELECT name, score, play_time_ms FROM retired_players ORDER BY score DESC, play_time_ms, name"_zv)) {
        retired_players.emplace_back(name, score, play_time);
    }
    rt.commit();
    return retired_players;
}

}  // namespace postgres
