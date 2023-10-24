// #include <iostream>
// #include <vector>
// #include <algorithm>
#include <random>
#include <math.h>
#include <time.h>
#include <sstream>
#include "compare.h"
#include "server.h"

class TexasHoldemPoker {
public:
    TexasHoldemPoker() {
        for (int s = HEARTS; s <= SPADES; s++) {
            for (int v = TWO; v <= ACE; v++) {
                deck.push_back({ static_cast<FaceValue>(v), static_cast<Suit>(s) });
            }
        }
    }

    void shuffleDeck() {
        std::random_shuffle(deck.begin(), deck.end());
    }

    Card dealCard() {
        Card card = deck.back();
        deck.pop_back();
        return card;
    }

private:
    std::vector<Card> deck;
};

class player {
public:
    bool isFolded, isAllin, isInGame;
    int chip, betAmount, playNum;
    
    player(int ini, int num) {
        isInGame = true;
        isFolded = isAllin = false;
        chip = ini;
        betAmount = 0;
        playNum = num;
    }

    player(bool a, bool b, bool c, int d, int e) {
        isFolded = a;
        isAllin = b;
        isInGame = c;
        chip = d;
        betAmount = e;
    }

    void reset() {
        isFolded = isAllin = false;
        betAmount = 0;
    }
};

std::vector<player> playList;
std::vector<std::vector<Card>> playerholeCards;
std::vector<Card> communityCards;
bool check_flag;
int check_count, players_in_game;

// network
extern std::vector <SOCKET> clientSocket;

extern std::map <int, std::string> username;
extern int players;

extern WSADATA data;
extern SOCKET sock;
extern sockaddr_in addr;

bool roundOver(int maxAmount) {
    int players = playList.size();
    for (const player& p : playList) {
        if (!p.isInGame) {
            continue;
        }
        // debugging
        // std::cout << p.betAmount << ' ' << p.isAllin << ' ' << p.isFolded << std::endl;
        if (!p.isFolded && !p.isAllin && p.betAmount != maxAmount) {
            return false;
        }
    }
    return true;
}

void call(player& p, const int maxAmount, int& pot) {
    int x = maxAmount - p.betAmount;
    p.chip -= x;
    p.betAmount += x;
    pot += x;
    std::string s = username[p.playNum] + " 选择跟注，目前最高下注额为" + std::to_string(maxAmount);
    broadcast(s);
}

void raise(player& p, int raiseAmount, int& maxAmount, int& pot) {
    maxAmount += raiseAmount;
    int x = maxAmount - p.betAmount;
    p.chip -= x;
    p.betAmount += x;
    pot += x;
    std::string s = username[p.playNum] + " 选择加注，目前最高下注额为" + std::to_string(maxAmount);
    broadcast(s);
}

void allin(player& p, int& maxAmount, int& pot) {
    p.isAllin = true;
    int x = p.chip;
    p.chip -= x;
    p.betAmount += x;
    pot += x;
    maxAmount = (p.betAmount > maxAmount) ? p.betAmount : maxAmount;
    std::string s = username[p.playNum] + " 选择all in！目前最高下注额为" + std::to_string(maxAmount);
    broadcast(s);
}

void fold(player& p) {
    p.isFolded = true;
    std::string s = username[p.playNum] + " 选择弃牌";
    broadcast(s);
}

struct playerStat {
    int playerNumber;
    int wins;
    int betAmount;
};

bool cmp(const playerStat& ps1, const playerStat& ps2) {
    if (ps1.wins > ps2.wins) {
        return true;
    } else if (ps1.wins < ps2.wins) {
        return false;
    } else {
        return ps1.betAmount < ps2.betAmount;
    }
}

bool inFinal(int playerNum) {
    return (playList[playerNum].isInGame && !playList[playerNum].isFolded);
}

void updatePos(int& sb, int& bb, int& dealer, int players) {
    do {
        std::cout << "sbing" << sb << std::endl;
        sb = (sb + 1) % players;
    } while (!playList[sb].isInGame);
    bb = (sb + 1) % players;
    while (!playList[bb].isInGame) {
        std::cout << "bbing" << bb << std::endl;
        bb = (bb + 1) % players;
    } 
    dealer = (bb + 1) % players;
    while (!playList[dealer].isInGame) {
        std::cout << "dealering" << dealer << std::endl;
        dealer = (dealer - 1) % players;
        if (dealer < 0) {
            dealer += players;
        }
    } 
    std::string s = "位置更新\n";
    s = "小盲位：" + username[sb] + " 大盲位：" + username[bb] + " 庄家：" + username[dealer];
    broadcast(s);
}

void f8fq(int pot, int dealer) {
    std::string m = "\n结算\n\n";

    int players = playList.size();
    std::vector<playerStat> playerStatList;
    for (int i = 0; i < players; i ++) {
        if (inFinal(i)) {
            playerStatList.push_back(playerStat{i, 0, playList[i].betAmount});
            m += username[i] + "的底牌：\n";
            for (const Card& card : playerholeCards[i]) {
                m += faceValueToString(card.faceValue) + " of " + suitToString(card.suit) + '\n';
            }
        }
    }
    broadcast(m);

    m = "公共牌：\n";
    for (const Card& card : communityCards) {
        m += faceValueToString(card.faceValue) + " of " + suitToString(card.suit) + '\n';
    }
    broadcast(m);

    int finalNum = playerStatList.size();
    for (int i = 0; i < finalNum; i ++) {
        for (int j = i + 1; j < finalNum; j ++) {
            int rk = comparePlayers(playerholeCards[playerStatList[i].playerNumber], playerholeCards[playerStatList[j].playerNumber], communityCards);
            if (rk == 1) {
                playerStatList[i].wins ++;
            }
            else if (rk == -1) {
                playerStatList[j].wins ++;
            }
        }
    }

    // debugging
    // playerStatList[0].wins = 1;
    // playerStatList[1].wins = 0;
    // int finalNum = 2;

    std::sort(playerStatList.begin(), playerStatList.end(), cmp);

    // debugging
    for (auto ps : playerStatList) {
        std::cout << ps.playerNumber << ' ' << ps.betAmount << ' ' << ps.wins << std::endl;
    }
    std::cout << "player " << playerStatList[0].playerNumber;
    m = "最终比较结果\n" + username[playerStatList[0].playerNumber];
    for (int i = 1; i < finalNum; i ++) {
        if (playerStatList[i].wins == playerStatList[i - 1].wins) {
            std::cout << " = ";
            m += " = ";
        } else {
            std:: cout << " > ";
            m += " > ";
        }
        std::cout << "player " << playerStatList[i].playerNumber;
        m += username[playerStatList[i].playerNumber];
    }
    std::cout << std::endl;
    broadcast(m);
    
    for (int i = 0; i < players; i ++) {
        if (playList[i].isInGame && playList[i].isFolded) {
            playerStatList.push_back(playerStat{i, -1, playList[i].betAmount});
        }
    }

    std::vector <int> getMoney;
    for (int i = 0; i < players; i ++) {
        getMoney.push_back(-playList[i].betAmount);
    }
    for (int i = 0; i < finalNum; i ++) {
        // count统计能和当前玩家分钱的玩家数(筹码大于等于自己且牌力相等)
        int count = 0;
        for (int j = i; j < finalNum; j ++) {
            if (playerStatList[i].wins == playerStatList[j].wins) {
                count ++;
            }
        }

        // 分钱
        int x = 0;//playerStatList[i].betAmount * (count + playerStatList[i].wins);
        for (int j = i; j < playerStatList.size(); j ++) {
            if (playerStatList[i].wins >= playerStatList[j].wins) {
                x += std::min(playerStatList[i].betAmount, playerStatList[j].betAmount);
            }
        }
        // debugging
        std::cout << playerStatList[i].playerNumber << ' ' << x << std::endl;
        // 处理零散筹码
        int odd = x % count;
        if (odd != 0) {
            x -= odd;
            // 给庄家后最近一个玩家
            int now = (dealer + 1) % players;
            while (true) {
                if (inFinal(now)) {
                    playList[now].chip += odd;
                    getMoney[now] += odd;
                    pot -= odd;
                    break;
                }
                now = (now + 1) % players;
            }
        }
        for (int j = i; j < playerStatList.size(); j ++) {
            if (playerStatList[i].wins == playerStatList[j].wins) {
                playList[playerStatList[j].playerNumber].chip += x / count;
                getMoney[playerStatList[j].playerNumber] += x / count;
                playerStatList[j].betAmount -= std::min(playList[playerStatList[i].playerNumber].betAmount, playerStatList[j].betAmount);
                pot -= x / count;
            }
            else {
                playerStatList[j].betAmount -= std::min(playList[playerStatList[i].playerNumber].betAmount, playerStatList[j].betAmount);
                pot -= x / count;
            }
        }
        if (pot == 0) {
            m = "收益情况\n";
            for (int j = 0; j < players; j ++) {
                if (getMoney[j] >= 0) {
                    m += username[j] + " +" + std::to_string(getMoney[j]) + '\n';
                } else {
                    m += username[j] + " " + std::to_string(getMoney[j]) + '\n';
                }
            }
            broadcast(m);
            return;
        }
    }
}

void makeDecision(int decisions, player& p, int& maxAmount, int& pot, std::string input) {
    // std::string input;
    // std::getline(std::cin, input);

    std::istringstream iss(input);
    std::string command;
    iss >> command;

    std::cout << "input: " << input << std::endl;
    std::cout << "command: " << command << std::endl;
    if (command == "r") {
        if (decisions < 4) {
            std::cout << "'r' command received. You can't raise. Fold." << std::endl;
            fold(p);
            return;
        }
        // raise
        int number;
        if (iss >> number) {
            std::cout << "'r' command received. Raise amount is " << number << std::endl;
            if (number < p.chip) {
                raise(p, number, maxAmount, pot);
            }
            else if (number == p.chip) {
                std::cout << "'r' command received. All in." << std::endl;
                allin(p, maxAmount, pot);
            }
            else {
                std::cout << "'r' command received. Not enough chips. Fold." << std::endl;
                fold(p);
            }
        } else {
            std::cout << "Invalid 'r' command. Fold." << std::endl;
            fold(p);
        }
    } else if (command == "c") {
        if (decisions < 4) {
            std::cout << "'c' command Received. You can't call. Fold." << std::endl;
            fold(p);
            return;
        }
        // call
        std::cout << "'c' command received." << std::endl;
        call(p, maxAmount, pot);
    } else if (command == "f") {
        // fold
        std::cout << "'f' command received." << std::endl;
        fold(p);
    } else if (command == "allin") {
        // all in
        std::cout << "'allin' command received." << std::endl;
        allin(p, maxAmount, pot);
    } else {
        // 处理无效命令
        std::cout << "Invalid command. Fold." << std::endl;
        fold(p);
    }
}

void makeDecision_check(player& p, int& maxAmount, int& pot, std::string input) {
    // std::string input;
    // std::getline(std::cin, input);

    std::istringstream iss(input);
    std::string command;
    iss >> command;
    if (command == "r") {
        // raise
        int number;
        if (iss >> number) {
            std::cout << "'r' command received. Raise amount is " << number << std::endl;
            if (number < p.chip) {
                raise(p, number, maxAmount, pot);
                check_flag = false;
            }
            else if (number == p.chip) {
                std::cout << "'r' command received. All in." << std::endl;
                allin(p, maxAmount, pot);
                check_flag = false;
            }
            else {
                std::cout << "'r' command received. Not enough chips. Fold." << std::endl;
                fold(p);
            }
        } else {
            std::cout << "Invalid 'r' command. Fold." << std::endl;
            fold(p);
        }
    } else if (command == "f") {
        // fold
        std::cout << "'f' command received." << std::endl;
        fold(p);
    } else if (command == "allin") {
        // all in
        std::cout << "'allin' command received." << std::endl;
        allin(p, maxAmount, pot);
        check_flag = false;
    } else if (command == "check") {
        std::cout << "'check' command received." << std::endl;
        std::string s = username[p.playNum] + " 选择check，目前最高下注额为" + std::to_string(p.betAmount);
        broadcast(s);
        return;
    } else {
        // 处理无效命令
        std::cout << "Invalid command. Fold." << std::endl;
        fold(p);
    }
}

bool earlyFoldStop(int pot) {
    int players = playList.size(), count = 0;
    for (int i = 0; i < players; i ++) {
        if (!playList[i].isFolded) {
            count ++;
            if (count > 1) {
                return false;
            }
        }
    }
    std::string m = "仅剩一名玩家在游戏中，本轮提前结束\n收益情况\n";
    for (int i = 0; i < players; i ++) {
        if (!playList[i].isFolded) {
            playList[i].chip += pot;
            m += username[i] + " +" + std::to_string(playList[i].betAmount) + '\n';
        } else {
            m += username[i] + " -" + std::to_string(pot) + '\n';
        }
    }
    broadcast(m);
    return true;
}

void statistics() {
    std::string message = "\n统计信息\n";
    int l = playList.size();
    for (int i = 0; i < l; i ++) {
        if (playList[i].isInGame) {
            std::cout << "player" << i << " chip: " << playList[i].chip << std::endl;
            message += username[i] + "拥有筹码： " + std::to_string(playList[i].chip) + '\n';
        } else {
            message += username[i] + "已被移出游戏\n";
        }
    }
    broadcast(message);
}

int main() {
    srand((unsigned int) time(NULL));

    networkInit();
    getUsername();

    // 游戏人数和初始筹码设定
    int players_in_game = players, initial_chips = 50, sb_amount = 1, bb_amount = 2;
    // 游戏初始位置和小盲大盲数值
    int sb = 0, bb = 1, dealer = players - 1, nowPlayer;
    // 设置初始筹码
    for (int i = 0; i < players; i ++) {
        player p = player(initial_chips, i);
        playList.push_back(p);
    }
    
    // debugging
    // playList[0] = player{true, false, true, 38, 12};
    // playList[1] = player{true, false, true, 30, 20};
    // playList[2] = player{true, false, true, 30, 20};
    // playList[3] = player{false, true, true, 0, 40};
    // playList[4] = player{false, true, true, 0, 50};
    // f8fq(152, 0);
    // statistics();
    // return 0;

    while (true) {
        std::string m;
        m = "新一轮游戏开始，洗牌中";
        broadcast(m);

        TexasHoldemPoker poker;
        poker.shuffleDeck();

        // 每局游戏开始时更新最大投注额和底池
        int pot = 0, maxAmount = 0;
        // 发底牌
        playerholeCards.clear();
        for (int i = 0; i < players; i ++) {
            std::vector<Card> holeCards;
            if (!playList[i].isInGame) {
                playerholeCards.push_back(holeCards);
                continue;
            }
            for (int j = 0; j < 2; j ++) {
                holeCards.push_back(poker.dealCard());
            }
            playerholeCards.push_back(holeCards);
        }

        // 输出底牌
        for (int i = 0; i < players; i ++) {
            if (!playList[i].isInGame) {
                continue;
            }
            std::cout << "玩家" << i << "的底牌：" << std::endl;
            m = "您的底牌:";
            for (const Card& card : playerholeCards[i]) {
                std::cout << faceValueToString(card.faceValue) << " of " << suitToString(card.suit) << std::endl;
                m += '\n' + faceValueToString(card.faceValue) + " of " + suitToString(card.suit);
            }
            mySend(i, m);
        }

        // 小盲，大盲
        if (playList[sb].chip < sb_amount) {
            m = username[sb] + "的筹码不足以支付小盲，被移出游戏";
            broadcast(m);
            std::cout << "sb " << sb << " kick out" << std::endl;
            playList[sb].isInGame = false;
            players_in_game --;
            statistics();
            if (players_in_game == 1) {
                for (auto p : playList) {
                    if (p.isInGame) {
                        m = "最后的胜利者是" + username[p.playNum] + "\nGame Over\n服务器即将关闭...";
                        broadcast(m);
                        m = "quit";
                        broadcast(m);
                    }
                }
                std::cout << "Game Over\n";
                break;
            }
            updatePos(sb, bb, dealer, players);
            continue;
        }
        if (playList[bb].chip < bb_amount) {
            m = username[bb] + "的筹码不足以支付大盲，被移出游戏";
            broadcast(m);
            std::cout << "bb " << bb << " kick out" << std::endl;
            playList[bb].isInGame = false;
            players_in_game --;
            statistics();
            if (players_in_game == 1) {
                for (auto p : playList) {
                    if (p.isInGame) {
                        m = "最后的胜利者是" + username[p.playNum] + "\nGame Over\n服务器即将关闭...";
                        broadcast(m);
                        m = "quit";
                        broadcast(m);
                    }
                }
                std::cout << "Game Over\n";
                break;
            }
            updatePos(sb, bb, dealer, players);
            continue;
        }
        if (playList[sb].chip == sb_amount) {
            allin(playList[sb], maxAmount, pot);
        }
        else {
            raise(playList[sb], sb_amount, maxAmount, pot);
        }
        if (playList[bb].chip == bb_amount) {
            allin(playList[bb], maxAmount, pot);
        }
        else {
            raise(playList[bb], bb_amount - sb_amount, maxAmount, pot);
        }

        // 第一轮投注
        nowPlayer = (bb + 1) % players;
        while (!roundOver(maxAmount)) {
            std::string message;

            // 排除出局玩家
            if (!playList[nowPlayer].isInGame) {
                nowPlayer = (nowPlayer + 1) % players;
                continue;
            }
            // 跳过allin或fold的返家
            if (playList[nowPlayer].isAllin || playList[nowPlayer].isFolded) {
                nowPlayer = (nowPlayer + 1) % players;
                continue;
            }

            // debugging：输出当前轮次玩家基本信息
            std::cout << "玩家" << nowPlayer << " maxAmount: " << maxAmount << " Your bet: " << playList[nowPlayer].betAmount << " Your chips: " << playList[nowPlayer].chip << std::endl;
            
            message = "玩家 " + username[nowPlayer] + " 思考中...";
            broadcast_except(message, nowPlayer);
            message = "底池大小:" + std::to_string(pot) + " 当前最高投注额:" + std::to_string(maxAmount) + " 您的投注额:" + std::to_string(playList[nowPlayer].betAmount) + " 您的筹码:" + std::to_string(playList[nowPlayer].chip);
            mySend(nowPlayer, message);
            if (playList[nowPlayer].chip + playList[nowPlayer].betAmount <= maxAmount) {
                std::cout << "fold or allin" << std::endl;
                message = "选择：弃牌或all in";
                mySend(nowPlayer, message);
                myReceive(nowPlayer, message);
                makeDecision(2, playList[nowPlayer], maxAmount, pot, message);
            }
            // else if (playList[nowPlayer].chip + playList[nowPlayer].betAmount == maxAmount) {
            //     std::cout << "call or fold or allin" << std::endl;
            //     makeDecision(3, playList[nowPlayer], maxAmount, pot);
            // }
            else if (playList[nowPlayer].chip + playList[nowPlayer].betAmount > maxAmount) {
                std::cout << "raise or call or fold or allin" << std::endl;
                message = "选择：加注、跟注、弃牌或all in";
                mySend(nowPlayer, message);
                myReceive(nowPlayer, message);
                makeDecision(4, playList[nowPlayer], maxAmount, pot, message);
            }
            if (earlyFoldStop(pot)) {
                goto roundEnd;
            }
            nowPlayer = (nowPlayer + 1) % players;
            // debugging
            std::cout << "round over: " << roundOver(maxAmount) << std::endl;
        }

        // 发翻牌
        communityCards.clear();
        for (int i = 0; i < 3; i++) {
            communityCards.push_back(poker.dealCard());
        }

        // 输出翻牌
        std::cout << "翻牌：" << std::endl;
        m = "翻牌：";
        for (const Card& card : communityCards) {
            std::cout << faceValueToString(card.faceValue) << " of " << suitToString(card.suit) << std::endl;
            m += '\n' + faceValueToString(card.faceValue) + " of " + suitToString(card.suit);
        }
        broadcast(m);

        // 第二轮投注
        nowPlayer = sb;
        check_flag = true;
        check_count = 0;
        do {
            std::string message;

            if (nowPlayer == sb) {
                check_count ++;
            }
            if (check_count == 2 && check_flag) {
                break;
            }
            // 排除出局玩家
            if (!playList[nowPlayer].isInGame) {
                nowPlayer = (nowPlayer + 1) % players;
                continue;
            }
            // 跳过allin或fold的玩家
            if (playList[nowPlayer].isAllin || playList[nowPlayer].isFolded) {
                nowPlayer = (nowPlayer + 1) % players;
                continue;
            }

            // debugging：输出当前轮次玩家基本信息
            std::cout << "玩家" << nowPlayer + 1 << " maxAmount: " << maxAmount << " Your bet: " << playList[nowPlayer].betAmount << " Your chips: " << playList[nowPlayer].chip << std::endl;

            message = "玩家 " + username[nowPlayer] + " 思考中...";
            broadcast_except(message, nowPlayer);
            message = "当前最高投注额: " + std::to_string(maxAmount) + " 您的投注额: " + std::to_string(playList[nowPlayer].betAmount) + " 您的筹码: " + std::to_string(playList[nowPlayer].chip);
            mySend(nowPlayer, message);
            if (check_flag) {
                std::cout << "check or raise or fold or allin" << std::endl;
                message = "选择：check、加注、弃牌或all in";
                mySend(nowPlayer, message);
                myReceive(nowPlayer, message);
                makeDecision_check(playList[nowPlayer], maxAmount, pot, message);
            }
            else {
                if (playList[nowPlayer].chip + playList[nowPlayer].betAmount <= maxAmount) {
                    std::cout << "fold or allin" << std::endl;
                    message = "选择：弃牌或all in";
                    mySend(nowPlayer, message);
                    myReceive(nowPlayer, message);
                    makeDecision(2, playList[nowPlayer], maxAmount, pot, message);
                }
                else if (playList[nowPlayer].chip + playList[nowPlayer].betAmount > maxAmount) {
                    std::cout << "raise or call or fold or allin" << std::endl;
                    message = "选择：加注、跟注、弃牌或all in";
                    mySend(nowPlayer, message);
                    myReceive(nowPlayer, message);
                    makeDecision(4, playList[nowPlayer], maxAmount, pot, message);
                }
            }
            
            if (earlyFoldStop(pot)) {
                goto roundEnd;
            }
            nowPlayer = (nowPlayer + 1) % players;
            // debugging
            std::cout << "round over: " << roundOver(maxAmount) << ' ' << check_flag << std::endl;
        } while (!roundOver(maxAmount) || check_flag);

        // 发转牌
        communityCards.push_back(poker.dealCard());

        // 输出转牌
        std::cout << "转牌：" << std::endl;
        std::cout << faceValueToString(communityCards[3].faceValue) << " of " << suitToString(communityCards[3].suit) << std::endl;
        m = "转牌：" + faceValueToString(communityCards[3].faceValue) + " of " + suitToString(communityCards[3].suit);
        broadcast(m);

        // 第三轮投注
        nowPlayer = sb;
        check_flag = true;
        check_count = 0;
        do {
            std::string message;

            if (nowPlayer == sb) {
                check_count ++;
            }
            if (check_count == 2 && check_flag) {
                break;
            }
            // 排除出局玩家
            if (!playList[nowPlayer].isInGame) {
                nowPlayer = (nowPlayer + 1) % players;
                continue;
            }
            // 跳过allin或fold的玩家
            if (playList[nowPlayer].isAllin || playList[nowPlayer].isFolded) {
                nowPlayer = (nowPlayer + 1) % players;
                continue;
            }

            // debugging：输出当前轮次玩家基本信息
            std::cout << "玩家" << nowPlayer + 1 << " maxAmount: " << maxAmount << " Your bet: " << playList[nowPlayer].betAmount << " Your chips: " << playList[nowPlayer].chip << std::endl;

            message = "玩家 " + username[nowPlayer] + " 思考中...";
            broadcast_except(message, nowPlayer);
            message = "当前最高投注额: " + std::to_string(maxAmount) + " 您的投注额: " + std::to_string(playList[nowPlayer].betAmount) + " 您的筹码: " + std::to_string(playList[nowPlayer].chip);
            mySend(nowPlayer, message);
            if (check_flag) {
                std::cout << "check or raise or fold or allin" << std::endl;
                message = "选择：check、加注、弃牌或all in";
                mySend(nowPlayer, message);
                myReceive(nowPlayer, message);
                makeDecision_check(playList[nowPlayer], maxAmount, pot, message);
            }
            else {
                if (playList[nowPlayer].chip + playList[nowPlayer].betAmount <= maxAmount) {
                    std::cout << "fold or allin" << std::endl;
                    message = "选择：弃牌或all in";
                    mySend(nowPlayer, message);
                    myReceive(nowPlayer, message);
                    makeDecision(2, playList[nowPlayer], maxAmount, pot, message);
                }
                else if (playList[nowPlayer].chip + playList[nowPlayer].betAmount > maxAmount) {
                    std::cout << "raise or call or fold or allin" << std::endl;
                    message = "选择：加注、跟注、弃牌或all in";
                    mySend(nowPlayer, message);
                    myReceive(nowPlayer, message);
                    makeDecision(4, playList[nowPlayer], maxAmount, pot, message);
                }
            }
            
            if (earlyFoldStop(pot)) {
                goto roundEnd;
            }
            nowPlayer = (nowPlayer + 1) % players;
            // debugging
            std::cout << "round over: " << roundOver(maxAmount) << ' ' << check_flag << std::endl;
        } while (!roundOver(maxAmount) || check_flag);
        
        // 发河牌
        communityCards.push_back(poker.dealCard());

        // 输出河牌
        std::cout << "河牌：" << std::endl;
        std::cout << faceValueToString(communityCards[4].faceValue) << " of " << suitToString(communityCards[4].suit) << std::endl;
        m = "河牌：" + faceValueToString(communityCards[4].faceValue) + " of " + suitToString(communityCards[4].suit);
        broadcast(m);

        // 第四轮投注
        nowPlayer = sb;
        check_flag = true;
        check_count = 0;
        do {
            std::string message;

            if (nowPlayer == sb) {
                check_count ++;
            }
            if (check_count == 2 && check_flag) {
                break;
            }
            // 排除出局玩家
            if (!playList[nowPlayer].isInGame) {
                nowPlayer = (nowPlayer + 1) % players;
                continue;
            }
            // 跳过allin或fold的玩家
            if (playList[nowPlayer].isAllin || playList[nowPlayer].isFolded) {
                nowPlayer = (nowPlayer + 1) % players;
                continue;
            }

            // debugging：输出当前轮次玩家基本信息
            std::cout << "玩家" << nowPlayer + 1 << " maxAmount: " << maxAmount << " Your bet: " << playList[nowPlayer].betAmount << " Your chips: " << playList[nowPlayer].chip << std::endl;

            message = "玩家 " + username[nowPlayer] + " 思考中...";
            broadcast_except(message, nowPlayer);
            message = "当前最高投注额: " + std::to_string(maxAmount) + " 您的投注额: " + std::to_string(playList[nowPlayer].betAmount) + " 您的筹码: " + std::to_string(playList[nowPlayer].chip);
            mySend(nowPlayer, message);
            if (check_flag) {
                std::cout << "check or raise or fold or allin" << std::endl;
                message = "选择：check、加注、弃牌或all in";
                mySend(nowPlayer, message);
                myReceive(nowPlayer, message);
                makeDecision_check(playList[nowPlayer], maxAmount, pot, message);
            }
            else {
                if (playList[nowPlayer].chip + playList[nowPlayer].betAmount <= maxAmount) {
                    std::cout << "fold or allin" << std::endl;
                    message = "选择：弃牌或all in";
                    mySend(nowPlayer, message);
                    myReceive(nowPlayer, message);
                    makeDecision(2, playList[nowPlayer], maxAmount, pot, message);
                }
                else if (playList[nowPlayer].chip + playList[nowPlayer].betAmount > maxAmount) {
                    std::cout << "raise or call or fold or allin" << std::endl;
                    message = "选择：加注、跟注、弃牌或all in";
                    mySend(nowPlayer, message);
                    myReceive(nowPlayer, message);
                    makeDecision(4, playList[nowPlayer], maxAmount, pot, message);
                }
            }
            
            if (earlyFoldStop(pot)) {
                goto roundEnd;
            }
            nowPlayer = (nowPlayer + 1) % players;
            // debugging
            std::cout << "round over: " << roundOver(maxAmount) << ' ' << check_flag << std::endl;
        } while (!roundOver(maxAmount) || check_flag);

        f8fq(pot, dealer);

        roundEnd:
        statistics();
        // 检测穷光蛋
        for (player& p : playList) {
            if (p.chip == 0) {
                p.isInGame = false; 
                players_in_game --;
            }
            else {
                p.reset();
            }
        }
        if (players_in_game == 1) {
            for (auto p : playList) {
                if (p.isInGame) {
                    m = "最后的胜利者是" + username[p.playNum] + "\nGame Over\n服务器即将关闭...";
                    broadcast(m);
                    m = "quit";
                    broadcast(m);
                }
            }
            std::cout << "Game Over\n";
            break;
        }
        updatePos(sb, bb, dealer, players);
        std::cout << "sb " << sb << "bb " << bb << "dealer " << dealer << "players " << players << std::endl;
    }
    
    // // 比较大小
    // int rk = comparePlayers(playerholeCards[0], playerholeCards[1], communityCards);
    
    // // std::vector<Card> t1, t2, tc;
    // // t1.push_back({NINE, DIAMONDS});
    // // t1.push_back({THREE, SPADES});
    // // t2.push_back({JACK, SPADES});
    // // t2.push_back({SIX, DIAMONDS});
    // // tc.push_back({QUEEN, SPADES});
    // // tc.push_back({QUEEN, HEARTS});
    // // tc.push_back({QUEEN, DIAMONDS});
    // // tc.push_back({QUEEN, HEARTS});
    // // tc.push_back({SEVEN, DIAMONDS});

    // // int rk = comparePlayers(t1, t2, tc);

    // // std::cout << "P1:" << std::endl;
    // // for (const Card& card : t1) {
    // //     std::cout << faceValueToString(card.faceValue) << " of " << suitToString(card.suit) << std::endl;
    // // }
    // // std::cout << "P2:" << std::endl;
    // // for (const Card& card : t2) {
    // //     std::cout << faceValueToString(card.faceValue) << " of " << suitToString(card.suit) << std::endl;
    // // }

    // // std::cout << "公共:" << std::endl;
    // // for (const Card& card : tc) {
    // //     std::cout << faceValueToString(card.faceValue) << " of " << suitToString(card.suit) << std::endl;
    // // }

    // if (rk == -1) {
    //     std::cout << "player2 wins" << std::endl;
    // }
    // else if (rk == 1) {
    //     std::cout << "player1 wins" << std::endl;
    // }
    // else {
    //     std::cout << "DRAW" << std::endl;
    // }

    return 0;
}
