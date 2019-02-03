#include <utility>
#include <iterator>
#include <algorithm>
#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>
#include <entt/entity/group.hpp>

TEST(PersistentView, Functionalities) {
    entt::registry<> registry;
    auto group = registry.group<>(entt::get<int, char>);
    auto cgroup = std::as_const(registry).group<>(entt::get<const int, const char>);

    ASSERT_TRUE(group.empty());

    const auto e0 = registry.create();
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    ASSERT_FALSE(group.empty());
    ASSERT_NO_THROW((group.begin()++));
    ASSERT_NO_THROW((++cgroup.begin()));

    ASSERT_NE(group.begin(), group.end());
    ASSERT_NE(cgroup.begin(), cgroup.end());
    ASSERT_EQ(group.size(), typename decltype(group)::size_type{1});

    registry.assign<int>(e0);

    ASSERT_EQ(group.size(), typename decltype(group)::size_type{2});

    registry.remove<int>(e0);

    ASSERT_EQ(group.size(), typename decltype(group)::size_type{1});

    registry.get<char>(e0) = '1';
    registry.get<char>(e1) = '2';
    registry.get<int>(e1) = 42;

    for(auto entity: group) {
        ASSERT_EQ(std::get<0>(cgroup.get<const int, const char>(entity)), 42);
        ASSERT_EQ(std::get<1>(group.get<int, char>(entity)), '2');
        ASSERT_EQ(cgroup.get<const char>(entity), '2');
    }

    ASSERT_EQ(*(group.data() + 0), e1);

    registry.remove<char>(e0);
    registry.remove<char>(e1);

    ASSERT_EQ(group.begin(), group.end());
    ASSERT_EQ(cgroup.begin(), cgroup.end());
    ASSERT_TRUE(group.empty());
}

TEST(PersistentView, ElementAccess) {
    entt::registry<> registry;
    auto group = registry.group<>(entt::get<int, char>);
    auto cgroup = std::as_const(registry).group<>(entt::get<const int, const char>);

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    for(typename decltype(group)::size_type i{}; i < group.size(); ++i) {
        ASSERT_EQ(group[i], i ? e0 : e1);
        ASSERT_EQ(cgroup[i], i ? e0 : e1);
    }
}

TEST(PersistentView, Contains) {
    entt::registry<> registry;
    auto group = registry.group<>(entt::get<int, char>);

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    registry.destroy(e0);

    ASSERT_FALSE(group.contains(e0));
    ASSERT_TRUE(group.contains(e1));
}

TEST(PersistentView, Empty) {
    entt::registry<> registry;

    const auto e0 = registry.create();
    registry.assign<double>(e0);
    registry.assign<int>(e0);
    registry.assign<float>(e0);

    const auto e1 = registry.create();
    registry.assign<char>(e1);
    registry.assign<float>(e1);

    for(auto entity: registry.group<>(entt::get<char, int, float>)) {
        (void)entity;
        FAIL();
    }

    for(auto entity: registry.group<>(entt::get<double, char, int, float>)) {
        (void)entity;
        FAIL();
    }
}

TEST(PersistentView, Each) {
    entt::registry<> registry;
    auto group = registry.group<>(entt::get<int, char>);

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    auto cgroup = std::as_const(registry).group<>(entt::get<const int, const char>);
    std::size_t cnt = 0;

    group.each([&cnt](auto, int &, char &) { ++cnt; });
    group.each([&cnt](int &, char &) { ++cnt; });

    ASSERT_EQ(cnt, std::size_t{4});

    cgroup.each([&cnt](auto, const int &, const char &) { --cnt; });
    cgroup.each([&cnt](const int &, const char &) { --cnt; });

    ASSERT_EQ(cnt, std::size_t{0});
}

TEST(PersistentView, Sort) {
    entt::registry<> registry;
    auto group = registry.group<>(entt::get<const int, unsigned int>);

    const auto e0 = registry.create();
    const auto e1 = registry.create();
    const auto e2 = registry.create();

    auto uval = 0u;
    auto ival = 0;

    registry.assign<unsigned int>(e0, uval++);
    registry.assign<unsigned int>(e1, uval++);
    registry.assign<unsigned int>(e2, uval++);

    registry.assign<int>(e0, ival++);
    registry.assign<int>(e1, ival++);
    registry.assign<int>(e2, ival++);

    for(auto entity: group) {
        ASSERT_EQ(group.get<unsigned int>(entity), --uval);
        ASSERT_EQ(group.get<const int>(entity), --ival);
    }

    registry.sort<unsigned int>(std::less<unsigned int>{});
    group.sort<unsigned int>();

    for(auto entity: group) {
        ASSERT_EQ(group.get<unsigned int>(entity), uval++);
        ASSERT_EQ(group.get<const int>(entity), ival++);
    }
}

TEST(PersistentView, IndexRebuiltOnDestroy) {
    entt::registry<> registry;
    auto group = registry.group<>(entt::get<int, unsigned int>);

    const auto e0 = registry.create();
    const auto e1 = registry.create();

    registry.assign<unsigned int>(e0, 0u);
    registry.assign<unsigned int>(e1, 1u);

    registry.assign<int>(e0, 0);
    registry.assign<int>(e1, 1);

    registry.destroy(e0);
    registry.assign<int>(registry.create(), 42);

    ASSERT_EQ(group.size(), typename decltype(group)::size_type{1});
    ASSERT_EQ(group[{}], e1);
    ASSERT_EQ(group.get<int>(e1), 1);
    ASSERT_EQ(group.get<unsigned int>(e1), 1u);

    group.each([e1](auto entity, auto ivalue, auto uivalue) {
        ASSERT_EQ(entity, e1);
        ASSERT_EQ(ivalue, 1);
        ASSERT_EQ(uivalue, 1u);
    });
}

TEST(PersistentView, ConstNonConstAndAllInBetween) {
    entt::registry<> registry;
    auto group = registry.group<>(entt::get<int, const char>);

    ASSERT_TRUE((std::is_same_v<decltype(group.get<int>(0)), int &>));
    ASSERT_TRUE((std::is_same_v<decltype(group.get<const int>(0)), const int &>));
    ASSERT_TRUE((std::is_same_v<decltype(group.get<const char>(0)), const char &>));
    ASSERT_TRUE((std::is_same_v<decltype(group.get<int, const char>(0)), std::tuple<int &, const char &>>));
    ASSERT_TRUE((std::is_same_v<decltype(group.get<const int, const char>(0)), std::tuple<const int &, const char &>>));

    group.each([](auto, auto &&i, auto &&c) {
        ASSERT_TRUE((std::is_same_v<decltype(i), int &>));
        ASSERT_TRUE((std::is_same_v<decltype(c), const char &>));
    });
}

TEST(PersistentView, Find) {
    entt::registry<> registry;
    auto group = registry.group<>(entt::get<int, const char>);

    const auto e0 = registry.create();
    registry.assign<int>(e0);
    registry.assign<char>(e0);

    const auto e1 = registry.create();
    registry.assign<int>(e1);
    registry.assign<char>(e1);

    const auto e2 = registry.create();
    registry.assign<int>(e2);
    registry.assign<char>(e2);

    const auto e3 = registry.create();
    registry.assign<int>(e3);
    registry.assign<char>(e3);

    registry.remove<int>(e1);

    ASSERT_NE(group.find(e0), group.end());
    ASSERT_EQ(group.find(e1), group.end());
    ASSERT_NE(group.find(e2), group.end());
    ASSERT_NE(group.find(e3), group.end());

    auto it = group.find(e2);

    ASSERT_EQ(*it, e2);
    ASSERT_EQ(*(++it), e3);
    ASSERT_EQ(*(++it), e0);
    ASSERT_EQ(++it, group.end());
    ASSERT_EQ(++group.find(e0), group.end());

    const auto e4 = registry.create();
    registry.destroy(e4);
    const auto e5 = registry.create();
    registry.assign<int>(e5);
    registry.assign<char>(e5);

    ASSERT_NE(group.find(e5), group.end());
    ASSERT_EQ(group.find(e4), group.end());
}

TEST(PersistentView, ExcludedComponents) {
    entt::registry<> registry;

    const auto e0 = registry.create();
    registry.assign<int>(e0, 0);

    const auto e1 = registry.create();
    registry.assign<int>(e1, 1);
    registry.assign<char>(e1);

    const auto group = registry.group<>(entt::get<int>, entt::exclude<char>);

    const auto e2 = registry.create();
    registry.assign<int>(e2, 2);

    const auto e3 = registry.create();
    registry.assign<int>(e3, 3);
    registry.assign<char>(e3);

    for(const auto entity: group) {
        if(entity == e0) {
            ASSERT_EQ(group.get<int>(e0), 0);
        } else if(entity == e2) {
            ASSERT_EQ(group.get<int>(e2), 2);
        } else {
            FAIL();
        }
    }

    registry.assign<char>(e0);
    registry.assign<char>(e2);

    ASSERT_TRUE(group.empty());

    registry.remove<char>(e1);
    registry.remove<char>(e3);

    for(const auto entity: group) {
        if(entity == e1) {
            ASSERT_EQ(group.get<int>(e1), 1);
        } else if(entity == e3) {
            ASSERT_EQ(group.get<int>(e3), 3);
        } else {
            FAIL();
        }
    }
}

TEST(PersistentView, EmptyAndNonEmptyTypes) {
    struct empty_type {};
    entt::registry<> registry;
    const auto group = registry.group<>(entt::get<int, empty_type>);

    const auto e0 = registry.create();
    registry.assign<empty_type>(e0);
    registry.assign<int>(e0);

    const auto e1 = registry.create();
    registry.assign<empty_type>(e1);
    registry.assign<int>(e1);

    registry.assign<int>(registry.create());

    for(const auto entity: group) {
        ASSERT_TRUE(entity == e0 || entity == e1);
    }

    group.each([e0, e1](const auto entity, const int &, const empty_type &) {
        ASSERT_TRUE(entity == e0 || entity == e1);
    });

    ASSERT_EQ(group.size(), typename decltype(group)::size_type{2});
    ASSERT_EQ(&group.get<empty_type>(e0), &group.get<empty_type>(e1));
}
