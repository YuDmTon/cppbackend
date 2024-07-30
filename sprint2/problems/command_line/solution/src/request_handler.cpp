#include "request_handler.h"

namespace http_handler {

// Возвращает true, если каталог path содержится внутри base.
bool RequestHandler::IsSubPath(fs::path path) {
    // Проверяем, что все компоненты base содержатся внутри path
    for (auto b = root_.begin(), p = path.begin(); b != root_.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

std::string RequestHandler::GetLowCaseExtention(fs::path path) {
    std::string ext = path.extension();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });
    return ext;
}

std::string RequestHandler::GetMime(fs::path path) {
    std::string ext = GetLowCaseExtention(path);
    //
    if ( ext == ".htm" || ext == ".html" ) return "text/html";
    if ( ext == ".css" )  return "text/css";
    if ( ext == ".txt" )  return "text/plain";
    if ( ext == ".js" )   return "text/javascript";
    if ( ext == ".json" ) return "application/json";
    if ( ext == ".xml" )  return "application/xml";
    if ( ext == ".png" )  return "image/png";
    if ( ext == ".jpg" || ext == ".jpe" || ext == ".jpeg" ) return "image/jpeg";
    if ( ext == ".gif" )  return "image/gif";
    if ( ext == ".bmp" )  return "image/bmp";
    if ( ext == ".ico" )  return "image/vnd.microsoft.icon";
    if ( ext == ".tif" || ext == ".tiff" ) return "image/tiff";
    if ( ext == ".svg" || ext == ".svgz" ) return "image/svg+xml";
    if ( ext == ".mp3" )  return "audio/mpeg";
    //
    return "application/octet-stream";
}

void RequestHandler::DumpRequest(const StringRequest& req) {
    std::cout << ">>> DumpRequest():" << std::endl;
    std::cout << req.method_string() << ' ' << req.target() << std::endl;
    // Выводим заголовки запроса
    for (const auto& header : req) {
        std::cout << "  "sv << header.name_string() << ": "sv << header.value() << std::endl;
    }
    std::cout << std::endl;
}
void RequestHandler::DumpResponse(const StringResponse& res) {
    std::cout << ">>> DumpResponse():" << std::endl;
    // Выводим заголовки запроса
    for (const auto& header : res) {
        std::cout << "  "sv << header.name_string() << ": "sv << header.value() << std::endl;
    }
    std::cout << res.body() << std::endl;
    std::cout << std::endl;
}

}  // namespace http_handler
