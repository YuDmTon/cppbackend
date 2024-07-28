#include <iostream>

#include "logger.h"

#include <boost/date_time.hpp>                              // BOOST_LOG_ATTRIBUTE_KEYWORD
#include <boost/log/trivial.hpp>                            // для BOOST_LOG_TRIVIAL
#include <boost/log/utility/setup/console.hpp>              // для вывода в консоль
#include <boost/log/utility/setup/file.hpp>                 // для вывода в файл
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


namespace logger {

using namespace std::literals;

void LogInit() {
/*
    logging::add_file_log(
        keywords::file_name  = "game_server.log", 
        keywords::format     = &GameLogFormatter,
        keywords::open_mode  = std::ios_base::app | std::ios_base::out,
        keywords::auto_flush = true
    );  
*/
    //
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

}  // namespace logger
