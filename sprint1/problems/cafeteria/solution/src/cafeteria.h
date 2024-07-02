#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <memory>

#include "hotdog.h"
#include "result.h"

#include <sstream>

namespace net = boost::asio;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

//// Order ////////////////////////////////////////////////////////////////////////////////////////
class Order : public std::enable_shared_from_this<Order> {
public:
    Order(net::io_context& io, int id, 
                               std::shared_ptr<Sausage> sausage, 
                               std::shared_ptr<Bread> bread, 
                               std::shared_ptr<GasCooker> gas_cooker,
                               HotDogHandler handler)
        : io_{io}
        , id_{id}
        , sausage_{sausage}
        , bread_{bread}
        , gas_cooker_{gas_cooker}
        , handler_{std::move(handler)} {
    }

    // Запускает асинхронное выполнение заказа
    void Execute() {
        FrySausage();
        BakeBread();
    }
    
private:
    void FrySausage() {
        net::post(io_, [self = shared_from_this()] () {
//            self->sausage_timer_.expires_after(HotDog::MIN_SAUSAGE_COOK_DURATION);
            self->sausage_->StartFry(*(self->gas_cooker_), [self] () {
                self->sausage_timer_.expires_after(HotDog::MIN_SAUSAGE_COOK_DURATION);
                self->sausage_timer_.async_wait([self](sys::error_code ec) {
                    self->OnFried(ec);
                });
            });
        });
    }
    void OnFried([[maybe_unused]]sys::error_code ec) {
        sausage_->StopFry();
        sausage_fried_ = true;
        CheckReadiness(ec);
    }
    
    void BakeBread() {
        net::post(io_, [self = shared_from_this()] () {
//            self->bread_timer_.expires_after(HotDog::MIN_BREAD_COOK_DURATION);
            self->bread_->StartBake(*(self->gas_cooker_), [self] () {
                self->bread_timer_.expires_after(HotDog::MIN_BREAD_COOK_DURATION);
                self->bread_timer_.async_wait([self](sys::error_code ec) {
                    self->OnBaked(ec);
                });
            });
        });
    }
    void OnBaked([[maybe_unused]]sys::error_code ec) {
        bread_->StopBaking();
        bread_baked_ = true;
        CheckReadiness(ec);
    }
    

    void CheckReadiness([[maybe_unused]]sys::error_code ec) {
        // Если все компоненты хот-дога готовы, собираем его
        if ( sausage_fried_ && bread_baked_ ) {
            HotDog hot_dog(id_, sausage_, bread_);
            handler_(Result(std::move(hot_dog)));
        }
    }
    
private:
    net::io_context&           io_;
    int                        id_;
    std::shared_ptr<Sausage>   sausage_;
    std::shared_ptr<Bread>     bread_;
    std::shared_ptr<GasCooker> gas_cooker_;
    HotDogHandler              handler_;
    //
    bool sausage_fried_ = false;
    bool bread_baked_   = false;
    
    //
    net::steady_timer sausage_timer_{io_, HotDog::MIN_SAUSAGE_COOK_DURATION};
    net::steady_timer bread_timer_  {io_, HotDog::MIN_BREAD_COOK_DURATION};
}; 

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(int order_id, HotDogHandler handler) {
        std::make_shared<Order>(io_, order_id, store_.GetSausage(), store_.GetBread(), gas_cooker_, std::move(handler))->Execute();
    }

private:
    net::io_context& io_;
    // Используется для создания ингредиентов хот-дога
    Store store_;
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};
