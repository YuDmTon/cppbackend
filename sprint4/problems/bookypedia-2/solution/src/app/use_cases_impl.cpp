#include <iostream>

#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"
#include "../domain/tag.h"

namespace app {
using namespace domain;

std::string UseCasesImpl::AddAuthor(const std::string& name) {
    return authors_.Save({AuthorId::New(), name}).ToString();
}

std::vector<std::pair<std::string, std::string>> UseCasesImpl::GetAuthors() {
    std::vector<std::pair<std::string, std::string>> ret_authors;
    for (const auto& author : authors_.Read()) {
        std::pair ret_author{author.GetId().ToString(), author.GetName()};
        ret_authors.push_back(ret_author);
    }
    return ret_authors;
}

std::optional<std::string> UseCasesImpl::FindAuthorByName(const std::string& name) {
    auto ret_tag = authors_.FindByName(name);
    if ( not ret_tag.has_value() ) {
        return std::nullopt;
    }
    return (*ret_tag).ToString();
}

void UseCasesImpl::DeleteAuthor(const std::string& author_id) {
    authors_.Delete(AuthorId::FromString(author_id));
}

void UseCasesImpl::EditAuthor(const std::string& author_id, const std::string& new_name) {
    authors_.Edit({AuthorId::FromString(author_id), new_name});
}





std::string UseCasesImpl::AddBook(const std::string& author_id, const std::string& title, int publication_year, const std::optional<std::set<std::string>>& tags) {
    return books_.Save({BookId::New(), AuthorId::FromString(author_id), title, publication_year}, tags).ToString();
}

std::vector<std::tuple<std::string, std::string, int>> UseCasesImpl::GetBooks() {
    std::vector<std::tuple<std::string, std::string, int>> ret_books;
    for (const auto& book : books_.Read()) {
        std::tuple<std::string, std::string, int> ret_book{book.title, book.author, book.publication_year};
        ret_books.push_back(ret_book);
    }
    return ret_books;
}

std::vector<std::pair<std::string, int>> UseCasesImpl::GetAuthorBooks(const std::string& author_id) {
    std::vector<std::pair<std::string, int>> ret_books;
    for (const auto& book : books_.ReadAuthorBooks(author_id)) {
        std::pair ret_book{book.GetTitle(), book.GetPublicationYear()};
        ret_books.push_back(ret_book);
    }
    return ret_books;
}

std::vector<std::tuple<std::string, std::string, std::string, int, std::string>> UseCasesImpl::GetBooksByTitle(const std::string& title) {
    std::vector<std::tuple<std::string, std::string, std::string, int, std::string>> ret_books;
    for (const auto& [id, title, author, publication_year, tags] : books_.ReadByTitle(title)) {
        std::tuple<std::string, std::string, std::string, int, std::string> ret_book{id, title, author, publication_year, tags};
        ret_books.push_back(ret_book);
    }
    return ret_books;
}

void UseCasesImpl::DeleteBook(const std::string& book_id) {
    books_.Delete(BookId::FromString(book_id));
}

void UseCasesImpl::EditBook(const std::string& book_id, const std::string& title, int publication_year, const std::optional<std::set<std::string>>& tags) {
    return books_.Edit({BookId::FromString(book_id), AuthorId::New(), title, publication_year}, tags);
}



void UseCasesImpl::AddTag(const std::string& book_id, const std::string& tag) {
    tags_.Save({BookId::FromString(book_id), tag});
}

std::vector<std::pair<std::string, std::string>> UseCasesImpl::GetBookTags(const std::string& book_id) {
    std::vector<std::pair<std::string, std::string>> ret_tags;
    for (const auto& tag : tags_.ReadBookTags(book_id)) {
        std::pair ret_tag{tag.GetBookId().ToString(), tag.GetTag()};
        ret_tags.push_back(ret_tag);
    }
    return ret_tags;
}

}  // namespace app
