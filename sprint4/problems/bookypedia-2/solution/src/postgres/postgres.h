#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include "../domain/author.h"
#include "../domain/book.h"
#include "../domain/tag.h"

namespace postgres {

//// AuthorRepositoryImpl //////////////////////////////////////////////////////////////////////////////
class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    domain::AuthorId Save(const domain::Author& author) override;
    std::vector<domain::Author> Read() override;
    std::optional<domain::AuthorId> FindByName(const std::string& name) override;
    void Delete(const domain::AuthorId& author_id) override;
    void Edit(const domain::Author& author) override;

private:
    pqxx::connection& connection_;
};



//// BookRepositoryImpl ////////////////////////////////////////////////////////////////////////////////
class BookRepositoryImpl : public domain::BookRepository {
public:
    explicit BookRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    domain::BookId Save(const domain::Book& book, const std::optional<std::set<std::string>>& tags) override;
    std::vector<domain::InfoBook> Read() override;
    std::vector<domain::Book> ReadAuthorBooks(const std::string& author_id) override;

    std::vector<domain::FullInfoBook> ReadByTitle(const std::string& title) override;
    void Delete(const domain::BookId& book_id) override;
    void Edit(const domain::Book& book, const std::optional<std::set<std::string>>& tags) override;

private:
    pqxx::connection& connection_;
};



//// TagRepositoryImpl /////////////////////////////////////////////////////////////////////////////////
class TagRepositoryImpl : public domain::TagRepository {
public:
    explicit TagRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(const domain::Tag& tag) override;
    std::vector<domain::Tag> ReadBookTags(const std::string& book_id) override;

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

    TagRepositoryImpl& GetTags() & {
        return tags_;
    }

private:
    pqxx::connection connection_;
    AuthorRepositoryImpl authors_{connection_};
    BookRepositoryImpl   books_  {connection_};
    TagRepositoryImpl    tags_   {connection_};
};

}  // namespace postgres
