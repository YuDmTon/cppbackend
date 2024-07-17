#include <iostream>

#include "json_loader.h"


//// Logs ///////////////////////////////
#include <boost/date_time.hpp>                              // BOOST_LOG_ATTRIBUTE_KEYWORD
#include <boost/log/trivial.hpp>                            // для BOOST_LOG_TRIVIAL
#include <boost/log/utility/setup/console.hpp>              // для вывода в консоль
#include <boost/log/utility/manipulators/add_value.hpp>     // logging::add_value

namespace json     = boost::json;
namespace keywords = boost::log::keywords;
namespace logging  = boost::log;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp,  "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(extra_data, "ExtraData", json::object)

void GameLogFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    // Выодим временную метку
    auto ts = *rec[timestamp];
    strm << "{\"timestamp\":\"" << to_iso_extended_string(ts) << "\",";
    // Выводим дополнительные данные.
    strm << "\"data\":" << rec[extra_data] << ",";
    // Выводим само сообщение.
    strm << "\"message\":\"" << rec[logging::expressions::smessage] << "\"}";
}
//// Logs ///////////////////////////////


namespace json_loader {

using namespace std::literals;

namespace json = boost::json;
namespace fs   = std::filesystem;

///////////////////////////////////////////
std::string ReadFile(const fs::path& path) {
    std::ifstream f(path, std::ios::in | std::ios::binary);
    if ( !f ) {
        std::string err = "Can't open config file '" + path.string() + "'";
        throw std::runtime_error(err);
    }
    const auto sz = fs::file_size(path);
    std::string result(sz, '\0');
    f.read(result.data(), sz);
    return result;
}

model::Game LoadGame(const fs::path& json_path, const fs::path& root_dir) {
    fs::path abs_srvr = fs::current_path();
    abs_srvr += "/";
    fs::path abs_json = abs_srvr;
    abs_json += json_path.string();
    abs_json  = fs::weakly_canonical(abs_json);
    fs::path abs_root = abs_srvr;
    abs_root += root_dir.string();
    abs_root  = fs::weakly_canonical(abs_root);
    //
    model::Game game;
    game.SetJsonStr(ReadFile(abs_json));
    //
    if ( !fs::is_directory(abs_root) ) {
        std::string err = "Root server directory '" + abs_root.string() + "' doesn't exist";
        throw std::runtime_error(err);
    }
    game.SetRootDir(abs_root.string());
    return game;
}

//// http ///////////////////////////////
std::string NotFound() {
    json::object result;
    result["code"] = "fileNotFound"s;
    result["name"] = "File not found"s;
    return json::serialize(result);
}

std::string BadRequest() {
    json::object result;
    result["code"] = "badRequest"s;
    result["name"] = "Bad request"s;
    return json::serialize(result);
}

std::string GetIdList(const std::string& json_str) {
    auto maps = json::parse(json_str);
    json::array result;
    for (const auto& map : maps.as_object().at("maps").as_array()) {
        json::object id;
        id["id"]   = map.as_object().at("id");
        id["name"] = map.as_object().at("name");
        result.push_back(id);
    }
    return json::serialize(result);
}

std::string GetMap(const std::string& json_str, const std::string& id) {
    auto maps  = json::parse(json_str).as_object().at("maps").as_array();
    auto is_id = [id](auto map) { return id == map.as_object().at("id").as_string(); };
    if (auto it = std::find_if(maps.begin(), maps.end(), is_id); it != maps.end()) {
        return json::serialize(*it);
    }
    return ""s;
}


//// Logs ///////////////////////////////
void LogInit() {
    logging::add_console_log( 
        std::cout,
        keywords::format     = &GameLogFormatter,
        keywords::auto_flush = true
    );
}

void LogStart(std::string address, unsigned port) {
    json::object data {
        {"port", port},
        {"address", address}
    };
    LogJson("server started"sv, data);
}

void LogStop() { LogStop(0, ""); }

void LogStop(int code, std::string what) {
    const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    json::object data {
        {"code", code}
    };
    if ( code != 0 ) {
        data["exception"] = what;
    }
    LogJson("server exited"sv, data);
}

void LogRequest(std::string ip, std::string_view uri, std::string method) {
    json::object data {
        {"ip",     ip},
        {"URI",    uri},
        {"method", method}
    };
    LogJson("request received"sv, data);
}

void LogResponse(int response_time, unsigned code, std::string_view content_type) {
    json::object data {
        {"response_time", response_time},
        {"code",          code},
        {"content_type",  content_type}
    };
    LogJson("response sent"sv, data);
}

void LogNetError(int code, std::string text, std::string_view where) {   
    json::object data {
        {"code",  code},
        {"text",  text},
        {"where", where}
    };
    LogJson("error"sv, data);
}

void LogJson(std::string_view message, json::object data) {   
    const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    BOOST_LOG_TRIVIAL(info) << message << logging::add_value(timestamp, now) << logging::add_value(extra_data, data);
}

}  // namespace json_loader
