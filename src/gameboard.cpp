#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <iostream>

extern "C" { 
    #include "unp.h"
} 

using namespace std;


//this is for server
/*
    To Do:
        監獄、入獄
        Player check banckrupt
        Card.execute()
        Field.execute()
    

    測試版:
        經過起點只有1$ (Field::execute())
        一般土地過路費*2 (Field::calcRent())


*/

class Gameboard;
class Player;

struct WaitingRoom {
    int playerNum;
    string playerNames[8];
    int sockfds[8];
};



class Card {    // 機會 or 命運卡
    /*
        目前想法：
            賺錢
            付錢
            入獄
            免刑卡（可保留）
            今天我生日（向所有人收錢）
            對所有人付錢
            共產主義當道（所有人現金平分）
            往前走指定步數
            移動到特定地點（起點、最近的公營事業or車站、最貴的那塊地...）
            指定骰子步數（可保留，之後擲骰前用）
            免付過路費1次（可保留）
            拆房子
            根據房屋數量繳稅
    */
    public:
        Card(): cardType(0), effect(0), keep(0), desciption("一張卡") {}
        Card(int cardType, int effect, int keep, string descr): cardType(cardType), effect(effect), keep(keep), desciption(descr) {}
        void execute(Player *player);
        int canKeep() const {return this->keep;}
        string getDesciption() const {return this->desciption;}
    private:
        int cardType = 0; // 0:機會 1:命運
        int effect = 0; // 0:Empty
        int keep = 0; // 0:不可保留 1:可保留
        string desciption = "一張卡";
};

#define CHANCE_CARD_TYPE_NUM 3
#define DESTINY_CARD_TYPE_NUM 3

Card randCard(int cardType) {
    int effect = 0;
    int keep = 0;
    string descr = "";
    if (cardType) {
        effect = rand();
        effect %= DESTINY_CARD_TYPE_NUM;
        effect += 1;
        switch (effect) {
            case 1:
                descr = "第一種命運，不可保留";
                break;
            case 2:
                descr = "第二種命運，不可保留";
                break;
            case 3:
                descr = "第三種命運，可保留";
                keep = 1;
                break;
            default:
                break;
        }
        
    } else {
        effect = rand();
        effect %= CHANCE_CARD_TYPE_NUM;
        effect += 1;
        switch (effect) {
            case 1:
                descr = "第一種機會，可保留";
                keep = 1;
                break;
            case 2:
                descr = "第二種機會，不可保留";
                break;
            case 3:
                descr = "第三種機會，不可保留";
                break;
            default:
                break;
        }
    }
    return Card(cardType, effect, keep, descr);
}


struct Dice {
    int d1 = 0;
    int d2 = 0;
};

Dice roll() {
    int d1, d2;
    d1 = rand();
    d2 = rand();
    d1 = (d1 % 6) + 1;
    d2 = (d2 % 6) + 1;
    cout << "骰子擲出 " << d1 << ", " << d2 << "\n";
    return {d1, d2};
}

class Player {
    public:
        Player(): id(0), name(""), sockfd(0) {this->cards = vector<Card>();}
        Player(int id, string name, int sockfd, Gameboard *board): id(id), name(name), sockfd(sockfd), gameboard(board) {this->cards = vector<Card>();}
        void keepCard(Card card) {
            this->cards.push_back(card);
        }
        void useCard(int n) {
            this->cards[n].execute(this);
            this->cards[n] = this->cards[this->cards.size()];
            this->cards.pop_back();
        }
        int getId() const {return this->id;}
        string getName() const {return this->name;}
        int getMoney() const {return this->money;}
        int getSockfd() const {return this->sockfd;}
        vector<Card> &getCards() {return this->cards;}
        Dice getLastMove() const {return this->lastMove;}
        int getPosition() const {return this->position;}
        void move(Dice dice) {
            this->lastMove = dice;
            this->position += dice.d1 + dice.d2;
            if (this->position >39) {
                this->position %= 40;
                this->passedStart = 1;
            }
        }
        int getPassedStart() const {return this->passedStart;}
        void resetPassedStart() {this->passedStart = 0;}
        void earn(int n) {
            this->money += n;
        }
        void pay(int n) {
            this->money -= n;
            if (this->money <= 0) {
                /*check bankrupt  待修改*/
                this->declareBankrupt();
            }
        }
        void payToPlayer(Player *other, int n) {
            other->earn(n);
            cout << this->name << " 付給 " << other->name << " " << n << "$\n";
            this->pay(n);
        }
        void declareBankrupt();
    private:
        int id;
        string name;
        int money = 1500;
        int sockfd;
        vector<Card> cards;
        Dice lastMove = {0, 0};
        int position = 0;
        int passedStart = 0;
        Gameboard *gameboard = nullptr;
};


struct RentInfo {
    int cost = 0;       // 地價
    int rent = 0;       // 空地過路費, 火車站*1, 公營事業*1的倍數
    int house1 = 0;     // 房屋過路費, 火車站*2, 公營事業*2的倍數
    int house2 = 0;     // 房屋過路費, 火車站*3
    int house3 = 0;     // 房屋過路費, 火車站*4
    int house4 = 0;
    int hotel = 0;      // 旅館過路費
    int houseCost = 0;  // 蓋房費用
};





class Field {   // 格子
    public:
        Field(): type(0), name("") {fill_n(this->siblings, 3, nullptr);}
        Field(int type, string name): type(type), name(name) {fill_n(this->siblings, 3, nullptr);}
        Field(int type, string name, int tax): type(type), name(name), rentInfo({0,tax,0,0,0,0,0,0}) {fill_n(this->siblings, 3, nullptr);}
        Field(int type, string name, int color, RentInfo rentInfo): type(type), name(name), color(color), rentInfo(rentInfo) {fill_n(this->siblings, 3, nullptr);}
        void setSibling(Field *sib) {this->siblings[this->siblingNum++] = sib;}
        RentInfo getRentInfo() const {return this->rentInfo;}
        int getTax() const {return ((this->type == 9) ? this->rentInfo.rent : 0);}
        string getName() const {return this->name;}
        Player* getOwner() {return this->owner;}
        void execute(Player *player) {
            // 執行動作
            switch (this->type) {
                // 0:Empty 1:起點 2:土地 3:車站 4:公共事業 5:機會 6:命運 7:入獄 8:監獄 9:稅
                case 1:
                    player->earn(1);
                    cout << player->getName() << " 經過起點, 獲得 1 $\n";
                    break;
                case 2:
                    if (this->owner == nullptr) {
                        checkBuy(player);
                    } else if (this->owner == player) {
                        checkBuildHouse(player);
                    } else {
                        player->payToPlayer(this->owner, this->calcRent());
                    }
                    break;
                case 3:
                    if (this->owner == nullptr) {
                        checkBuy(player);
                    } else if (this->owner != player) {
                        player->payToPlayer(this->owner, this->calcRailwayRent());
                    }
                    break;
                case 4:
                    if (this->owner == nullptr) {
                        checkBuy(player);
                    } else if (this->owner != player) {
                        player->payToPlayer(this->owner, this->calcUtilityRent(player));
                    }
                    break;
                case 5: {
                    Card card = randCard(0);
                    cout << player->getName() << "抽到 \"" << card.getDesciption() << "\"\n";
                    if (card.canKeep()) {
                        player->keepCard(card);
                    } else {
                        card.execute(player);
                    }
                    break;
                }
                case 6: {
                    Card card = randCard(1);
                    cout << player->getName() << "抽到 \"" << card.getDesciption() << "\"\n";
                    if (card.canKeep()) {
                        player->keepCard(card);
                    } else {
                        card.execute(player);
                    }
                    break;
                }
                case 7:
                    break;
                case 8:
                    break;
                case 9:
                    player->pay(this->getTax());
                    cout << player->getName() << " 支付 " << this->name << " " << this->getTax() << "$\n";
                    break;
                default:
                    break;
            }
            
        }
        void checkBuy(Player *player) {
            if (player->getMoney() >= this->rentInfo.cost) {
                int buy;
                cout << "是否購買 " << this->name << " (" << this->rentInfo.cost << "$) ? 0:no 1:yes\n";
                cin >> buy;
                while ((buy != 0) && (buy != 1)) {
                    cout << "invalid input\n";
                    cin >> buy;
                }
                if (buy == 1) {
                    player->pay(this->rentInfo.cost);
                       this->owner = player;
                }
            } else {
                cout << "金錢不足，無法購買\n";
            }
        }
        void checkBuildHouse(Player *player) {
            if (this->house < 5) {
                if (player->getMoney() >= this->rentInfo.houseCost) {
                    int buy;
                    cout << "是否在 " << this->name << ((this->house == 4) ? "蓋旅館 (" : "蓋房子 (") << this->rentInfo.houseCost << "$) ? 0:no 1:yes\n";
                    cin >> buy;
                    while ((buy != 0) && (buy != 1)) {
                        cout << "invalid input\n";
                        cin >> buy;
                    }
                    if (buy == 1) {
                        player->pay(this->rentInfo.houseCost);
                        this->house++;
                    }
                } else {
                    cout << "金錢不足，無法蓋" << ((this->house == 4) ? "旅館\n" : "房子\n");
                }
            }
            
        }
        int calcRent() {
            int sameColor = 0;
            int mult = 2;
            for (int i = 0; i < this->siblingNum; i++) {
                if (this->siblings[i]->owner == this->owner) {
                    sameColor++;
                }
            }

            if (sameColor == this->siblingNum) {
                mult = 4;
            }
            switch (this->house) {
                case 0:
                    return this->rentInfo.rent * mult;
                case 1:
                    return this->rentInfo.house1 * mult;
                case 2:
                    return this->rentInfo.house2 * mult;
                case 3:
                    return this->rentInfo.house3 * mult;
                case 4:
                    return this->rentInfo.house4 * mult;
                case 5:
                    return this->rentInfo.hotel * mult;
                default:
                    return 0;
            }
            return 0;
        }
        int calcRailwayRent() {
            int sameColor = 0;
            for (int i = 0; i < this->siblingNum; i++) {
                if (this->siblings[i]->owner == this->owner) {
                    sameColor++;
                }
            }
            switch (sameColor) {
                case 0:
                    return this->rentInfo.rent;
                case 1:
                    return this->rentInfo.house1;
                case 2:
                    return this->rentInfo.house2;
                case 3:
                    return this->rentInfo.house3;
                default:
                    return 25;
            }
            return 25;
        }
        int calcUtilityRent(Player *player) {
            int move = player->getLastMove().d1 + player->getLastMove().d2;
            int sameColor = 0;
            if (this->siblings[0]->owner == this->owner) {
                sameColor = 1;
            }
            if (sameColor == 1) {
                return move * this->rentInfo.house1;
            }
            return move * this->rentInfo.rent;
        }
    private:
        int type; // 0:Empty 1:起點 2:土地 3:車站 4:公共事業 5:機會 6:命運 7:入獄 8:監獄 9:稅
        string name; // for all
        Player *owner = nullptr; // for type 2, 3, 4
        int house = 0; // for type 2, if house == 5, it's hotel
        int color = 0; // 0:brown 1:skyblue 2:pink 3:orange 4:red 5:yellow 6:green 7:blue for type 2
        int siblingNum = 0; // 同顏色的地or車站的數量 for type 2, 3
        Field *siblings[3]; // 同顏色的地or車站 for type 2, 3
        int mortgage = 0; // 0:沒事 1:抵押中
        RentInfo rentInfo = {0,0,0,0,0,0,0,0};
};


class Gameboard {   // 遊戲盤 aka 整個遊戲（包括銀行、玩家、場地）
    public:
        Gameboard(): playerNum(8) {
            this->players = new Player[8];
            this->bankruptStat = new int[8]{0};
            this->fields = new Field[40];
            this->initGame();
        }
        Gameboard(WaitingRoom *room): playerNum(room->playerNum) {
            this->players = new Player[room->playerNum];
            this->bankruptStat = new int[room->playerNum];
            this->fields = new Field[40];
            for (int i = 0; i < room->playerNum; i++) {
                this->bankruptStat[i] = 0;
                this->players[i] = Player(i, (room->playerNames)[i], (room->sockfds)[i], this);
                cout << (room->playerNames)[i] << " 加入遊戲\n";
            }
            this->initGame();
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
        Field* getField(int num) {
            return &(this->fields[num]);
        }
        Player* getTurnPlayer() {
            return &(this->players[this->turnPlayer]);
        }
        int getTurnPlayerNum() {
            return this->turnPlayer;
        }
        int isEnded() const {
            return this->end;
        }
        void nextTurn() {
            if (this->checkEnd() > 0) {
                do {
                    if (this->turnPlayer >= this->playerNum-1) {
                        this->turnPlayer = 0;
                    } else {
                        this->turnPlayer++;
                    }
                } while (bankruptStat[this->turnPlayer]);
                cout << "\n\n現在是 " << this->players[this->turnPlayer].getName() << " 的回合\n";
                cout << "現在狀況: ";
                for (int i = 0; i < this->playerNum; i++) {
                    if (this->bankruptStat[i] == 0) {
                        cout << this->players[i].getName() << ": " << this->players[i].getMoney() << " $   ";
                    }
                } 
                cout << "\n";
            }
        }
        int checkEnd() { // return 還沒破產的人數
            int n = 0;
            for (int i = 0; i < this->playerNum; i++) {
                if (this->bankruptStat[i] == 0) {
                    n++;
                }
            }
            if (n <= 1) {
                this->end = 1;
            }
            return n;
        }
        string winner() {
            if (this->checkEnd() <= 1) {
                int n;
                for (n = 0; n < this->playerNum; n++) {
                    if (this->bankruptStat[n] == 0) {
                        return this->players[n].getName();
                    }
                }
            }
            return "";
        }
        void setBankrupt(int id) {
            this->bankruptStat[id] = 1;
            cout << this->players[id].getName() << " 破產了\n";
        }
        void printTrunPlayerMove() {
            Dice lastMove = this->getTurnPlayer()->getLastMove();
            cout << this->getTurnPlayer()->getName() << " 走了 " << lastMove.d1 + lastMove.d2 << " 步，來到 " << this->fields[this->getTurnPlayer()->getPosition()].getName() << "\n";
        }
        void checkPassStart() {
            if (this->getTurnPlayer()->getPassedStart()) {
                this->fields[0].execute(this->getTurnPlayer());
                this->getTurnPlayer()->resetPassedStart();
            }
        }
        int turnPlayerBankrupt() {
            return this->bankruptStat[this->turnPlayer];
        }

        void initGame () {
            // 0:Empty 1:起點 2:土地 3:車站 4:公共事業 5:機會 6:命運 7:入獄 8:監獄 9:稅
            // 0:brown 1:skyblue 2:pink 3:orange 4:red 5:yellow 6:green 7:blue
            this->setField(0, 1, "起點");
            this->setField(1, 2, "Brown 1", 0, {60,2,10,30,90,160,250,50});
            this->setField(2, 6, "命運");
            this->setField(3, 2, "Brown 2", 0, {60,4,20,60,180,320,450,50});
            this->setField(4, 9, "所得稅", 200);
            this->setField(5, 3, "Train 1", 0, {200,25,50,100,200,0,0,0});
            this->setField(6, 2, "Skyblue 1", 1, {100,6,30,90,270,400,550,50});
            this->setField(7, 5, "機會");
            this->setField(8, 2, "Skyblue 2", 1, {100,6,30,90,270,400,550,50});
            this->setField(9, 2, "Skyblue 3", 1, {120,8,40,100,300,450,600,50});
            this->setField(10, 8, "監獄");
            this->setField(11, 2, "Pink 1", 2, {140,10,50,150,450,625,750,100});
            this->setField(12, 4, "電力公司", 0, {150,4,10,0,0,0,0,0});
            this->setField(13, 2, "Pink 2", 2, {140,10,50,150,450,625,750,100});
            this->setField(14, 2, "Pink 3", 2, {160,12,60,180,500,700,900,100});
            this->setField(15, 3, "Train 2", 0, {200,25,50,100,200,0,0,0});
            this->setField(16, 2, "Orange 1", 3, {180,14,70,200,550,750,950,100});
            this->setField(17, 6, "命運");
            this->setField(18, 2, "Orange 2", 3, {180,14,70,200,550,750,950,100});
            this->setField(19, 2, "Orange 3", 3, {200,16,80,220,600,800,1000,100});
            this->setField(20, 0, "免費停車");
            this->setField(21, 2, "Red 1", 4, {220,18,90,250,700,875,1050,150});
            this->setField(22, 5, "機會");
            this->setField(23, 2, "Red 2", 4, {220,18,90,250,700,875,1050,150});
            this->setField(24, 2, "Red 3", 4, {240,20,100,300,750,925,1100,150});
            this->setField(25, 3, "Train 3", 0, {200,25,50,100,200,0,0,0});
            this->setField(26, 2, "Yellow 1", 5, {260,22,110,330,800,975,1150,150});
            this->setField(27, 2, "Yellow 2", 5, {260,22,110,330,800,975,1150,150});
            this->setField(28, 4, "自來水公司", 0, {150,4,10,0,0,0,0,0});
            this->setField(29, 2, "Yellow 3", 5, {280,24,120,360,850,1025,1200,150});
            this->setField(30, 7, "入獄");
            this->setField(31, 2, "Green 1", 6, {300,26,130,390,900,1100,1275,200});
            this->setField(32, 2, "Green 2", 6, {300,26,130,390,900,1100,1275,200});
            this->setField(33, 6, "命運");
            this->setField(34, 2, "Green 3", 6, {320,28,150,450,1000,1200,1400,200});
            this->setField(35, 3, "Train 4", 0, {200,25,50,100,200,0,0,0});
            this->setField(36, 5, "機會");
            this->setField(37, 2, "Blue 1", 7, {350,35,175,500,1100,1300,1500,200});
            this->setField(38, 9, "奢侈稅", 100);
            this->setField(39, 2, "Blue 2", 7, {400,50,200,600,1400,1700,2000,200});

            this->fields[1].setSibling(&(this->fields[3]));
            this->fields[3].setSibling(&(this->fields[1]));
            this->fields[6].setSibling(&(this->fields[8]));
            this->fields[6].setSibling(&(this->fields[9]));
            this->fields[8].setSibling(&(this->fields[6]));
            this->fields[8].setSibling(&(this->fields[9]));
            this->fields[9].setSibling(&(this->fields[6]));
            this->fields[9].setSibling(&(this->fields[8]));
            this->fields[11].setSibling(&(this->fields[13]));
            this->fields[11].setSibling(&(this->fields[14]));
            this->fields[13].setSibling(&(this->fields[11]));
            this->fields[13].setSibling(&(this->fields[14]));
            this->fields[14].setSibling(&(this->fields[11]));
            this->fields[14].setSibling(&(this->fields[13]));
            this->fields[16].setSibling(&(this->fields[18]));
            this->fields[16].setSibling(&(this->fields[19]));
            this->fields[18].setSibling(&(this->fields[16]));
            this->fields[18].setSibling(&(this->fields[19]));
            this->fields[19].setSibling(&(this->fields[16]));
            this->fields[19].setSibling(&(this->fields[18]));
            this->fields[21].setSibling(&(this->fields[23]));
            this->fields[21].setSibling(&(this->fields[24]));
            this->fields[23].setSibling(&(this->fields[21]));
            this->fields[23].setSibling(&(this->fields[24]));
            this->fields[24].setSibling(&(this->fields[21]));
            this->fields[24].setSibling(&(this->fields[23]));
            this->fields[26].setSibling(&(this->fields[27]));
            this->fields[26].setSibling(&(this->fields[29]));
            this->fields[27].setSibling(&(this->fields[26]));
            this->fields[27].setSibling(&(this->fields[29]));
            this->fields[29].setSibling(&(this->fields[26]));
            this->fields[29].setSibling(&(this->fields[27]));
            this->fields[31].setSibling(&(this->fields[32]));
            this->fields[31].setSibling(&(this->fields[34]));
            this->fields[32].setSibling(&(this->fields[31]));
            this->fields[32].setSibling(&(this->fields[34]));
            this->fields[34].setSibling(&(this->fields[31]));
            this->fields[34].setSibling(&(this->fields[32]));
            this->fields[37].setSibling(&(this->fields[39]));
            this->fields[39].setSibling(&(this->fields[37]));

            this->fields[5].setSibling(&(this->fields[15]));
            this->fields[5].setSibling(&(this->fields[25]));
            this->fields[5].setSibling(&(this->fields[35]));
            this->fields[15].setSibling(&(this->fields[5]));
            this->fields[15].setSibling(&(this->fields[25]));
            this->fields[15].setSibling(&(this->fields[35]));
            this->fields[25].setSibling(&(this->fields[5]));
            this->fields[25].setSibling(&(this->fields[15]));
            this->fields[25].setSibling(&(this->fields[35]));
            this->fields[35].setSibling(&(this->fields[5]));
            this->fields[35].setSibling(&(this->fields[15]));
            this->fields[35].setSibling(&(this->fields[25]));
        
            this->fields[12].setSibling(&(this->fields[28]));
            this->fields[28].setSibling(&(this->fields[12]));
    
        }
        
    private:
        int playerNum;
        Player *players;
        int *bankruptStat;
        Field *fields;
        int turnPlayer = -1;
        int end = 0;
};


void Player::declareBankrupt() {
    this->gameboard->setBankrupt(this->id);
}

void Card::execute(Player *player) {
    if (this->cardType) {
        switch (this->effect) {
            // 命運
            case 1:
                break;
            case 2:
                break;
            case 3:
                break;
            default:
                break;
        }
    } else {
        // 機會
        switch (this->effect) {
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
            
}




void game(WaitingRoom *room) {
    Gameboard board = Gameboard(room);

    int command;
    while (cin.good()) {

        cin >> command;
        //board.setBankrupt(command);
        board.nextTurn();
        Dice dice = roll();
        board.getTurnPlayer()->move(dice);
        board.printTrunPlayerMove();
        board.checkPassStart();
        board.getField(board.getTurnPlayer()->getPosition())->execute(board.getTurnPlayer());

        board.checkEnd();
        if ((board.isEnded()) || (command == -1)) {
            break;
        }
    }

    if ((board.isEnded())) {
        cout << board.winner() << " 贏了！\n";
    }
    

    
}


int main () {
    WaitingRoom room = {3, {"Explosion0w0", "kwkwkwkak", "LIAN26880912"}, {3000, 3001, 3002}};
    game(&room);


}