#pragma once
#include <optional>
#include <string>
#include <vector>

#include "book.h"
#include "../util/tagged_uuid.h"

namespace domain {

class Tag {
public:
    Tag(BookId book_id, std::string tag)
        : book_id_(std::move(book_id))
        , tag_(std::move(tag)) {
    }

    const BookId& GetBookId() const noexcept {
        return book_id_;
    }

    const std::string& GetTag() const noexcept {
        return tag_;
    }

private:
    BookId      book_id_;
    std::string tag_;
};

class TagRepository {
public:
    virtual void Save(const Tag& tag) = 0;
    virtual std::vector<Tag> ReadBookTags(const std::string& book_id) = 0;

protected:
    ~TagRepository() = default;
};

}  // namespace domain
