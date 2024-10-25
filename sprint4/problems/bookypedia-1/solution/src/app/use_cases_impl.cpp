#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

std::vector<std::pair<std::string, std::string>> UseCasesImpl::GetAuthors() {
    std::vector<std::pair<std::string, std::string>> ret_authors;
    for (const auto& author : authors_.Read()) {
        std::pair ret_author{author.GetId().ToString(), author.GetName()};
        ret_authors.emplace_back(ret_author);
    }
    return ret_authors;
}

void UseCasesImpl::AddBook(const std::string& author_id, const std::string& title, int publication_year) {
    books_.Save({BookId::New(), domain::AuthorId::FromString(author_id), title, publication_year});
}

std::vector<std::tuple<std::string, std::string, std::string, int>> UseCasesImpl::GetBooks() {
    std::vector<std::tuple<std::string, std::string, std::string, int>> ret_books;
    for (const auto& book : books_.Read()) {
        std::tuple ret_book{book.GetId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(), book.GetPublicationYear()};
        ret_books.emplace_back(ret_book);
    }
    return ret_books;
}

std::vector<std::pair<std::string, int>> UseCasesImpl::GetAuthorBooks(const std::string& author_id) {
    std::vector<std::pair<std::string, int>> ret_books;
    for (const auto& book : books_.ReadAuthorBook(author_id)) {
        std::pair ret_book{book.GetTitle(), book.GetPublicationYear()};
        ret_books.emplace_back(ret_book);
    }
    return ret_books;
}

}  // namespace app
