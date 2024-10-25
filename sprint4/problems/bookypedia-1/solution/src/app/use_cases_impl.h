#pragma once
#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books)
        : authors_{authors}
        , books_{books}  {
    }

    void AddAuthor(const std::string& name) override;
    std::vector<std::pair<std::string, std::string>> GetAuthors() override;

    void AddBook(const std::string& author_id, const std::string& title, int publication_year) override;
    std::vector<std::tuple<std::string, std::string, std::string, int>> GetBooks() override;
    virtual std::vector<std::pair<std::string, int>> GetAuthorBooks(const std::string& author_id) override;

private:
    domain::AuthorRepository& authors_;
    domain::BookRepository&   books_;
};

}  // namespace app
