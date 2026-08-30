#pragma once

#include "interval_io.hpp"
#include "test_utility.hpp"

#ifdef INT_TREE_DRAW_EXAMPLES
#    include <interval-tree/draw.hpp>
#    include <filesystem>
#endif

#include <ctime>
#include <random>
#include <cmath>
#include <iostream>

struct Oracle
{
    int livingInstances = 0;
};

template <typename numerical_type, typename interval_kind_ = lib_interval_tree::closed>
class OracleInterval : public lib_interval_tree::interval<numerical_type, interval_kind_>
{
  public:
    using lib_interval_tree::interval<numerical_type, interval_kind_>::low_;
    using lib_interval_tree::interval<numerical_type, interval_kind_>::high_;

    OracleInterval(Oracle* oracle, numerical_type start, numerical_type end)
        : lib_interval_tree::interval<numerical_type, interval_kind_>(start, end)
        , oracle_{oracle}
    {
        ++oracle_->livingInstances;
    }
    OracleInterval(OracleInterval const& other)
        : lib_interval_tree::interval<numerical_type, interval_kind_>(other.low_, other.high_)
        , oracle_{other.oracle_}
    {
        ++oracle_->livingInstances;
    }
    OracleInterval(OracleInterval&& other)
        : lib_interval_tree::interval<numerical_type, interval_kind_>(other.low_, other.high_)
        , oracle_{other.oracle_}
    {
        other.oracle_ = nullptr;
    }
    OracleInterval& operator=(OracleInterval const& other)
    {
        lib_interval_tree::interval<numerical_type, interval_kind_>::operator=(other);
        oracle_ = other.oracle_;
        return *this;
    }
    OracleInterval& operator=(OracleInterval&& other)
    {
        lib_interval_tree::interval<numerical_type, interval_kind_>::operator=(other);
        oracle_ = other.oracle_;
        other.oracle_ = nullptr;
        return *this;
    }
    ~OracleInterval()
    {
        if (oracle_ != nullptr)
            --oracle_->livingInstances;
    }
    OracleInterval join(OracleInterval const& other) const
    {
        return OracleInterval{oracle_, std::min(low_, other.low_), std::max(high_, other.high_)};
    }

  private:
    Oracle* oracle_;
};

template <typename numerical_type, typename interval_kind_ = lib_interval_tree::closed>
OracleInterval<numerical_type, interval_kind_>
makeSafeOracleInterval(Oracle* oracle, numerical_type lhs, numerical_type rhs)
{
    return OracleInterval<numerical_type, interval_kind_>{oracle, std::min(lhs, rhs), std::max(lhs, rhs)};
}

class EraseTests : public ::testing::Test
{
  public:
    using interval_type = OracleInterval<int>;

  public:
    auto makeTree()
    {
        lib_interval_tree::interval_tree_t<int> regularTree;
        regularTree.insert({16, 21});
        regularTree.insert({8, 9});
        regularTree.insert({25, 30});
        regularTree.insert({5, 8});
        regularTree.insert({15, 23});
        regularTree.insert({17, 19});
        regularTree.insert({26, 26});
        regularTree.insert({0, 3});
        regularTree.insert({6, 10});
        regularTree.insert({19, 20});
        return regularTree;
    }

    static lib_interval_tree::interval_tree_t<int> makeTreeWithLows(std::initializer_list<int> lows)
    {
        lib_interval_tree::interval_tree_t<int> tree;
        for (auto const low : lows)
            tree.insert({low, low + 1});
        return tree;
    }

    static void verifyEraseResult(lib_interval_tree::interval_tree_t<int>& tree, int erasedLow)
    {
        EXPECT_EQ(tree.find({erasedLow, erasedLow + 1}), std::end(tree));
        testRedBlackPropertyViolation(tree);
        testMaxProperty(tree);
    }

    static void drawTreeForTest(lib_interval_tree::interval_tree_t<int> const& tree, std::string const& stage)
    {
#ifdef INT_TREE_DRAW_EXAMPLES
        std::filesystem::create_directory("test_drawings");
        auto const* testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
        lib_interval_tree::drawTree("test_drawings/" + std::string{testInfo->name()} + "_" + stage + ".png", tree);
#else
        (void)tree;
        (void)stage;
#endif
    }

  protected:
    Oracle oracle;
    lib_interval_tree::interval_tree<OracleInterval<int>> oracleTree;
    std::default_random_engine gen;
    std::uniform_int_distribution<int> distSmall{-500, 500};
    std::uniform_int_distribution<int> distLarge{-50000, 50000};
};

TEST_F(EraseTests, EraseSingleElement)
{
    auto inserted_interval = interval_type{&oracle, 0, 16};

    oracleTree.insert(std::move(inserted_interval));

    EXPECT_EQ(oracle.livingInstances, 1);
    oracleTree.erase(oracleTree.begin());

    EXPECT_EQ(oracle.livingInstances, 0);
    EXPECT_EQ(oracleTree.empty(), true);
    EXPECT_EQ(oracleTree.size(), 0);
}

TEST_F(EraseTests, ManualClearTest)
{
    constexpr int amount = 10'000;

    for (int i = 0; i != amount; ++i)
        oracleTree.insert(makeSafeOracleInterval(&oracle, distSmall(gen), distSmall(gen)));

    for (auto i = std::begin(oracleTree); i != std::end(oracleTree);)
        i = oracleTree.erase(i);

    EXPECT_EQ(oracle.livingInstances, 0);
    EXPECT_EQ(oracleTree.empty(), true);
}

TEST_F(EraseTests, ClearTest)
{
    constexpr int amount = 10'000;

    for (int i = 0; i != amount; ++i)
        oracleTree.insert(makeSafeOracleInterval(&oracle, distSmall(gen), distSmall(gen)));

    oracleTree.clear();

    EXPECT_EQ(oracle.livingInstances, 0);
    EXPECT_EQ(oracleTree.empty(), true);
}

TEST_F(EraseTests, ExpectedElementIsDeleted)
{
    oracleTree.insert(makeSafeOracleInterval(&oracle, 16, 21));
    oracleTree.insert(makeSafeOracleInterval(&oracle, 8, 9));
    oracleTree.insert(makeSafeOracleInterval(&oracle, 25, 30));
    oracleTree.insert(makeSafeOracleInterval(&oracle, 5, 8));
    oracleTree.insert(makeSafeOracleInterval(&oracle, 15, 23));
    oracleTree.insert(makeSafeOracleInterval(&oracle, 17, 19));
    oracleTree.insert(makeSafeOracleInterval(&oracle, 26, 26));
    oracleTree.insert(makeSafeOracleInterval(&oracle, 0, 3));
    oracleTree.insert(makeSafeOracleInterval(&oracle, 6, 10));
    oracleTree.insert(makeSafeOracleInterval(&oracle, 19, 20));

    oracleTree.erase(oracleTree.find(makeSafeOracleInterval(&oracle, 17, 19)));
    EXPECT_EQ(oracleTree.find(makeSafeOracleInterval(&oracle, 17, 19)), oracleTree.end());
    EXPECT_EQ(oracleTree.size(), 9);
}

TEST_F(EraseTests, RandomEraseTest)
{
    constexpr int amount = 10'000;
    constexpr int deleteAmount = 50;

    for (int i = 0; i != amount; ++i)
        oracleTree.insert(makeSafeOracleInterval(&oracle, distSmall(gen), distSmall(gen)));

    for (int i = 0; i != deleteAmount; ++i)
    {
        std::uniform_int_distribution<int> dist{0, amount - i - 1};
        auto end = dist(gen);
        auto iter = oracleTree.begin();
        for (int j = 0; j != end; ++j)
            ++iter;
        oracleTree.erase(iter);
    }

    EXPECT_EQ(oracle.livingInstances, amount - deleteAmount);
    testMaxProperty(oracleTree);
    testTreeHeightHealth(oracleTree);
    testRedBlackPropertyViolation(oracleTree);
}

TEST_F(EraseTests, EraseOfBlackLeafPreservesRedBlackProperties)
{
    lib_interval_tree::interval_tree_t<int> tree;
    tree.insert({0, 1});
    tree.insert({1, 2});
    tree.insert({2, 3});
    tree.insert({3, 4});

    drawTreeForTest(tree, "before");
    tree.erase(tree.find({0, 1}));
    drawTreeForTest(tree, "after");

    testRedBlackPropertyViolation(tree);
}

TEST_F(EraseTests, FromIssue66Test)
{
    lib_interval_tree::interval_tree_t<int> tree;
    tree.insert({-2, 0});
    tree.insert({-4, -3});
    tree.insert({-4, 4});
    tree.insert({1, 5});
    tree.insert({-2, 1});
    tree.insert({-4, -4});

    drawTreeForTest(tree, "before");
    tree.erase(tree.find({-4, 4}));
    drawTreeForTest(tree, "after");

    testRedBlackPropertyViolation(tree);
}

TEST_F(EraseTests, SequentialEraseKeepsRedBlackProperties)
{
    constexpr int amount = 256;

    lib_interval_tree::interval_tree_t<int> tree;
    for (int i = 0; i != amount; ++i)
        tree.insert({i, i + 1});

    for (int i = 0; i != amount; ++i)
    {
        std::uniform_int_distribution<int> dist{0, amount - i - 1};
        auto iter = tree.begin();
        const auto advanceBy = dist(gen);
        for (int j = 0; j != advanceBy; ++j)
            ++iter;
        tree.erase(iter);
        if (!tree.empty())
            testRedBlackPropertyViolation(tree);
    }

    EXPECT_TRUE(tree.empty());
}

TEST_F(EraseTests, EraseRedLeaf)
{
    using namespace lib_interval_tree;
    auto tree = makeTreeWithLows({0, 1, 2});
    auto node = tree.find({0, 1});
    ASSERT_EQ(node.color(), rb_color::red);
    ASSERT_EQ(node.left(), std::end(tree));
    ASSERT_EQ(node.right(), std::end(tree));

    drawTreeForTest(tree, "before");
    tree.erase(node);
    drawTreeForTest(tree, "after");

    verifyEraseResult(tree, 0);
}

TEST_F(EraseTests, EraseBlackNodeWithRedChild)
{
    using namespace lib_interval_tree;
    auto tree = makeTreeWithLows({0, 1, 2, 3});
    auto node = tree.find({2, 3});
    ASSERT_EQ(node.color(), rb_color::black);
    ASSERT_EQ(node.left(), std::end(tree));
    ASSERT_EQ(node.right().color(), rb_color::red);

    drawTreeForTest(tree, "before");
    tree.erase(node);
    drawTreeForTest(tree, "after");

    verifyEraseResult(tree, 2);
}

TEST_F(EraseTests, EraseRootWithSingleChild)
{
    using namespace lib_interval_tree;
    auto tree = makeTreeWithLows({0, 1});
    auto node = tree.find({0, 1});
    ASSERT_EQ(node.parent(), std::end(tree));
    ASSERT_EQ(node.left(), std::end(tree));
    ASSERT_EQ(node.right().color(), rb_color::red);

    drawTreeForTest(tree, "before");
    tree.erase(node);
    drawTreeForTest(tree, "after");

    EXPECT_EQ(tree.root().interval().low(), 1);
    EXPECT_EQ(tree.root().color(), rb_color::black);
    verifyEraseResult(tree, 0);
}

TEST_F(EraseTests, EraseNodeWithTwoChildrenAndRedSuccessor)
{
    using namespace lib_interval_tree;
    auto tree = makeTreeWithLows({0, 1, 2});
    auto node = tree.find({1, 2});
    ASSERT_NE(node.left(), std::end(tree));
    ASSERT_NE(node.right(), std::end(tree));
    ASSERT_EQ(node.right().left(), std::end(tree));
    ASSERT_EQ(node.right().color(), rb_color::red);

    drawTreeForTest(tree, "before");
    tree.erase(node);
    drawTreeForTest(tree, "after");

    verifyEraseResult(tree, 1);
}

TEST_F(EraseTests, EraseNodeWithTwoChildrenAndBlackSuccessor)
{
    using namespace lib_interval_tree;
    auto tree = makeTreeWithLows({0, 2, 3, 1});
    auto node = tree.find({2, 3});
    ASSERT_NE(node.left(), std::end(tree));
    ASSERT_NE(node.right(), std::end(tree));
    ASSERT_EQ(node.right().left(), std::end(tree));
    ASSERT_EQ(node.right().color(), rb_color::black);

    drawTreeForTest(tree, "before");
    tree.erase(node);
    drawTreeForTest(tree, "after");

    verifyEraseResult(tree, 2);
}

TEST_F(EraseTests, EraseFixupLeftDeficitRedSibling)
{
    using namespace lib_interval_tree;
    auto tree = makeTreeWithLows({0, 1, 2, 3, 4, 5});
    auto node = tree.find({0, 1});
    ASSERT_EQ(node.color(), rb_color::black);
    ASSERT_EQ(node.left(), std::end(tree));
    ASSERT_EQ(node.right(), std::end(tree));
    auto sibling = node.parent().right();
    ASSERT_EQ(sibling.color(), rb_color::red);
    ASSERT_EQ(sibling.left().color(), rb_color::black);
    ASSERT_EQ(sibling.left().left(), std::end(tree));
    ASSERT_EQ(sibling.left().right(), std::end(tree));

    drawTreeForTest(tree, "before");
    tree.erase(node);
    drawTreeForTest(tree, "after");

    verifyEraseResult(tree, 0);
}

TEST_F(EraseTests, EraseFixupLeftDeficitRedSiblingThenRedFarNephew)
{
    using namespace lib_interval_tree;
    auto tree = makeTreeWithLows({0, 1, 2, 4, 5, 3});
    auto node = tree.find({0, 1});
    ASSERT_EQ(node.color(), rb_color::black);
    ASSERT_EQ(node.left(), std::end(tree));
    ASSERT_EQ(node.right(), std::end(tree));
    auto sibling = node.parent().right();
    ASSERT_EQ(sibling.color(), rb_color::red);
    ASSERT_EQ(sibling.left().color(), rb_color::black);
    ASSERT_EQ(sibling.left().right().color(), rb_color::red);

    drawTreeForTest(tree, "before");
    tree.erase(node);
    drawTreeForTest(tree, "after");

    verifyEraseResult(tree, 0);
}

TEST_F(EraseTests, EraseFixupLeftDeficitBlackSiblingWithBlackNephews)
{
    using namespace lib_interval_tree;
    auto tree = makeTreeWithLows({0, 1, 2, 3, 4, 5, 6, 7});
    auto node = tree.find({0, 1});
    ASSERT_EQ(node.color(), rb_color::black);
    ASSERT_EQ(node.left(), std::end(tree));
    ASSERT_EQ(node.right(), std::end(tree));
    ASSERT_EQ(node.parent().color(), rb_color::red);
    auto sibling = node.parent().right();
    ASSERT_EQ(sibling.color(), rb_color::black);
    ASSERT_EQ(sibling.left(), std::end(tree));
    ASSERT_EQ(sibling.right(), std::end(tree));

    drawTreeForTest(tree, "before");
    tree.erase(node);
    drawTreeForTest(tree, "after");

    verifyEraseResult(tree, 0);
}

TEST_F(EraseTests, EraseFixupLeftDeficitBlackSiblingPropagatesUpwards)
{
    using namespace lib_interval_tree;
    auto tree = makeTreeWithLows({9, 0, 1, 2, 3, 5, 4, 7, 8, 6});
    tree.erase(tree.find({7, 8}));
    tree.erase(tree.find({2, 3}));
    tree.erase(tree.find({3, 4}));

    ASSERT_EQ(tree.size(), 7);
    for (auto i = std::begin(tree); i != std::end(tree); ++i)
        ASSERT_EQ(i.color(), rb_color::black);

    drawTreeForTest(tree, "before");
    tree.erase(tree.find({0, 1}));
    drawTreeForTest(tree, "after");

    verifyEraseResult(tree, 0);
}

TEST_F(EraseTests, EraseFixupLeftDeficitRedNearNephew)
{
    using namespace lib_interval_tree;
    auto tree = makeTreeWithLows({0, 1, 3, 2});
    auto node = tree.find({0, 1});
    ASSERT_EQ(node.color(), rb_color::black);
    ASSERT_EQ(node.left(), std::end(tree));
    ASSERT_EQ(node.right(), std::end(tree));
    auto sibling = node.parent().right();
    ASSERT_EQ(sibling.color(), rb_color::black);
    ASSERT_EQ(sibling.left().color(), rb_color::red);
    ASSERT_EQ(sibling.right(), std::end(tree));

    drawTreeForTest(tree, "before");
    tree.erase(node);
    drawTreeForTest(tree, "after");

    verifyEraseResult(tree, 0);
}

TEST_F(EraseTests, EraseFixupLeftDeficitRedFarNephew)
{
    using namespace lib_interval_tree;
    auto tree = makeTreeWithLows({0, 1, 2, 3});
    auto node = tree.find({0, 1});
    ASSERT_EQ(node.color(), rb_color::black);
    ASSERT_EQ(node.left(), std::end(tree));
    ASSERT_EQ(node.right(), std::end(tree));
    auto sibling = node.parent().right();
    ASSERT_EQ(sibling.color(), rb_color::black);
    ASSERT_EQ(sibling.left(), std::end(tree));
    ASSERT_EQ(sibling.right().color(), rb_color::red);

    drawTreeForTest(tree, "before");
    tree.erase(node);
    drawTreeForTest(tree, "after");

    verifyEraseResult(tree, 0);
}

TEST_F(EraseTests, EraseFixupRightDeficitRedSibling)
{
    using namespace lib_interval_tree;
    auto tree = makeTreeWithLows({5, 4, 3, 2, 1, 0});
    auto node = tree.find({5, 6});
    ASSERT_EQ(node.color(), rb_color::black);
    ASSERT_EQ(node.left(), std::end(tree));
    ASSERT_EQ(node.right(), std::end(tree));
    auto sibling = node.parent().left();
    ASSERT_EQ(sibling.color(), rb_color::red);
    ASSERT_EQ(sibling.right().color(), rb_color::black);
    ASSERT_EQ(sibling.right().left(), std::end(tree));
    ASSERT_EQ(sibling.right().right(), std::end(tree));

    drawTreeForTest(tree, "before");
    tree.erase(node);
    drawTreeForTest(tree, "after");

    verifyEraseResult(tree, 5);
}

TEST_F(EraseTests, EraseFixupRightDeficitRedSiblingThenRedFarNephew)
{
    using namespace lib_interval_tree;
    auto tree = makeTreeWithLows({5, 4, 3, 1, 0, 2});
    auto node = tree.find({5, 6});
    ASSERT_EQ(node.color(), rb_color::black);
    ASSERT_EQ(node.left(), std::end(tree));
    ASSERT_EQ(node.right(), std::end(tree));
    auto sibling = node.parent().left();
    ASSERT_EQ(sibling.color(), rb_color::red);
    ASSERT_EQ(sibling.right().color(), rb_color::black);
    ASSERT_EQ(sibling.right().left().color(), rb_color::red);

    drawTreeForTest(tree, "before");
    tree.erase(node);
    drawTreeForTest(tree, "after");

    verifyEraseResult(tree, 5);
}

TEST_F(EraseTests, EraseFixupRightDeficitBlackSiblingWithBlackNephews)
{
    using namespace lib_interval_tree;
    auto tree = makeTreeWithLows({7, 6, 5, 4, 3, 2, 1, 0});
    auto node = tree.find({7, 8});
    ASSERT_EQ(node.color(), rb_color::black);
    ASSERT_EQ(node.left(), std::end(tree));
    ASSERT_EQ(node.right(), std::end(tree));
    ASSERT_EQ(node.parent().color(), rb_color::red);
    auto sibling = node.parent().left();
    ASSERT_EQ(sibling.color(), rb_color::black);
    ASSERT_EQ(sibling.left(), std::end(tree));
    ASSERT_EQ(sibling.right(), std::end(tree));

    drawTreeForTest(tree, "before");
    tree.erase(node);
    drawTreeForTest(tree, "after");

    verifyEraseResult(tree, 7);
}

TEST_F(EraseTests, EraseFixupRightDeficitBlackSiblingPropagatesUpwards)
{
    using namespace lib_interval_tree;
    auto tree = makeTreeWithLows({0, 9, 8, 7, 6, 4, 5, 2, 1, 3});
    tree.erase(tree.find({2, 3}));
    tree.erase(tree.find({7, 8}));
    tree.erase(tree.find({6, 7}));

    ASSERT_EQ(tree.size(), 7);
    for (auto i = std::begin(tree); i != std::end(tree); ++i)
        ASSERT_EQ(i.color(), rb_color::black);

    drawTreeForTest(tree, "before");
    tree.erase(tree.find({9, 10}));
    drawTreeForTest(tree, "after");

    verifyEraseResult(tree, 9);
}

TEST_F(EraseTests, EraseFixupRightDeficitRedNearNephew)
{
    using namespace lib_interval_tree;
    auto tree = makeTreeWithLows({3, 2, 0, 1});
    auto node = tree.find({3, 4});
    ASSERT_EQ(node.color(), rb_color::black);
    ASSERT_EQ(node.left(), std::end(tree));
    ASSERT_EQ(node.right(), std::end(tree));
    auto sibling = node.parent().left();
    ASSERT_EQ(sibling.color(), rb_color::black);
    ASSERT_EQ(sibling.right().color(), rb_color::red);
    ASSERT_EQ(sibling.left(), std::end(tree));

    drawTreeForTest(tree, "before");
    tree.erase(node);
    drawTreeForTest(tree, "after");

    verifyEraseResult(tree, 3);
}

TEST_F(EraseTests, EraseFixupRightDeficitRedFarNephew)
{
    using namespace lib_interval_tree;
    auto tree = makeTreeWithLows({3, 2, 1, 0});
    auto node = tree.find({3, 4});
    ASSERT_EQ(node.color(), rb_color::black);
    ASSERT_EQ(node.left(), std::end(tree));
    ASSERT_EQ(node.right(), std::end(tree));
    auto sibling = node.parent().left();
    ASSERT_EQ(sibling.color(), rb_color::black);
    ASSERT_EQ(sibling.right(), std::end(tree));
    ASSERT_EQ(sibling.left().color(), rb_color::red);

    drawTreeForTest(tree, "before");
    tree.erase(node);
    drawTreeForTest(tree, "after");

    verifyEraseResult(tree, 3);
}

TEST_F(EraseTests, MassiveDeleteEntireTreeWithEraseReturnIterator)
{
    constexpr int amount = 1000;

    for (int i = 0; i != amount; ++i)
        oracleTree.insert(makeSafeOracleInterval(&oracle, distSmall(gen), distSmall(gen)));

    for (auto iter = oracleTree.begin(); !oracleTree.empty();)
    {
        iter = oracleTree.erase(iter);
    }

    EXPECT_EQ(oracle.livingInstances, 0);
    testMaxProperty(oracleTree);
    testTreeHeightHealth(oracleTree);
}

TEST_F(EraseTests, ReturnedIteratorPointsToNextInOrderNode)
{
    auto regularTree = makeTree();
    auto iter = regularTree.erase(regularTree.find({16, 21}));
    EXPECT_EQ(*iter, (decltype(regularTree)::interval_type{17, 19})) << *iter;

    regularTree = makeTree();
    iter = regularTree.erase(regularTree.find({8, 9}));
    EXPECT_EQ(*iter, (decltype(regularTree)::interval_type{15, 23})) << *iter;

    regularTree = makeTree();
    iter = regularTree.erase(regularTree.find({25, 30}));
    EXPECT_EQ(*iter, (decltype(regularTree)::interval_type{26, 26})) << *iter;

    regularTree = makeTree();
    iter = regularTree.erase(regularTree.find({5, 8}));
    EXPECT_EQ(*iter, (decltype(regularTree)::interval_type{6, 10})) << *iter;

    regularTree = makeTree();
    iter = regularTree.erase(regularTree.find({15, 23}));
    EXPECT_EQ(*iter, (decltype(regularTree)::interval_type{16, 21})) << *iter;

    regularTree = makeTree();
    iter = regularTree.erase(regularTree.find({17, 19}));
    EXPECT_EQ(*iter, (decltype(regularTree)::interval_type{19, 20})) << *iter;

    regularTree = makeTree();
    iter = regularTree.erase(regularTree.find({26, 26}));
    EXPECT_EQ(iter, regularTree.end());

    regularTree = makeTree();
    iter = regularTree.erase(regularTree.find({0, 3}));
    EXPECT_EQ(*iter, (decltype(regularTree)::interval_type{5, 8})) << *iter;

    regularTree = makeTree();
    iter = regularTree.erase(regularTree.find({6, 10}));
    EXPECT_EQ(*iter, (decltype(regularTree)::interval_type{8, 9})) << *iter;

    regularTree = makeTree();
    iter = regularTree.erase(regularTree.find({19, 20}));
    EXPECT_EQ(*iter, (decltype(regularTree)::interval_type{25, 30})) << *iter;
}

TEST_F(EraseTests, CanEraseEntireTreeUsingReturnedIterator)
{
    auto tree = makeTree();
    for (auto iter = tree.begin(); iter != tree.end();)
        iter = tree.erase(iter);
    EXPECT_EQ(tree.empty(), true);
}

TEST_F(EraseTests, FromNuiTest)
{
    lib_interval_tree::interval_tree_t<int> tree;
    tree.insert({0, 0});
    tree.insert({4, 4});
    tree.insert({13, 13});

    auto iter = tree.erase(tree.find({4, 4}));
    EXPECT_EQ(*iter, (decltype(tree)::interval_type{13, 13})) << *iter;
}

TEST_F(EraseTests, EraseRangeOnEmptyTreeDoesNothing)
{
    lib_interval_tree::interval_tree_t<int> tree;
    ASSERT_NO_FATAL_FAILURE(tree.erase_range({0, 10}, false));
    EXPECT_TRUE(tree.empty());
}

TEST_F(EraseTests, EraseRangeOnIntervalInsideRangeIsRemoved)
{
    lib_interval_tree::interval_tree_t<int> tree;
    tree.insert({0, 10});

    tree.erase_range({-10, 20}, false);
    EXPECT_TRUE(tree.empty());
}

TEST_F(EraseTests, EraseRangeOnIntervalEncompassingRangeIsRemoved)
{
    lib_interval_tree::interval_tree_t<int> tree;
    tree.insert({0, 10});

    tree.erase_range({3, 5}, false);
    EXPECT_TRUE(tree.empty());
}

TEST_F(EraseTests, NonOverlappingIntervalIsNotRemoved)
{
    lib_interval_tree::interval_tree_t<int> tree;
    tree.insert({0, 10});

    tree.erase_range({20, 30}, false);
    EXPECT_EQ(tree.size(), 1);
    EXPECT_EQ(*tree.begin(), (decltype(tree)::interval_type{0, 10}));
}

TEST_F(EraseTests, NonOverlappingIntervalIsNotRemoved2)
{
    lib_interval_tree::interval_tree_t<int> tree;
    tree.insert({0, 10});
    tree.insert({25, 35});

    tree.erase_range({20, 30}, false);
    EXPECT_EQ(tree.size(), 1);
    EXPECT_EQ(*tree.begin(), (decltype(tree)::interval_type{0, 10}));
}

TEST_F(EraseTests, EraseRangeOnIntervalWithLeftSlice)
{
    lib_interval_tree::interval_tree_t<int> tree;
    tree.insert({0, 10});

    tree.erase_range({-5, 5}, true);
    EXPECT_EQ(tree.size(), 1);
    EXPECT_EQ(*tree.begin(), (decltype(tree)::interval_type{5, 10}));
}

TEST_F(EraseTests, EraseRangeOnIntervalWithRightSlice)
{
    lib_interval_tree::interval_tree_t<int> tree;
    tree.insert({0, 10});

    tree.erase_range({5, 15}, true);
    EXPECT_EQ(tree.size(), 1);
    EXPECT_EQ(*tree.begin(), (decltype(tree)::interval_type{0, 5}));
}

TEST_F(EraseTests, EraseRangeMiddleCutOut)
{
    lib_interval_tree::interval_tree_t<int> tree;
    tree.insert({0, 10});

    tree.erase_range({3, 5}, true);
    EXPECT_EQ(tree.size(), 2);
    EXPECT_EQ(*tree.begin(), (decltype(tree)::interval_type{0, 3}));
    EXPECT_EQ(*(++tree.begin()), (decltype(tree)::interval_type{5, 10}));
}

TEST_F(EraseTests, EraseRangeLeftSliceIsNotReinsertedIfParamIsFalse)
{
    lib_interval_tree::interval_tree_t<int> tree;
    tree.insert({0, 10});

    tree.erase_range({-5, 5}, false);
    EXPECT_TRUE(tree.empty());
}

TEST_F(EraseTests, EraseRangeRightSliceIsNotReinsertedIfParamIsFalse)
{
    lib_interval_tree::interval_tree_t<int> tree;
    tree.insert({0, 10});

    tree.erase_range({5, 15}, false);
    EXPECT_TRUE(tree.empty());
}

TEST_F(EraseTests, SlicesAreReinsertedWithMultioverlap)
{
    lib_interval_tree::interval_tree_t<int> tree;
    tree.insert({0, 10});
    tree.insert({5, 15});
    tree.insert({10, 20});

    tree.erase_range({3, 12}, true);
    EXPECT_EQ(tree.size(), 2);
    EXPECT_EQ(*tree.begin(), (decltype(tree)::interval_type{0, 3}));
    EXPECT_EQ(*(++tree.begin()), (decltype(tree)::interval_type{12, 20}));
}