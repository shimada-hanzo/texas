#include <vector>
#include <algorithm>
#include <iostream>
#include "card.h"

enum HandRank {
    HIGH_CARD, // 高牌
    ONE_PAIR, // 一对
    TWO_PAIR, // 两对
    THREE_OF_A_KIND, // 三条
    STRAIGHT, // 顺子
    FLUSH, // 同花
    FULL_HOUSE, // 葫芦
    FOUR_OF_A_KIND, // 四条
    STRAIGHT_FLUSH, // 同花顺
    ROYAL_FLUSH // 皇家同花顺
};

// 结构表示玩家的手牌和公共牌
// struct PlayerHand {
//     std::vector<Card> holeCards;
//     std::vector<Card> communityCards;
// };

// bool compareCards(const Card& card1, const Card& card2);
// bool isHighCard(const std::vector<Card>& cards);
// bool isOnePair(const std::vector<Card>& cards);
// bool isTwoPair(const std::vector<Card>& cards);
// 比较两种牌型的大小
int compareHands(const int& rank1, const int& rank2, const std::vector<Card>& hand1, const std::vector<Card>& hand2);
// 比较两个玩家的牌型大小
int comparePlayers(const std::vector<Card>& holeCards1, const std::vector<Card>& holeCards2, const std::vector<Card>& communityCards);