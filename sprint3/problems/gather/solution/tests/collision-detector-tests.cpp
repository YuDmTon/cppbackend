#define _USE_MATH_DEFINES
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <iostream>

#include "../src/collision_detector.h"

using namespace collision_detector;

// Напишите здесь тесты для функции collision_detector::FindGatherEvents
class TestItemGathererProvider : public ItemGathererProvider {
public:
    size_t ItemsCount() const override { return items_.size(); }
    Item GetItem(size_t idx) const override { return items_.at(idx); }
    size_t GatherersCount() const override { return gatherers_.size(); }
    Gatherer GetGatherer(size_t idx) const override { return gatherers_.at(idx); }
    // 
    void SetItems(std::vector<Item> items) { items_ = items; }
    void SetGatherers(std::vector<Gatherer> gatherers) { gatherers_ = gatherers; }
private:
    std::vector<Item>     items_;
    std::vector<Gatherer> gatherers_;
};

std::ostream& operator<<(std::ostream& os, const GatheringEvent& event)
{
    os << "[ item = " << event.item_id << ", gatherer = " << event.gatherer_id << ", sq_dist = " << event.sq_distance << ", time = " << event.time << " ]";
    return os;
}

class TestStand : public testing::Test {
public:
    void Run() { events_ = FindGatherEvents(provider_); }
    void SetItems(std::vector<Item> items) { provider_.SetItems(items); }
    void SetGatherers(std::vector<Gatherer> gatherers) { provider_.SetGatherers(gatherers); }
protected:
    TestItemGathererProvider    provider_;
    std::vector<GatheringEvent> events_;
};

////
TEST_F(TestStand, FullTest) {
    std::vector<Item>     items     = { {{2, 3.5}, 0.2}, {{4, 4}, 0.2}, {{4.5, 1.5}, 0.2}, {{4.5, 3.5}, 0.2}, {{5.5, 2.5}, 0.2}, {{5.5, 4.5}, 0.2}, {{6, 2}, 0.2}, {{8, 2.5}, 0.2} };
    std::vector<Gatherer> gatherers = { {{0, 3}, {11, 3}, 0.3}, {{5, 6}, {5, 0}, 0.3} };
    SetItems(items);
    SetGatherers(gatherers);
    Run();
    for (const auto& event : events_) {
        std::cout << event << std::endl;
    }
    //
    int events_count = 0;
    for (size_t i = 0; i < gatherers.size(); ++i) {
        for (size_t j = 0; j < items.size(); ++j ) {
            CollectionResult cr = TryCollectPoint(gatherers[i].start_pos, gatherers[i].end_pos, items[j].position);
            std::cout << "item = " << j << ", gatherer = " << i << ", ratio = " << cr.proj_ratio << ", dist = " << cr.sq_distance;
            if ( cr.IsCollected(0.3 + 0.2) ) {
                ++events_count;
                std::cout << ", collected+";
            }
            std::cout << std::endl;
        }
    }
    std::cout << "events_.size() = " << events_.size() << std::endl;
    std::cout << "events_count   = " << events_count   << std::endl;
    EXPECT_EQ(events_.size(), events_count);
}


