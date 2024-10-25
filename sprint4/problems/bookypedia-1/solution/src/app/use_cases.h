#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <utility>

namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual std::vector<std::pair<std::string, std::string>> GetAuthors() = 0;

    virtual void AddBook(const std::string& author_id, const std::string& title, int publication_year) = 0;
    virtual std::vector<std::tuple<std::string, std::string, std::string, int>> GetBooks() = 0;
    virtual std::vector<std::pair<std::string, int>> GetAuthorBooks(const std::string& author_id) = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
