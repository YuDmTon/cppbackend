#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include "../domain/author.h"
#include "../domain/book.h"

namespace postgres {

//// AuthorRepositoryImpl //////////////////////////////////////////////////////////////////////////////
class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(const domain::Author& author) override;
    std::vector<domain::Author> Read() override;

private:
    pqxx::connection& connection_;
};



//// BookRepositoryImpl ////////////////////////////////////////////////////////////////////////////////
class BookRepositoryImpl : public domain::BookRepository {
public:
    explicit BookRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(const domain::Book& book) override;
    std::vector<domain::Book> Read() override;
    std::vector<domain::Book> ReadAuthorBook(const std::string& author_id) override;

private:
    pqxx::connection& connection_;
};



//// Database /////////////////////////////////////////////////////////////////////////////////////////
class Database {
public:
    explicit Database(pqxx::connection connection);

    AuthorRepositoryImpl& GetAuthors() & {
        return authors_;
    }

    BookRepositoryImpl& GetBooks() & {
        return books_;
    }

private:
    pqxx::connection connection_;
    AuthorRepositoryImpl authors_{connection_};
    BookRepositoryImpl   books_  {connection_};
};

}  // namespace postgres
