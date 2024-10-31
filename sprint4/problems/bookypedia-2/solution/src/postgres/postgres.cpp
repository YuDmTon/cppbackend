#include "postgres.h"

////#include <boost/uuid/uuid.hpp>
#include <pqxx/zview.hxx>
#include <pqxx/pqxx>

#include <iostream>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

//// AuthorRepositoryImpl //////////////////////////////////////////////////////////////////////////////
domain::AuthorId AuthorRepositoryImpl::Save(const domain::Author& author) {
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
    return author.GetId();
}

std::vector<domain::Author> AuthorRepositoryImpl::Read() {
    std::vector<domain::Author> authors;
    pqxx::read_transaction rt(connection_);
    for (auto& [id, name] : rt.query<std::string, std::string>("SELECT * FROM authors ORDER BY name"_zv)) {
        domain::Author author(domain::AuthorId::FromString(id), name);
        authors.push_back(author);
    }
    rt.commit();
    return authors;
}

std::optional<domain::AuthorId> AuthorRepositoryImpl::FindByName(const std::string& name) {
    pqxx::read_transaction rt(connection_);
    auto result = rt.exec_params(R"(SELECT id FROM authors WHERE name=$1;)"_zv, name);
    if ( result.empty() ) {
        return std::nullopt;
    }
    domain::AuthorId author_id(domain::AuthorId::FromString(result[0]["id"].c_str()));
    rt.commit();
    return author_id;
}

void AuthorRepositoryImpl::Delete(const domain::AuthorId& author_id) {
    pqxx::work work{connection_};
    work.exec_params(R"(DELETE FROM authors WHERE id=$1;)"_zv, author_id.ToString());
    //
    auto books = work.exec_params(R"(SELECT id FROM books WHERE author_id=$1;)"_zv, author_id.ToString());
    if ( !books.empty() ) {
        for (const auto& book : books) {
            work.exec_params(R"(DELETE FROM book_tags WHERE book_id=$1;)"_zv, book["id"].c_str());
        }
    }
    //
    work.exec_params(R"(DELETE FROM books WHERE author_id=$1;)"_zv, author_id.ToString());
    work.commit();
}

void AuthorRepositoryImpl::Edit(const domain::Author& author) {
    pqxx::work work{connection_};
    work.exec_params(R"(UPDATE authors SET name=$1 WHERE id=$2;)"_zv, author.GetName(), author.GetId().ToString());
    work.commit();
}



//// BookRepositoryImpl ////////////////////////////////////////////////////////////////////////////////
domain::BookId BookRepositoryImpl::Save(const domain::Book& book, const std::optional<std::set<std::string>>& tags) {
/*
std::cout << "Book::Save(): id = '" << book.GetId().ToString() << "', author_id = '" << book.GetAuthorId().ToString() << "'" << std::endl;
std::cout << "           title = '" << book.GetTitle() << "', publication_year = " << book.GetPublicationYear() << ", has_tags = " << std::boolalpha << tags.has_value() << ":" <<std::endl;
if ( tags.has_value() ) {
std::cout << "[ ";
for (const auto tag : tags.value())  std::cout << "'" << tag << "', ";
std::cout << "]\n" << std::endl;
}
*/
    pqxx::work work{connection_};
    work.exec_params(
        R"(
            INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4)
                ON CONFLICT (id) DO UPDATE SET author_id=$2, title=$3, publication_year=$4;
        )"_zv,
        book.GetId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(), book.GetPublicationYear());
    //
    if ( tags.has_value() ) {
        for (const auto& tag : tags.value()) {
            work.exec_params(R"(INSERT INTO book_tags (book_id, tag) VALUES ($1, $2);)"_zv, book.GetId().ToString(), tag);
        }
    }
    //
    work.commit();
    return book.GetId();
}

std::vector<domain::InfoBook> BookRepositoryImpl::Read() {
    std::vector<domain::InfoBook> books;
    pqxx::read_transaction rt(connection_);
    for (auto& [title, name, publication_year] : rt.query<std::string, std::string, int>(
                "SELECT title, name, publication_year FROM books JOIN authors ON books.author_id=authors.id ORDER BY title, name, publication_year;"_zv
            )) {
        domain::InfoBook book(title, name, publication_year);
        books.push_back(book);
    }
    rt.commit();
    return books;
}

std::vector<domain::Book> BookRepositoryImpl::ReadAuthorBooks(const std::string& author_id) {
    std::vector<domain::Book> books;
    pqxx::read_transaction rt(connection_);
    for (const auto& row : rt.exec_params(R"(SELECT * FROM books WHERE author_id=$1 ORDER BY publication_year, title;)"_zv, author_id)) {
        domain::Book book(
            domain::BookId::FromString(row["id"].c_str()),
            domain::AuthorId::FromString(row["author_id"].c_str()),
            row["title"].c_str(),
            row["publication_year"].as<int>());
        books.push_back(book);
    }
    rt.commit();
    return books;
}

std::vector<domain::FullInfoBook> BookRepositoryImpl::ReadByTitle(const std::string& title) {
    std::vector<domain::FullInfoBook> ret_books;
    pqxx::read_transaction rt(connection_);
    if ( title.empty() ) {
        for (const auto& [id, title, name, publication_year] : rt.query<std::string, std::string, std::string, int>(
                "SELECT books.id, title, name, publication_year FROM books JOIN authors ON books.author_id=authors.id ORDER BY title, name, publication_year;"_zv
            )) {
            std::string tags;
            domain::FullInfoBook ret_book{id, title, name, publication_year, tags};
            ret_books.push_back(ret_book);
        }
    } else {
        for (const auto& row : rt.exec_params(R"(SELECT books.id AS book_id, title, name, publication_year FROM books JOIN authors ON books.author_id=authors.id WHERE title=$1;)"_zv, title)) {
            std::string tags;
            bool first = true;
            for (const auto& tag_row : rt.exec_params(R"(SELECT tag FROM book_tags WHERE book_id=$1)"_zv, row["book_id"].c_str())) {
                if ( !first ) {
                    tags += ", ";
                }
                first = false;
                tags += tag_row["tag"].c_str();
            }
            domain::FullInfoBook ret_book{row["book_id"].c_str(), row["title"].c_str(), row["name"].c_str(), row["publication_year"].as<int>(), tags};
            ret_books.push_back(ret_book);
        }
    }
    rt.commit();
    return ret_books;
}

void BookRepositoryImpl::Delete(const domain::BookId& book_id) {
    pqxx::work work{connection_};
    work.exec_params(R"(DELETE FROM books WHERE id=$1;)"_zv, book_id.ToString());
    work.exec_params(R"(DELETE FROM book_tags WHERE book_id=$1;)"_zv, book_id.ToString());
    work.commit();
}

void BookRepositoryImpl::Edit(const domain::Book& book, const std::optional<std::set<std::string>>& tags) {
//std::cout << "book_id = '" << book.GetId().ToString() << "', author_id = '" << book.GetAuthorId().ToString() << "', title = '" << book.GetTitle() << "', year = " << book.GetPublicationYear() << std::endl;
    pqxx::work work{connection_};
    if ( !book.GetTitle().empty() ) {
        work.exec_params(R"(UPDATE books SET title=$1 WHERE id=$2;)"_zv, book.GetTitle(), book.GetId().ToString());
    }
    if ( book.GetPublicationYear() != 0 ) {
        work.exec_params(R"(UPDATE books SET publication_year=$1 WHERE id=$2;)"_zv, book.GetPublicationYear(), book.GetId().ToString());
    }
    //
    if ( tags.has_value() ) {
        work.exec_params(R"(DELETE FROM book_tags WHERE book_id=$1;)"_zv, book.GetId().ToString());
        for (const auto& tag : tags.value()) {
            work.exec_params(R"(INSERT INTO book_tags (book_id, tag) VALUES ($1, $2);)"_zv, book.GetId().ToString(), tag);
        }
    }
    //
    work.commit();
}




//// TagRepositoryImpl /////////////////////////////////////////////////////////////////////////////////
void TagRepositoryImpl::Save(const domain::Tag& tag) {
    pqxx::work work{connection_};
    work.exec_params( R"(INSERT INTO book_tags (book_id, tag) VALUES ($1, $2);)"_zv, tag.GetBookId().ToString(), tag.GetTag());
    work.commit();
}

std::vector<domain::Tag> TagRepositoryImpl::ReadBookTags(const std::string& book_id) {
    std::vector<domain::Tag> tags;
    pqxx::read_transaction rt(connection_);
    for (const auto& row : rt.exec_params(R"(SELECT * FROM book_tags WHERE book_id=$1 ORDER BY tag;)"_zv, book_id)) {
        domain::Tag tag(
            domain::BookId::FromString(row["book_id"].c_str()),
            row["tag"].c_str());
        tags.push_back(tag);
    }
    rt.commit();
    return tags;
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
            title varchar(100) NOT NULL,
            publication_year integer
        );
    )"_zv);
    // ... создать другие таблицы
    work.exec(R"(
        CREATE TABLE IF NOT EXISTS book_tags (
            book_id UUID NOT NULL,
            tag varchar(30) NOT NULL
        );
    )"_zv);

    // коммитим изменения
    work.commit();
}

}  // namespace postgres
