// book_manager.cpp

#include <boost/json.hpp>
#include <pqxx/pqxx>

#include <iostream>
#include <optional>

using namespace std::literals;
// libpqxx использует zero-terminated символьные литералы вроде "abc"_zv;
using pqxx::operator"" _zv;

namespace json = boost::json;

int main(int argc, const char* argv[]) {
    try {
        if (argc == 1) {
            std::cout << "Usage: book_manager <conn-string>\n"sv;
            return EXIT_SUCCESS;
        } else if (argc != 2) {
            std::cerr << "Invalid command line\n"sv;
            return EXIT_FAILURE;
        }

        // Создаём подключение к БД
        pqxx::connection conn{argv[1]};

        // Если таблица не существует, создаём её
        pqxx::work w(conn);
        w.exec(
            "CREATE TABLE IF NOT EXISTS books ("
                "id SERIAL PRIMARY KEY, "
                "title varchar(100) NOT NULL, "
                "author varchar(100) NOT NULL, "
                "year integer NOT NULL, "
                "ISBN char(13) UNIQUE"
            ");"_zv
        );
        w.commit();

        // Предварительно готовим запрос
        constexpr auto add_book = "add_book"_zv;
        conn.prepare(add_book, "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, $4)"_zv);

        //
        std::string input;
        while ( true ) {
            std::getline(std::cin, input);
            if ( input.empty() ) {
                continue;
            }
            json::object request = json::parse(input).as_object();
            std::string  action  = json::value_to<std::string>(request.at("action"));
            if ( action == "add_book" ) {
                json::object payload = request.at("payload").as_object();
                std::string title  = json::value_to<std::string>(payload.at("title"));
                std::string author = json::value_to<std::string>(payload.at("author"));
                int         year   = json::value_to<int>(payload.at("year"));
                std::string isbn;
                if ( !payload.at("ISBN").is_null() ) {
                    isbn = json::value_to<std::string>(payload.at("ISBN")).c_str();
                }
                pqxx::work w(conn);
                std::cout << "{\"result\":true}" << std::endl;
                try {
                    if ( !isbn.empty() ) {
                        w.exec_prepared(add_book, title, author, year, isbn);
                    } else {
                        w.exec_prepared(add_book, title, author, year, nullptr);
                    }
                } catch (const std::exception& e) {
                    std::cout << "{\"result\":false}" << std::endl;
                }
                w.commit();
            } else if ( action == "all_books" ) {
                pqxx::read_transaction rt(conn);
                json::array all_books;
                for (auto [id, title, author, year, isbn] : rt.query<int, std::string, std::string, int, std::optional<std::string>>("SELECT * FROM books"_zv)) {
                    json::object book;
                    book["id"]     = id;
                    book["title"]  = id;
                    book["author"] = id;
                    book["year"]   = id;
                    if ( isbn ) {
                        book["ISBN"] = *isbn;
                    } else {
                        book["ISBN"] = nullptr;
                    }
                    all_books.push_back(book);
                }
                std::cout << json::serialize(all_books) << std::endl;
            } else if ( action == "exit" ) {
                break;
            } else {
                std::cerr << "Unknown request. Will be ignored" << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
