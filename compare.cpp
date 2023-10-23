#include "compare.h"

// 用于比较牌面大小的函数
bool compareCards(const Card& card1, const Card& card2) {
    return card1.faceValue < card2.faceValue;
}

// 检测一对
bool isOnePair(const std::vector<Card>& cards) {
    for (int i = 0; i < 4; i++) {
        if (cards[i].faceValue == cards[i + 1].faceValue) {
            return true; // 发现一对
        }
    }
    return false;
}

// 检测两对
bool isTwoPair(const std::vector<Card>& cards) {
    int pairs = 0;
    for (int i = 0; i < 4; i++) {
        if (cards[i].faceValue == cards[i + 1].faceValue) {
            pairs++;
            i++;
        }
    }
    return pairs == 2;
}

// 检测三条
bool isThreeOfAKind(const std::vector<Card>& cards) {
    for (int i = 0; i < 3; i++) {
        if (cards[i].faceValue == cards[i + 2].faceValue) {
            return true; // 发现三条
        }
    }
    return false;
}

// 检测顺子
bool isStraight(const std::vector<Card>& cards) {
    // 检查是否存在一个连续的五张牌，同时考虑ACE的两种可能值
    for (int i = 0; i < 3; i++) {
        if (cards[i].faceValue + 1 == cards[i + 1].faceValue &&
            cards[i].faceValue + 2 == cards[i + 2].faceValue &&
            cards[i].faceValue + 3 == cards[i + 3].faceValue &&
            (cards[i].faceValue + 4 == cards[i + 4].faceValue ||
             (cards[i].faceValue == TWO && cards[4].faceValue == ACE))) {
            return true; // 发现顺子
        }
    }
    return false;
}

// 检测同花
bool isFlush(const std::vector<Card>& cards) {
    Suit suit = cards[0].suit;
    for (const Card& card : cards) {
        if (card.suit != suit) {
            return false;
        }
    }
    return true; // 所有牌都具有相同的花色，是同花
}

// 检测葫芦
bool isFullHouse(const std::vector<Card>& cards) {
    if ((cards[0].faceValue == cards[1].faceValue && cards[2].faceValue == cards[3].faceValue && cards[3].faceValue == cards[4].faceValue) ||
        (cards[0].faceValue == cards[1].faceValue && cards[1].faceValue == cards[2].faceValue && cards[3].faceValue == cards[4].faceValue)) {
        return true; // 发现葫芦
    }
    return false;
}

// 检测四条
bool isFourOfAKind(const std::vector<Card>& cards) {
    if (cards[0].faceValue == cards[1].faceValue && cards[1].faceValue == cards[2].faceValue && cards[2].faceValue == cards[3].faceValue) {
        return true; // 发现四条
    } else if (cards[1].faceValue == cards[2].faceValue && cards[2].faceValue == cards[3].faceValue && cards[3].faceValue == cards[4].faceValue) {
        return true; // 发现四条
    }
    return false;
}

// 检测同花顺
bool isStraightFlush(const std::vector<Card>& cards) {
    return isFlush(cards) && isStraight(cards); // 同花且顺子即为同花顺
}

// 检测皇家同花顺
bool isRoyalFlush(const std::vector<Card>& cards) {
    // 先检测是否是同花顺，再检查最大的牌是否是A（ACE）
    return isStraightFlush(cards) && cards.back().faceValue == ACE;
}

// 检测牌型
HandRank detectHandRank(const std::vector<Card>& combination) {
    if (isRoyalFlush(combination)) {
        return ROYAL_FLUSH;
    }
    else if (isStraight(combination)) {
        return STRAIGHT_FLUSH;
    }
    else if (isFourOfAKind(combination)) {
        return FOUR_OF_A_KIND;
    }
    else if (isFullHouse(combination)) {
        return FULL_HOUSE;
    }
    else if (isFlush(combination)) {
        return FLUSH;
    }
    else if (isStraight(combination)) {
        return STRAIGHT;
    }
    else if (isThreeOfAKind(combination)) {
        return THREE_OF_A_KIND;
    }
    else if (isTwoPair(combination)) {
        return TWO_PAIR;
    }
    else if (isOnePair(combination)) {
        return ONE_PAIR;
    }
    else {
        return HIGH_CARD;
    }
}

// 枚举最佳的五张牌组成最大牌型
std::vector<Card> chooseBestHand(const std::vector<Card>& holeCards, const std::vector<Card>& communityCards, int &bestRank) {
    std::vector<Card> allCards = communityCards;
    allCards.insert(allCards.end(), holeCards.begin(), holeCards.end());
    std::sort(allCards.begin(), allCards.end(), compareCards);

    std::vector<Card> bestHand;

    int n = allCards.size();
    for (int i = 0; i < (1 << n); ++ i) {
        if (__builtin_popcount(i) == 5) { 
            std::vector<Card> combination;
            for (int j = 0; j < n; ++ j) {
                if (i & (1 << j)) {
                    combination.push_back(allCards[j]);
                }
            }
            // 检测当前组合的牌型
            int currentRank = detectHandRank(combination);
            // // debugging
            // std::cout << "now comb:" << std::endl;
            // for (auto it: combination) {
            //     std::cout << faceValueToString(it.faceValue) << ' ';
            // }
            // std::cout << std::endl << currentRank << std::endl;
            // 如果当前组合的牌型更好，更新最佳牌型和最佳手牌
            if (currentRank > bestRank) {
                bestRank = currentRank;
                bestHand = combination;
            }
            else if (currentRank == bestRank) {
                int cp = compareHands(currentRank, currentRank, combination, bestHand);
                if (cp == 1) {
                    bestHand = combination;
                }
            }
        }
    }

    return bestHand;
}

// 比较两种牌型的大小
int compareHands(const int& rank1, const int& rank2, const std::vector<Card>& hand1, const std::vector<Card>& hand2) {
    if (rank1 < rank2) {
        return -1;
    } else if (rank1 > rank2) {
        return 1;
    } else {
        if (rank1 == STRAIGHT_FLUSH) {
            if (hand1[4].faceValue < hand2[4].faceValue) {
                return -1;
            }
            else if (hand1[4].faceValue > hand2[4].faceValue) {
                return 1;
            }
            else {
                return 0;
            }
        }
        else if (rank1 == FOUR_OF_A_KIND) {
            int FOUR1, FOUR2, ONE1, ONE2;
            if (hand1[0].faceValue == hand1[1].faceValue) {
                FOUR1 = hand1[0].faceValue;
                ONE1 = hand1[4].faceValue;
            }
            else {
                ONE1 = hand1[0].faceValue;
                FOUR1 = hand1[4].faceValue;
            }
            if (hand2[0].faceValue == hand2[1].faceValue) {
                FOUR2 = hand2[0].faceValue;
                ONE2 = hand2[4].faceValue;
            }
            else {
                ONE2 = hand2[0].faceValue;
                FOUR2 = hand2[4].faceValue;
            }

            if (FOUR1 < FOUR2) {
                return -1;
            } 
            else if (FOUR1 > FOUR2) {
                return 1;
            }
            else {
                if (ONE1 < ONE2) {
                    return -1;
                }
                else if (ONE1 > ONE2) {
                    return 1;
                }
                else {
                    return 0;
                }
            }
        }
        else if (rank1 == FULL_HOUSE) {
            int THREE1, THREE2, TWO1, TWO2;
            if (hand1[0].faceValue == hand1[2].faceValue) {
                THREE1 = hand1[0].faceValue;
                TWO1 = hand1[4].faceValue;
            }
            else {
                TWO1 = hand1[0].faceValue;
                THREE1 = hand1[4].faceValue;
            }
            if (hand2[0].faceValue == hand2[2].faceValue) {
                THREE2 = hand2[0].faceValue;
                TWO2 = hand2[4].faceValue;
            }
            else {
                TWO2 = hand2[0].faceValue;
                THREE2 = hand2[4].faceValue;
            }

            if (THREE1 < THREE2) {
                return -1;
            } 
            else if (THREE1 > THREE2) {
                return 1;
            }
            else {
                if (TWO1 < TWO2) {
                    return -1;
                }
                else if (TWO1 > TWO2) {
                    return 1;
                }
                else {
                    return 0;
                }
            }
        }
        else if (rank1 == FLUSH) {
            for (int i = 4; i >= 0; i --) {
                // std::cout << hand1[i].faceValue << ' ' << hand2[i].faceValue << std::endl;
                if (hand1[i].faceValue < hand2[i].faceValue) {
                    return -1;
                }
                else if (hand1[i].faceValue > hand2[i].faceValue) {
                    return 1;
                }
            }
            return 0;
        }
        else if (rank1 == STRAIGHT) {
            if (hand1[4].faceValue < hand2[4].faceValue) {
                return -1;
            }
            else if (hand1[4].faceValue > hand2[4].faceValue) {
                return 1;
            }
            else {
                return 0;
            }
        }
        else if (rank1 == THREE_OF_A_KIND) {
            int THREE1 = hand1[2].faceValue, THREE2 = hand2[2].faceValue;
            if (THREE1 > THREE2) {
                return 1;
            }
            else if (THREE1 < THREE2) {
                return -1;
            }
            else {
                // 处理三条剩下的两张单牌
                std::vector<Card> remaining1, remaining2;
                for (const Card& card : hand1) {
                    if (card.faceValue != THREE1) {
                        remaining1.push_back(card);
                    }
                }
                for (const Card& card : hand2) {
                    if (card.faceValue != THREE2) {
                        remaining2.push_back(card);
                    }
                }
                std::sort(remaining1.begin(), remaining1.end(), compareCards);
                std::sort(remaining2.begin(), remaining2.end(), compareCards);
                for (int i = 1; i >= 0; i --) {
                    if (remaining1[i].faceValue > remaining2[i].faceValue) {
                        return 1; 
                    } else if (remaining1[i].faceValue < remaining2[i].faceValue) {
                        return -1; 
                    }
                }
                return 0;
            }
        }
        else if (rank1 == TWO_PAIR) {
            // Pij为第i个玩家手牌中第j个对子（从小到大），Hi为第i个玩家手上的单牌
            int P11, P12, H1, P21, P22, H2;
            if (hand1[0].faceValue == hand1[1].faceValue && hand1[2].faceValue == hand1[3].faceValue) {
                P11 = hand1[0].faceValue;
                P12 = hand1[2].faceValue;
                H1 = hand1[4].faceValue;
            }
            else if (hand1[0].faceValue == hand1[1].faceValue && hand1[3].faceValue == hand1[4].faceValue) {
                P11 = hand1[0].faceValue;
                P12 = hand1[3].faceValue;
                H1 = hand1[2].faceValue;
            }
            else {
                P11 = hand1[1].faceValue;
                P12 = hand1[3].faceValue;
                H1 = hand1[0].faceValue;
            }
            if (hand2[0].faceValue == hand2[1].faceValue && hand2[2].faceValue == hand2[3].faceValue) {
                P21 = hand2[0].faceValue;
                P22 = hand2[2].faceValue;
                H2 = hand2[4].faceValue;
            }
            else if (hand2[0].faceValue == hand2[1].faceValue && hand2[3].faceValue == hand2[4].faceValue) {
                P21 = hand2[0].faceValue;
                P22 = hand2[3].faceValue;
                H2 = hand2[2].faceValue;
            }
            else {
                P21 = hand2[1].faceValue;
                P22 = hand2[3].faceValue;
                H2 = hand2[0].faceValue;
            }

            if (P12 < P22) {
                return -1;
            }
            else if (P12 > P22) {
                return 1;
            }
            else {
                if (P11 < P21) {
                    return -1;
                }
                else if (P11 > P21) {
                    return 1;
                }
                else {
                    if (H1 < H2) {
                        return -1;
                    }
                    else if (H1 > H2) {
                        return 1;
                    }
                    else {
                        return 0;
                    }
                }
            }
        }
        else if (rank1 == ONE_PAIR) {
            int TWO1, TWO2;
            for (int i = 0; i < 4; i ++) {
                if (hand1[i].faceValue == hand1[i + 1].faceValue) {
                    TWO1 = hand1[i].faceValue;
                }
                if (hand2[i].faceValue == hand2[i + 1].faceValue) {
                    TWO2 = hand2[i].faceValue;
                }
            }
            if (TWO1 < TWO2) {
                return -1;
            }
            else if (TWO1 > TWO2) {
                return 1;
            }
            else {
                // 处理一对剩下的三张单牌
                std::vector<Card> remaining1, remaining2;
                for (const Card& card : hand1) {
                    if (card.faceValue != TWO1) {
                        remaining1.push_back(card);
                    }
                }
                for (const Card& card : hand2) {
                    if (card.faceValue != TWO2) {
                        remaining2.push_back(card);
                    }
                }
                std::sort(remaining1.begin(), remaining1.end(), compareCards);
                std::sort(remaining2.begin(), remaining2.end(), compareCards);
                for (int i = 2; i >= 0; i --) {
                    if (remaining1[i].faceValue > remaining2[i].faceValue) {
                        return 1; 
                    } else if (remaining1[i].faceValue < remaining2[i].faceValue) {
                        return -1; 
                    }
                }
                return 0;
            }
        }
        else {
            for (int i = 4; i >= 0; i --) {
                if (hand1[i].faceValue < hand2[i].faceValue) {
                    return -1;
                }
                else if (hand1[i].faceValue > hand2[i].faceValue) {
                    return 1;
                }
            }
            return 0;
        }
    }
}

// 比较两个玩家的牌型大小
int comparePlayers(const std::vector<Card>& holeCards1, const std::vector<Card>& holeCards2, const std::vector<Card>& communityCards) {
    int rank1 = -1;
    std::vector<Card> hand1 = chooseBestHand(holeCards1, communityCards, rank1);
    int rank2 = -1;
    std::vector<Card> hand2 = chooseBestHand(holeCards2, communityCards, rank2);

    // // debugging
    // std::cout << "best hand1:" << std::endl;
    // for (auto it : hand1) {
    //     std::cout << faceValueToString(it.faceValue) << ' ' << suitToString(it.suit) << std::endl;
    // }
    // std::cout << "best hand2:" << std::endl;
    // for (auto it : hand2) {
    //     std::cout << faceValueToString(it.faceValue) << ' ' << suitToString(it.suit) << std::endl;
    // }
    // if (isRoyalFlush(hand1)) {
    //     std::cout << "hand1 Royal Flush" << std::endl;
    // } else if (isStraightFlush(hand1)) {
    //     std::cout << "hand1 Straight Flush" << std::endl;
    // } else if (isFourOfAKind(hand1)) {
    //     std::cout << "hand1 Four of a Kind" << std::endl;
    // } else if (isFullHouse(hand1)) {
    //     std::cout << "hand1 Full House" << std::endl;
    // } else if (isFlush(hand1)) {
    //     std::cout << "hand1 Flush" << std::endl;
    // } else if (isStraight(hand1)) {
    //     std::cout << "hand1 Straight" << std::endl;
    // } else if (isThreeOfAKind(hand1)) {
    //     std::cout << "hand1 Three of a Kind" << std::endl;
    // } else if (isTwoPair(hand1)) {
    //     std::cout << "hand1 Two Pair" << std::endl;
    // }
    // else if (isOnePair(hand1)) {
    //     std::cout << "hand1 One Pair" << std::endl;
    // }
    // else {
    //     std::cout << "hand1 High Card" << std::endl;
    // }

    // if (isRoyalFlush(hand2)) {
    //     std::cout << "hand2 Royal Flush" << std::endl;
    // } else if (isStraightFlush(hand2)) {
    //     std::cout << "hand2 Straight Flush" << std::endl;
    // } else if (isFourOfAKind(hand2)) {
    //     std::cout << "hand2 Four of a Kind" << std::endl;
    // } else if (isFullHouse(hand2)) {
    //     std::cout << "hand2 Full House" << std::endl;
    // } else if (isFlush(hand2)) {
    //     std::cout << "hand2 Flush" << std::endl;
    // } else if (isStraight(hand2)) {
    //     std::cout << "hand2 Straight" << std::endl;
    // } else if (isThreeOfAKind(hand2)) {
    //     std::cout << "hand2 Three of a Kind" << std::endl;
    // } else if (isTwoPair(hand2)) {
    //     std::cout << "hand2 Two Pair" << std::endl;
    // }
    // else if (isOnePair(hand2)) {
    //     std::cout << "hand2 One Pair" << std::endl;
    // }
    // else {
    //     std::cout << "hand2 High Card" << std::endl;
    // }

    return compareHands(rank1, rank2, hand1, hand2);
}