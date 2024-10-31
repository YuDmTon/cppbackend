#pragma once
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "author.h"
#include "../util/tagged_uuid.h"

namespace domain {

namespace detail {
struct BookTag {};
}  // namespace detail

using BookId = util::TaggedUUID<detail::BookTag>;

struct InfoBook {
    std::string title;
    std::string author;
    int         publication_year;
};

struct FullInfoBook {
    std::string id;
    std::string title;
    std::string author;
    int         publication_year;
    std::string tags;
};

class Book {
public:
    Book(BookId id, AuthorId author_id, std::string title, int publication_year)
        : id_(std::move(id))
        , author_id_(std::move(author_id))
        , title_(std::move(title))
        , publication_year_(publication_year) {
    }

    const BookId& GetId() const noexcept {
        return id_;
    }

    const AuthorId& GetAuthorId() const noexcept {
        return author_id_;
    }

    const std::string& GetTitle() const noexcept {
        return title_;
    }

    const int GetPublicationYear() const noexcept {
        return publication_year_;
    }

private:
    BookId      id_;
    AuthorId    author_id_;
    std::string title_;
    int         publication_year_;
};

class BookRepository {
public:
    virtual BookId Save(const Book& book, const std::optional<std::set<std::string>>& tags) = 0;
    virtual std::vector<InfoBook> Read() = 0;
    virtual std::vector<Book> ReadAuthorBooks(const std::string& author_id) = 0;

    virtual std::vector<FullInfoBook> ReadByTitle(const std::string& title) = 0;

    virtual void Delete(const BookId& book_id) = 0;

    virtual void Edit(const Book& book, const std::optional<std::set<std::string>>& tags) = 0;

protected:
    ~BookRepository() = default;
};

}  // namespace domain
