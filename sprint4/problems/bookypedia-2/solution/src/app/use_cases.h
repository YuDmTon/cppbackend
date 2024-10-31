#pragma once

#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include <utility>

namespace app {

class UseCases {
public:
    virtual std::string AddAuthor(const std::string& name) = 0;
    virtual std::vector<std::pair<std::string, std::string>> GetAuthors() = 0;
    virtual std::optional<std::string> FindAuthorByName(const std::string& name) = 0;
    virtual void DeleteAuthor(const std::string& author_id) = 0;
    virtual void EditAuthor(const std::string& author_id, const std::string& new_name) = 0;

    virtual std::string AddBook(const std::string& author_id, const std::string& title, int publication_year, const std::optional<std::set<std::string>>& tags) = 0;
    virtual std::vector<std::tuple<std::string, std::string, int>> GetBooks() = 0;
    virtual std::vector<std::pair<std::string, int>> GetAuthorBooks(const std::string& author_id) = 0;
    virtual std::vector<std::tuple<std::string, std::string, std::string, int, std::string>> GetBooksByTitle(const std::string& title) = 0;
    virtual void DeleteBook(const std::string& book_id) = 0;
    virtual void EditBook(const std::string& book_id, const std::string& title, int publication_year, const std::optional<std::set<std::string>>& tags) = 0;

    virtual void AddTag(const std::string& book_id, const std::string& tag) = 0;
    virtual std::vector<std::pair<std::string, std::string>> GetBookTags(const std::string& book_id) = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
