#include <string>

enum FaceValue { TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, JACK, QUEEN, KING, ACE };
enum Suit { HEARTS, DIAMONDS, CLUBS, SPADES };

struct Card {
    FaceValue faceValue;
    Suit suit;
};

std::string suitToString(Suit suit);
std::string faceValueToString(FaceValue faceValue);