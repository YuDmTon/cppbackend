#pragma once
#include <iosfwd>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace menu {
class Menu;
}

namespace app {
class UseCases;
}

namespace ui {
namespace detail {

struct AddBookParams {
    std::string title;
    std::string author_id;
    int         publication_year = 0;
    std::optional<std::set<std::string>> tags;
};

struct AuthorInfo {
    std::string id;
    std::string name;
};

struct BookInfo {
    std::string title;
    int         publication_year;
};

struct BookInfoExt {
    std::string title;
    std::string author;
    int         publication_year;
};



struct FullBookInfo {
    std::string id;
    std::string title;
    std::string author;
    int         publication_year;
    std::string tags;
    std::string ToString() {
        std::ostringstream oss;
//        oss << "Id: " << id << "\n";
        oss << "Title: " << title << "\n";
        oss << "Author: " << author << "\n";
        oss << "Publication year: " << publication_year << "\n";
        if ( !tags.empty() ) {
            oss << "Tags: " << tags << "\n";
        }
        return oss.str();
    }
};

}  // namespace detail

class View {
public:
    View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output);

private:
    bool AddAuthor(std::istream& cmd_input) const;
    bool AddBook(std::istream& cmd_input) const;
    bool ShowAuthors() const;
    bool ShowBooks() const;
    bool ShowAuthorBooks() const;

    std::optional<detail::AddBookParams> GetBookParams(std::istream& cmd_input) const;
    std::optional<std::string> SelectAuthor(bool prompt = true) const;
    std::vector<detail::AuthorInfo> GetAuthors() const;
    std::vector<detail::BookInfoExt> GetBooks() const;
    std::vector<detail::BookInfo> GetAuthorBooks(const std::string& author_id) const;

    //
    std::optional<std::string> GetAuthorName() const;
    std::optional<std::set<std::string>> GetTags() const;
    //
    bool DeleteAuthor(std::istream& cmd_input) const;
    std::optional<std::string> GetOrSelectAutor(std::istream& cmd_input) const;
    //
    bool EditAuthor(std::istream& cmd_input) const;
    //
    bool ShowBook(std::istream& cmd_input) const;
    std::vector<detail::FullBookInfo> GetBooksByTitle(const std::string& title) const;
    //
    bool DeleteBook(std::istream& cmd_input) const;
    //
    bool EditBook(std::istream& cmd_input) const;

private:
    menu::Menu& menu_;
    app::UseCases& use_cases_;
    std::istream& input_;
    std::ostream& output_;
};

}  // namespace ui
