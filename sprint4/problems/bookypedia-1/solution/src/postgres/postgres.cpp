#include "postgres.h"

////#include <boost/uuid/uuid.hpp>
#include <pqxx/zview.hxx>
#include <pqxx/pqxx>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

//// AuthorRepositoryImpl //////////////////////////////////////////////////////////////////////////////
void AuthorRepositoryImpl::Save(const domain::Author& author) {
    // Пока каждое обращение к репозиторию выполняется внутри отдельной транзакции
    // В будущих уроках вы узнаете про паттерн Unit of Work, при помощи которого сможете несколько
    // запросов выполнить в рамках одной транзакции.
    // Вы также может самостоятельно почитать информацию про этот паттерн и применить его здесь.
    pqxx::work work{connection_};
    work.exec_params(
        R"(
            INSERT INTO authors (id, name) VALUES ($1, $2)
                ON CONFLICT (id) DO UPDATE SET name=$2;
        )"_zv,
        author.GetId().ToString(), author.GetName());
    work.commit();
}

std::vector<domain::Author> AuthorRepositoryImpl::Read() {
    std::vector<domain::Author> authors;
    pqxx::read_transaction rt(connection_);
    for (auto& [id, name] : rt.query<std::string, std::string>("SELECT * FROM authors ORDER BY name"_zv)) {
        domain::Author author(domain::AuthorId::FromString(id), name);
        authors.push_back(author);
    }
    return authors;
}



//// BookRepositoryImpl ////////////////////////////////////////////////////////////////////////////////
void BookRepositoryImpl::Save(const domain::Book& book) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
            INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4)
                ON CONFLICT (id) DO UPDATE SET author_id=$2, title=$3, publication_year=$4;
        )"_zv,
        book.GetId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(), book.GetPublicationYear());
    work.commit();
}

std::vector<domain::Book> BookRepositoryImpl::Read() {
    std::vector<domain::Book> books;
    pqxx::read_transaction rt(connection_);
    for (auto& [id, author_id, title, publication_year] : rt.query<std::string, std::string, std::string, int>("SELECT * FROM books ORDER BY publication_year, title"_zv)) {
        domain::Book book(domain::BookId::FromString(id), domain::AuthorId::FromString(author_id), title, publication_year);
        books.push_back(book);
    }
    return books;
}

std::vector<domain::Book> BookRepositoryImpl::ReadAuthorBook(const std::string& author_id) {
    std::vector<domain::Book> books;
    pqxx::read_transaction rt(connection_);
    for (const auto& row : rt.exec_params(R"(SELECT * FROM books WHERE author_id=$1 ORDER BY publication_year, title)"_zv, author_id)) {
        domain::Book book(
            domain::BookId::FromString(row["id"].c_str()),
            domain::AuthorId::FromString(row["author_id"].c_str()),
            row["title"].c_str(),
            row["publication_year"].as<int>());
        books.push_back(book);
    }
    return books;
}



//// Database /////////////////////////////////////////////////////////////////////////////////////////
Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    //
    work.exec(R"(
        CREATE TABLE IF NOT EXISTS authors (
            id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
            name varchar(100) UNIQUE NOT NULL
        );
    )"_zv);
    // ... создать другие таблицы
    work.exec(R"(
        CREATE TABLE IF NOT EXISTS books (
            id UUID CONSTRAINT book_id_constraint PRIMARY KEY,
            author_id UUID NOT NULL,
            title varchar(100) UNIQUE NOT NULL,
            publication_year integer
        );
    )"_zv);

    // коммитим изменения
    work.commit();
}

}  // namespace postgres
