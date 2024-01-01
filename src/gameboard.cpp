#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <iostream>

using namespace std;


//this is for server
/*
    To Do:
        Player check banckrupt
        Card.execute()
        Field.execute()

        

*/

struct WaitingRoom {
    int playerNum;
    string playerNames[8];
    int sockfds[8];
};



class Card {    // 機會 or 命運卡
    public:
        Card(): cardType(0) {}
        Card(int cardType): cardType(cardType) {}
        void execute() {
            switch (this->cardType) {
                case 1:
                    break;
                case 2:
                    break;
                case 3:
                    break;
                default:
                    break;
            }
        }
    private:
        int cardType; // 0:Empty
};


class Player {
    public:
        Player(): id(0), name(""), money(1500), sockfd(0), lastMove(0) {this->cards = vector<Card>();}
        Player(int id, string name, int sockfd): id(id), name(name), money(1500), sockfd(sockfd), lastMove(0) {this->cards = vector<Card>();}
        void drawCard(int cardType) {
            this->cards.push_back(Card(cardType));
        }
        void useCard(int n) {
            //this->cards[n].execute();
            this->cards[n] = this->cards[this->cards.size()];
            this->cards.pop_back();
        }
        int getId() {return this->id;}
        string getName() {return this->name;}
        int getMoney() {return this->money;}
        int getSockfd() {return this->sockfd;}
        vector<Card> &getCards() {return this->cards;}
        int getLastMove() {return this->lastMove;}
        void earn(int n) {
            this->money += n;
        }
        void pay(int n) {
            this->money -= n;
            if (this->money <= 0) {
                /*check bankrupt*/
            }
        }
    private:
        int id;
        string name;
        int money;
        int sockfd;
        vector<Card> cards;
        int lastMove;
};


struct RentInfo {
    int cost = 0;       // 地價
    int rent = 0;       // 空地過路費
    int house1 = 0;     // 房屋過路費
    int house2 = 0;
    int house3 = 0;
    int house4 = 0;
    int hotel = 0;      // 旅館過路費
    int houseCost = 0;  // 蓋房費用
};


struct Dice {
    int d1;
    int d2;
    static Dice roll() {
        int end = RAND_MAX / 6;
        end *= 6;
        int d1, d2;
        while ((d1 = rand()) >= end);
        while ((d2 = rand()) >= end);
        return {d1%6, d2%6};
    }

};


#define STATION_COST 200
#define STATION_RENT1 25
#define STATION_RENT2 50
#define STATION_RENT3 100
#define STATION_RENT4 200

#define UTILITY_COST 150
#define UTILITY_MULT1 4
#define UTILITY_MULT2 10


class Field {   // 格子
    public:
        Field(): type(0), name(""), owner(nullptr), house(0), color(0), siblingNum(0), mortgage(0), rentInfo({0,0,0,0,0,0,0,0}) {fill_n(this->siblings, 3, nullptr);}
        Field(int type, string name): type(type), name(name), owner(nullptr), house(0), color(0), siblingNum(0), mortgage(0), rentInfo({0,0,0,0,0,0,0,0}) {fill_n(this->siblings, 3, nullptr);}
        Field(int type, string name, int tax): type(type), name(name), owner(nullptr), house(0), color(0), siblingNum(0), mortgage(0), rentInfo({0,tax,0,0,0,0,0,0}) {fill_n(this->siblings, 3, nullptr);}
        Field(int type, string name, int color, RentInfo rentInfo): type(type), name(name), owner(nullptr), house(0), color(color), siblingNum(0), mortgage(0), rentInfo(rentInfo) {fill_n(this->siblings, 3, nullptr);}
        void setSibling(Field *sib) {this->siblings[this->siblingNum++] = sib;}
        RentInfo &getRentInfo() {return this->rentInfo;}
        int getTax() {return ((this->type == 9) ? this->rentInfo.rent : 0);}
        void execute(Player *player) {
            // 執行動作
            switch (this->type) {
                case 1:
                    player->earn(200);
                    cout << player->getName() << " 經過起點, 獲得200$\n";
                    break;
                case 2:
                    if (this->owner == nullptr) {
                        /*check buy field*/
                    } else if (this->owner == player) {
                        /*chech build house*/
                    } else {
                        /*pay rent*/
                    }
                    break;
                case 3:
                    if (this->owner == nullptr) {
                        /*check buy field*/
                    } else if (this->owner != player) {
                        /*pay rent*/
                    }
                    break;
                case 4:
                    if (this->owner == nullptr) {
                        /*check buy field*/
                    } else if (this->owner != player) {
                        /*pay rent*/
                    }
                    break;
                case 5:
                    /*draw chance*/
                    break;
                case 6:
                    /*draw destiny*/
                    break;
                case 7:
                case 8:
                case 9:
                    player->pay(this->getTax());
                    cout << player->getName() << " 支付 " << this->name << " " << this->getTax() << "$\n";
                    


                default:
                    break;
            }
            
        }
    private:
        int type; // 0:Empty 1:起點 2:土地 3:車站 4:公共事業 5:機會 6:命運 7:入獄 8:監獄 9:稅
        string name; // for all
        Player *owner; // for type 2, 3, 4
        int house; // for type 2
        int color; // 0:brown 1:skyblue 2:pink 3:orange 4:red 5:yellow 6:green 7:blue for type 2
        int siblingNum; // 同顏色的地or車站的數量 for type 2, 3
        Field *siblings[3]; // 同顏色的地or車站 for type 2, 3
        int mortgage; // 0:沒事 1:抵押中
        RentInfo rentInfo;
};


class Gameboard {   // 遊戲盤 aka 整個遊戲（包括銀行、玩家、場地）
    public:
        Gameboard(): playerNum(8) {
            this->players = new Player[8];
            this->bankruptStat = new int[8]{0};
            this->fields = new Field[40];
        }
        Gameboard(int playerNum, string *playerNames, int *sockfds): playerNum(playerNum) {
            this->players = new Player[playerNum];
            this->bankruptStat = new int[playerNum]{0};
            this->fields = new Field[40];
            for (int i = 0; i < playerNum; i++) {
                this->players[i] = Player(i, playerNames[i], sockfds[i]);
            }
        }
        ~Gameboard() {
            delete[] this->players;
            delete[] this->bankruptStat;
            delete[] this->fields;
        }
        void setField(int num, int type, string name) {
            this->fields[num] = Field(type, name);
        }
        void setField(int num, int type, string name, int tax) {
            this->fields[num] = Field(type, name, tax);
        }
        void setField(int num, int type, string name, int color, RentInfo rentInfo) {
            this->fields[num] = Field(type, name, color, rentInfo);
        }
        Field& getField(int num) {
            return (this->fields[num]);
        }
    private:
        int playerNum;
        Player *players;
        int *bankruptStat;
        Field *fields;
};




Gameboard *createGame (WaitingRoom *room) {
    Gameboard* board = new Gameboard(room->playerNum, room->playerNames, room->sockfds);
    // 0:Empty 1:起點 2:土地 3:車站 4:公共事業 5:機會 6:命運 7:入獄 8:監獄 9:稅
    // 0:brown 1:skyblue 2:pink 3:orange 4:red 5:yellow 6:green 7:blue
    board->setField(0, 1, "起點");
    board->setField(1, 2, "Brown 1", 0, {60,2,10,30,90,160,250,50});
    board->setField(2, 6, "命運");
    board->setField(3, 2, "Brown 2", 0, {60,4,20,60,180,320,450,50});
    board->setField(4, 9, "所得稅", 200);
    board->setField(5, 3, "Train 1");
    board->setField(6, 2, "Skyblue 1", 1, {100,6,30,90,270,400,550,50});
    board->setField(7, 5, "機會");
    board->setField(8, 2, "Skyblue 2", 1, {100,6,30,90,270,400,550,50});
    board->setField(9, 2, "Skyblue 3", 1, {120,8,40,100,300,450,600,50});
    board->setField(10, 8, "監獄");
    board->setField(11, 2, "Pink 1", 2, {140,10,50,150,450,625,750,100});
    board->setField(12, 4, "電力公司");
    board->setField(13, 2, "Pink 2", 2, {140,10,50,150,450,625,750,100});
    board->setField(14, 2, "Pink 3", 2, {160,12,60,180,500,700,900,100});
    board->setField(15, 3, "Train 2");
    board->setField(16, 2, "Orange 1", 3, {180,14,70,200,550,750,950,100});
    board->setField(17, 6, "命運");
    board->setField(18, 2, "Orange 2", 3, {180,14,70,200,550,750,950,100});
    board->setField(19, 2, "Orange 3", 3, {200,16,80,220,600,800,1000,100});
    board->setField(20, 0, "免費停車");
    board->setField(21, 2, "Red 1", 4, {220,18,90,250,700,875,1050,150});
    board->setField(22, 5, "機會");
    board->setField(23, 2, "Red 2", 4, {220,18,90,250,700,875,1050,150});
    board->setField(24, 2, "Red 3", 4, {240,20,100,300,750,925,1100,150});
    board->setField(25, 3, "Train 3");
    board->setField(26, 2, "Yellow 1", 5, {260,22,110,330,800,975,1150,150});
    board->setField(27, 2, "Yellow 2", 5, {260,22,110,330,800,975,1150,150});
    board->setField(28, 4, "自來水公司");
    board->setField(29, 2, "Yellow 3", 5, {280,24,120,360,850,1025,1200,150});
    board->setField(30, 7, "入獄");
    board->setField(31, 2, "Green 1", 6, {300,26,130,390,900,1100,1275,200});
    board->setField(32, 2, "Green 2", 6, {300,26,130,390,900,1100,1275,200});
    board->setField(33, 6, "命運");
    board->setField(34, 2, "Green 3", 6, {320,28,150,450,1000,1200,1400,200});
    board->setField(35, 3, "Train 4");
    board->setField(36, 5, "機會");
    board->setField(37, 2, "Blue 1", 7, {350,35,175,500,1100,1300,1500,200});
    board->setField(38, 9, "奢侈稅", 100);
    board->setField(39, 2, "Blue 2", 7, {400,50,200,600,1400,1700,2000,200});

    board->getField(1).setSibling(&(board->getField(3)));
    board->getField(3).setSibling(&(board->getField(1)));
    board->getField(6).setSibling(&(board->getField(8)));
    board->getField(6).setSibling(&(board->getField(9)));
    board->getField(8).setSibling(&(board->getField(6)));
    board->getField(8).setSibling(&(board->getField(9)));
    board->getField(9).setSibling(&(board->getField(6)));
    board->getField(9).setSibling(&(board->getField(8)));
    board->getField(11).setSibling(&(board->getField(13)));
    board->getField(11).setSibling(&(board->getField(14)));
    board->getField(13).setSibling(&(board->getField(11)));
    board->getField(13).setSibling(&(board->getField(14)));
    board->getField(14).setSibling(&(board->getField(11)));
    board->getField(14).setSibling(&(board->getField(13)));
    board->getField(16).setSibling(&(board->getField(18)));
    board->getField(16).setSibling(&(board->getField(19)));
    board->getField(18).setSibling(&(board->getField(16)));
    board->getField(18).setSibling(&(board->getField(19)));
    board->getField(19).setSibling(&(board->getField(16)));
    board->getField(19).setSibling(&(board->getField(18)));
    board->getField(21).setSibling(&(board->getField(23)));
    board->getField(21).setSibling(&(board->getField(24)));
    board->getField(23).setSibling(&(board->getField(21)));
    board->getField(23).setSibling(&(board->getField(24)));
    board->getField(24).setSibling(&(board->getField(21)));
    board->getField(24).setSibling(&(board->getField(23)));
    board->getField(26).setSibling(&(board->getField(27)));
    board->getField(26).setSibling(&(board->getField(29)));
    board->getField(27).setSibling(&(board->getField(26)));
    board->getField(27).setSibling(&(board->getField(29)));
    board->getField(29).setSibling(&(board->getField(26)));
    board->getField(29).setSibling(&(board->getField(27)));
    board->getField(31).setSibling(&(board->getField(32)));
    board->getField(31).setSibling(&(board->getField(34)));
    board->getField(32).setSibling(&(board->getField(31)));
    board->getField(32).setSibling(&(board->getField(34)));
    board->getField(34).setSibling(&(board->getField(31)));
    board->getField(34).setSibling(&(board->getField(32)));
    board->getField(37).setSibling(&(board->getField(39)));
    board->getField(39).setSibling(&(board->getField(37)));

    board->getField(5).setSibling(&(board->getField(15)));
    board->getField(5).setSibling(&(board->getField(25)));
    board->getField(5).setSibling(&(board->getField(35)));
    board->getField(15).setSibling(&(board->getField(5)));
    board->getField(15).setSibling(&(board->getField(25)));
    board->getField(15).setSibling(&(board->getField(35)));
    board->getField(25).setSibling(&(board->getField(5)));
    board->getField(25).setSibling(&(board->getField(15)));
    board->getField(25).setSibling(&(board->getField(35)));
    board->getField(35).setSibling(&(board->getField(5)));
    board->getField(35).setSibling(&(board->getField(15)));
    board->getField(35).setSibling(&(board->getField(25)));
    
    board->getField(12).setSibling(&(board->getField(28)));
    board->getField(28).setSibling(&(board->getField(12)));



    return board;    
};