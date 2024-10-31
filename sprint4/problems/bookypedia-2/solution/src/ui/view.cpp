#include "view.h"

#include <boost/algorithm/string/trim.hpp>
#include <cassert>
#include <iostream>
#include <limits>

#include "../app/use_cases.h"
#include "../menu/menu.h"

using namespace std::literals;
namespace ph = std::placeholders;

namespace ui {
namespace detail {

std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
    out << author.name;
    return out;
}

std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
    out << book.title << ", " << book.publication_year;
    return out;
}

std::ostream& operator<<(std::ostream& out, const BookInfoExt& book) {
    out << book.title << " by " << book.author << ", " << book.publication_year;
    return out;
}

std::ostream& operator<<(std::ostream& out, const FullBookInfo& book) {
    out << book.title << " by " << book.author << ", " << book.publication_year;
    return out;
}

}  // namespace detail

template <typename T>
void PrintVector(std::ostream& out, const std::vector<T>& vector) {
    int i = 1;
    for (auto& value : vector) {
        out << i++ << " " << value << std::endl;
    }
}

View::View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output)
    : menu_{menu}
    , use_cases_{use_cases}
    , input_{input}
    , output_{output} {
    menu_.AddAction(  //
        "AddAuthor"s, "name"s, "Adds author"s, std::bind(&View::AddAuthor, this, ph::_1)
        // либо
        // [this](auto& cmd_input) { return AddAuthor(cmd_input); }
    );
    menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s,
                    std::bind(&View::AddBook, this, ph::_1));
    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&View::ShowAuthors, this));
    menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&View::ShowBooks, this));
    menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s,
                    std::bind(&View::ShowAuthorBooks, this));
    menu_.AddAction("DeleteAuthor"s, {}, "Delete author and all his books"s,
                    std::bind(&View::DeleteAuthor, this, ph::_1));
    menu_.AddAction("EditAuthor"s, {}, "Edit author name"s,
                    std::bind(&View::EditAuthor, this, ph::_1));
    menu_.AddAction("ShowBook"s, {}, "Show book detail info"s,
                    std::bind(&View::ShowBook, this, ph::_1));
    menu_.AddAction("DeleteBook"s, {}, "Delete book"s,
                    std::bind(&View::DeleteBook, this, ph::_1));
    menu_.AddAction("EditBook"s, {}, "Edit book"s,
                    std::bind(&View::EditBook, this, ph::_1));
}

//// по факту API ////////////////////////////////////////////////////////////////////////////////
bool View::AddAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        if ( name.empty() ) {
            output_ << "Failed to add author"sv << std::endl;
        } else {
            use_cases_.AddAuthor(std::move(name));
        }
    } catch (const std::exception&) {
        output_ << "Failed to add author"sv << std::endl;
    }
    return true;
}

bool View::AddBook(std::istream& cmd_input) const {
    try {
        if (auto params = GetBookParams(cmd_input)) {
            if (not params.has_value()) {
                return true;
            }
            //
            use_cases_.AddBook(std::move(params->author_id), std::move(params->title), params->publication_year, params->tags);
        }
    } catch (const std::exception&) {
        output_ << "Failed to add book"sv << std::endl;
    }
    return true;
}

bool View::ShowAuthors() const {
        PrintVector(output_, GetAuthors());
    return true;
}

bool View::ShowBooks() const {
    PrintVector(output_, GetBooks());
    return true;
}

bool View::ShowAuthorBooks() const {
    // TODO: handle error
    try {
        if (auto author_id = SelectAuthor()) {
            if (not author_id.has_value()) {
                return true;
            }
            PrintVector(output_, GetAuthorBooks(*author_id));
        }
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to Show Books");
    }
    return true;
}


//// по факту служебные функции ////////////////////////////////////////////////////////////////////////////
std::optional<detail::AddBookParams> View::GetBookParams(std::istream& cmd_input) const {
    detail::AddBookParams params;
    // get book params
    cmd_input >> params.publication_year;
    std::getline(cmd_input, params.title);
    boost::algorithm::trim(params.title);
    if ( params.publication_year == 0 || params.title.empty() ) {
        return std::nullopt;
    }
    // try input author name
    auto author_name = GetAuthorName();
    if ( author_name.has_value() ) {
        auto author_id = use_cases_.FindAuthorByName(*author_name);
        if ( not author_id.has_value() ) {
            output_ << "No author found. Do you want to add Jack London (y/n)?" << std::endl;
            std::string yes;
            input_ >> yes;
            if ( yes == "Y" || yes == "y" ) {
                author_id = use_cases_.AddAuthor(std::move(*author_name));
            } else {
                output_ << "Failed to add book" << std::endl;
                return std::nullopt;
            }
        }
        params.author_id = *author_id;
    } else { // select author from list
        auto author_id = SelectAuthor();
        if (not author_id.has_value()) {
            return std::nullopt;
        } else {
            params.author_id = *author_id;
        }
    }
    // try input tags
    output_ << "Enter tags (comma separated):" << std::endl;
    params.tags = GetTags();
    return params;
}

std::optional<std::string> View::SelectAuthor(bool prompt) const {
    if ( prompt ) {
        output_ << "Select author:" << std::endl;
    }
    auto authors = GetAuthors();
    PrintVector(output_, authors);
    output_ << "Enter author # or empty line to cancel" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int author_idx;
    try {
        author_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid author num");
    }

    --author_idx;
    if (author_idx < 0 or author_idx >= authors.size()) {
        throw std::runtime_error("Invalid author num");
    }

    return authors[author_idx].id;
}

std::vector<detail::AuthorInfo> View::GetAuthors() const {
    std::vector<detail::AuthorInfo> autors;
    for (const auto& [id, name] : use_cases_.GetAuthors()) {
        detail::AuthorInfo author_info{id, name};
        autors.push_back(author_info);
    }
    return autors;
}

std::vector<detail::BookInfoExt> View::GetBooks() const {
    std::vector<detail::BookInfoExt> books;
    for (const auto& [title, author, publication_year] : use_cases_.GetBooks()) {
        detail::BookInfoExt book_info{title, author, publication_year};
        books.push_back(book_info);
    }
    return books;
}

std::vector<detail::BookInfo> View::GetAuthorBooks(const std::string& author_id) const {
    std::vector<detail::BookInfo> books;
    for (const auto& [title, publication_year] : use_cases_.GetAuthorBooks(author_id)) {
        detail::BookInfo book_info{title, publication_year};
        books.push_back(book_info);
    }
    return books;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
std::optional<std::string> View::GetAuthorName() const {
    output_ << "Enter author name or empty line to select from list:" << std::endl;
    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }
    return str;
}

std::optional<std::set<std::string>> View::GetTags() const {
    // try read
    std::set<std::string> tags;
    std::string tags_list;
//    input_.ignore(10, '\n');
    if (!std::getline(input_, tags_list) || tags_list.empty()) {
        return std::nullopt;
    }
    // parse read tags list
    std::string tag;
    for (size_t i = 0; i < tags_list.size(); ++i) {
        if ( tags_list[i] == ' ' ) {
            continue;
        }
        //
        if ( tags_list[i] == ',' || i == tags_list.size() - 1 ) {
            if ( !tag.empty() ) {
                tags.insert(tag);
                tag.clear();
            }
            continue;
        }
        //
        if ( i != 0 && !tag.empty() && tags_list[i] != ' ' && tags_list[i - 1] == ' ' ) {
            tag += ' ';
        }
        tag += tags_list[i];
    }
    if ( !tag.empty() ) {
        tags.insert(tag);
    }
    return tags;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////
bool View::DeleteAuthor(std::istream& cmd_input) const {
    try {
        if (auto author_id = GetOrSelectAutor(cmd_input)) {
            if (not author_id.has_value()) {
                return true;
            }
            use_cases_.DeleteAuthor(std::move(*author_id));
        }
    } catch (const std::exception&) {
        output_ << "Failed to delete author"sv << std::endl;
    }
    return true;
}

std::optional<std::string> View::GetOrSelectAutor(std::istream& cmd_input) const {
    std::string author_name;
    if (!std::getline(cmd_input, author_name) || author_name.empty()) {
        return SelectAuthor(false);
    } else {
        boost::algorithm::trim(author_name);
        auto author_id = use_cases_.FindAuthorByName(author_name);
        if ( not author_id.has_value() ) {
            output_ << "Failed to delete author" << std::endl;
            return std::nullopt;
        }
        return author_id;
    }
}



////////////////////////////////////////////////////////////////////////////////////////////////////////
bool View::EditAuthor(std::istream& cmd_input) const {
    try {
        if (auto author_id = GetOrSelectAutor(cmd_input)) {
            if (not author_id.has_value()) {
                return true;
            }
            output_ << "Enter new name:" << std::endl;
            std::string new_name;
            if (!std::getline(input_, new_name) || new_name.empty()) {
                output_ << "Failed to edit author"sv << std::endl;
                return true;
            }
            use_cases_.EditAuthor(std::move(*author_id), std::move(new_name));
        }
    } catch (const std::exception&) {
        output_ << "Failed to edit author"sv << std::endl;
    }
    return true;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////
bool View::ShowBook(std::istream& cmd_input) const {
    std::string title;
    if (!std::getline(cmd_input, title) || title.empty()) {
        auto books = GetBooksByTitle("");
        PrintVector(output_, books);
        output_ << "Enter the book # or empty line to cancel" << std::endl;

        std::string str;
//        input_ >> str;
        std::getline(input_, str);
        if ( str.empty() ) {
            return true;
        }

        int book_idx;
        try {
            book_idx = std::stoi(str);
        } catch (std::exception const&) {
            throw std::runtime_error("Invalid book num");
        }

        --book_idx;
        if (book_idx < 0 or book_idx >= books.size()) {
            throw std::runtime_error("Invalid book num");
        }

        output_ << books[book_idx].ToString();
    } else {
        boost::algorithm::trim(title);
        auto books = GetBooksByTitle(title);
        if ( books.empty() ) {
            return true;
        }
        if ( books.size() == 1 ) {
            output_ << books[0].ToString();
        } else {
            PrintVector(output_, books);
            output_ << "Enter the book # or empty line to cancel" << std::endl;
    
            std::string str;
//            input_ >> str;
            std::getline(input_, str);
            if ( str.empty() ) {
                return true;
            }
            input_.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
            int book_idx;
            try {
                book_idx = std::stoi(str);
            } catch (std::exception const&) {
                throw std::runtime_error("Invalid book num");
            }
    
            --book_idx;
            if (book_idx < 0 or book_idx >= books.size()) {
                throw std::runtime_error("Invalid book num");
            }
    
            output_ << books[book_idx].ToString();
        }
    }
//    input_.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return true;
}

std::vector<detail::FullBookInfo> View::GetBooksByTitle(const std::string& title) const {
    std::vector<detail::FullBookInfo> books;
    for (const auto& [id, title, author, publication_year, tags] : use_cases_.GetBooksByTitle(title)) {
        detail::FullBookInfo book_info{id, title, author, publication_year, tags};
        books.push_back(book_info);
    }
    return books;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////
bool View::DeleteBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::string id;
        if (!std::getline(cmd_input, title) || title.empty()) {
            auto books = GetBooksByTitle("");
            PrintVector(output_, books);
            output_ << "Enter the book # or empty line to cancel" << std::endl;

            std::string str;
//            input_ >> str;
            std::getline(input_, str);
            if ( str.empty() ) {
                return true;
            }

            int book_idx;
            try {
                book_idx = std::stoi(str);
            } catch (std::exception const&) {
                throw std::runtime_error("Invalid book num");
            }

            --book_idx;
            if (book_idx < 0 or book_idx >= books.size()) {
                throw std::runtime_error("Invalid book num");
            }

            id = books[book_idx].id;
        } else {
            boost::algorithm::trim(title);
            auto books = GetBooksByTitle(title);
            if ( books.empty() ) {
                return true;
            }
            if ( books.size() == 1 ) {
                id = books[0].id;
            } else {
                PrintVector(output_, books);
                output_ << "Enter the book # or empty line to cancel" << std::endl;
    
                std::string str;
//                input_ >> str;
                std::getline(input_, str);
                if ( str.empty() ) {
                    return true;
                }
    
                int book_idx;
                try {
                    book_idx = std::stoi(str);
                } catch (std::exception const&) {
                    throw std::runtime_error("Invalid book num");
                }
    
                --book_idx;
                if (book_idx < 0 or book_idx >= books.size()) {
                    throw std::runtime_error("Invalid book num");
                }
    
                id = books[book_idx].id;
            }
        }
//        input_.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        //
        use_cases_.DeleteBook(id);
    } catch (const std::exception&) {
        output_ << "Failed to delete book"sv << std::endl;
    }
    return true;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////
bool View::EditBook(std::istream& cmd_input) const {
    try {
        std::string title;
        detail::FullBookInfo book_info;
        if (!std::getline(cmd_input, title) || title.empty()) {
            auto books = GetBooksByTitle("");
            PrintVector(output_, books);
            output_ << "Enter the book # or empty line to cancel" << std::endl;

            std::string str;
//            input_ >> str;
            std::getline(input_, str);
            if ( str.empty() ) {
                return true;
            }

            int book_idx;
            try {
                book_idx = std::stoi(str);
            } catch (std::exception const&) {
                throw std::runtime_error("Invalid book num");
            }

            --book_idx;
            if (book_idx < 0 or book_idx >= books.size()) {
                throw std::runtime_error("Invalid book num");
            }

            book_info = books[book_idx];
        } else {
            boost::algorithm::trim(title);
            auto books = GetBooksByTitle(title);
            if ( books.empty() ) {
                return true;
            }
            if ( books.size() == 1 ) {
                book_info = books[0];
            } else {
                PrintVector(output_, books);
                output_ << "Enter the book # or empty line to cancel" << std::endl;
    
                std::string str;
//                input_ >> str;
                std::getline(input_, str);
                if ( str.empty() ) {
                    return true;
                }
    
                int book_idx;
                try {
                    book_idx = std::stoi(str);
                } catch (std::exception const&) {
                    throw std::runtime_error("Invalid book num");
                }
    
                --book_idx;
                if (book_idx < 0 or book_idx >= books.size()) {
                    throw std::runtime_error("Invalid book num");
                }
    
                book_info = books[book_idx];
            }
        }
//        input_.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        //
        output_ << "Enter new title or empty line to use the current one (" << book_info.title << "):" << std::endl;
        std::string new_title;
//        input_ >> new_title;
        std::getline(input_, new_title);
//std::cout << "new_title = '" << new_title << "'" << std::endl;
        //
        output_ << "Enter publication year or empty line to use the current one (" << book_info.publication_year << "):" << std::endl;
        int new_publication_year = 0;
//        input_ >> new_publication_year;
        std::string str;
        std::getline(input_, str);
        if ( !str.empty() ) {
            try {
                new_publication_year = std::stoi(str);
            } catch (std::exception const&) {
                throw std::runtime_error("Invalid year");
            }
        }
//std::cout << "new_publication_year = " << new_publication_year << std::endl;
        //
        output_ << "Enter tags (current tags: " << book_info.tags << "):" << std::endl;
        auto new_tags = GetTags();
/*
std::cout << ">>> '" << new_title << "', " << new_publication_year;
if ( new_tags.has_value() ) {
std::cout << ", [";
for (const auto& tag: new_tags.value()) std::cout << tag << ", ";
std::cout << "]";
}
std::cout << std::endl;
*/
        //
        use_cases_.EditBook(book_info.id, new_title, new_publication_year, new_tags);
    } catch (const std::exception&) {
        output_ << "Failed to delete book"sv << std::endl;
    }
    return true;
}



}  // namespace ui
