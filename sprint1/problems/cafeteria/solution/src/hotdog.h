#pragma once
#include <chrono>
#include <functional>
#include <optional>
#include <stdexcept>

#include "gascooker.h"
#include "ingredients.h"

#include <sstream>
/*
Класс Хот-дог.
*/
class HotDog {
public:
    // Минимальная и максимальная длительность выпекания хлеба и приготовления сосиски для
    // использования в хот-доге.
    // Если время приготовления ингредиентов хот-дога не попадают в эти интервалы, хот-дог
    // считается забракованным

    constexpr static Clock::duration MIN_SAUSAGE_COOK_DURATION = Milliseconds{1500};
    constexpr static Clock::duration MAX_SAUSAGE_COOK_DURATION = Milliseconds{2000};
    constexpr static Clock::duration MIN_BREAD_COOK_DURATION   = Milliseconds{1000};
    constexpr static Clock::duration MAX_BREAD_COOK_DURATION   = Milliseconds{1500};

    HotDog(int id, std::shared_ptr<Sausage> sausage, std::shared_ptr<Bread> bread)
        : id_{id}
        , sausage_{std::move(sausage)}
        , bread_{std::move(bread)} {
        if (sausage_->GetCookDuration() < MIN_SAUSAGE_COOK_DURATION
            || sausage_->GetCookDuration() > MAX_SAUSAGE_COOK_DURATION) {
            ////
//            std::stringstream ss;
//            ss  << "Hot dog #" << id << ": Invalid sausage cook duration " << sausage_->GetCookDuration().count() << " in thread #" << std::this_thread::get_id() << std::endl;
//            std::cout << ss.str();
            ////
            throw std::invalid_argument("Invalid sausage cook duration");
        }

        if (bread_->GetBakingDuration() < MIN_BREAD_COOK_DURATION
            || bread_->GetBakingDuration() > MAX_BREAD_COOK_DURATION) {
            ////
//            std::stringstream ss;
//            ss  << "Hot dog #" << id << ": Invalid bread cook duration " << sausage_->GetCookDuration().count() << " in thread #" << std::this_thread::get_id() << std::endl;
//            std::cout << ss.str();
            ////
            throw std::invalid_argument("Invalid bread cook duration");
        }
    }

    int GetId() const noexcept {
        return id_;
    }

    const Sausage& GetSausage() const noexcept {
        return *sausage_;
    }

    const Bread& GetBread() const noexcept {
        return *bread_;
    }

private:
    int id_;
    std::shared_ptr<Sausage> sausage_;
    std::shared_ptr<Bread>   bread_;
};
