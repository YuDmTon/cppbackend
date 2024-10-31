#pragma once
#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"
#include "../domain/tag_fwd.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books, domain::TagRepository& tags)
        : authors_{authors}
        , books_{books}
        , tags_(tags) {
    }

    std::string AddAuthor(const std::string& name) override;
    std::vector<std::pair<std::string, std::string>> GetAuthors() override;
    std::optional<std::string> FindAuthorByName(const std::string& name) override;
    void DeleteAuthor(const std::string& author_id) override;
    void EditAuthor(const std::string& author_id, const std::string& new_name) override;

    std::string AddBook(const std::string& author_id, const std::string& title, int publication_year, const std::optional<std::set<std::string>>& tags) override;
    std::vector<std::tuple<std::string, std::string, int>> GetBooks() override;
    std::vector<std::pair<std::string, int>> GetAuthorBooks(const std::string& author_id) override;
    std::vector<std::tuple<std::string, std::string, std::string, int, std::string>> GetBooksByTitle(const std::string& title) override;
    void DeleteBook(const std::string& book_id) override;
    void EditBook(const std::string& book_id, const std::string& title, int publication_year, const std::optional<std::set<std::string>>& tags) override;

    void AddTag(const std::string& book_id, const std::string& tta) override;
    std::vector<std::pair<std::string, std::string>> GetBookTags(const std::string& book_id) override;

private:
    domain::AuthorRepository& authors_;
    domain::BookRepository&   books_;
    domain::TagRepository&    tags_;
};

}  // namespace app
