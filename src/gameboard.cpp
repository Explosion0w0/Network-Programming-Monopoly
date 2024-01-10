#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <time.h>
#include <chrono>
#include <thread>

extern "C" { 
    #include "unp.h"
} 

using namespace std;
using namespace std::this_thread; // sleep_for, sleep_until
using namespace std::chrono; // nanoseconds, system_clock, seconds




//this is for server
/*
    
    測試版:
        經過起點只有1$ (Field::execute())
        一般土地過路費*3 (Field::calcRent())
        Timeout 10s (void game() -> alarm())

    待改:
        每次送訊息給所有人就要初始化一個vector, not good
    還沒改完io:
        setBankrupt, checkbuy, sell
        log who buy what
*/



static void sig_alrm(int signo) {
    return;
}

void sig_chld(int signo)
{
	pid_t	pid;
	int		stat;

	pid = wait(&stat);
	printf("child %d terminated\n", pid);
	return;
}

class Gameboard;
class Player;
class Field;




struct WaitingRoom {
    int playerNum = 0;
    string playerNames[8];
    int sockfds[8];
    void addPlayer(char *name, int connfd) {
        this->playerNames[this->playerNum] = string(name);
        this->sockfds[this->playerNum++] = connfd;
    }
    void addPlayer(string name, int connfd) {
        this->playerNames[this->playerNum] = name;
        this->sockfds[this->playerNum++] = connfd;
    }
};


void sendToUser(int sockfd, char *str) {
    Writen(sockfd, str, strlen(str));
    sleep_for(seconds(1));
}
void sendToUser(int sockfd, string &str) {
    Writen(sockfd, const_cast<char *>(str.c_str()), str.size());
    sleep_for(seconds(1));
}

void sendToAllLivePlayer(Gameboard *board, char *str);
void sendToAllLivePlayer(Gameboard *board, string &str);

void commandMove(string &s, int playerId, int steps) {
    if (s.back() == '\n') s.pop_back();
    s.append("balance ").append(to_string(playerId)).append(" ").append(to_string(steps)).append("/\n");    
}
void commandBuild(string &s, int propertyId, int num) {
    if (s.back() == '\n') s.pop_back();
    s.append("build ").append(to_string(propertyId)).append(" ").append(to_string(num)).append("/\n");    
}
void commandLog(string &s, string &msg) {
    if (s.back() == '\n') s.pop_back();
    s.append("log ").append(msg).append("/\n");    
}
void commandBalance(string &s, int num) {
    if (s.back() == '\n') s.pop_back();
    s.append("balance ").append(to_string(num)).append("/\n");    
}
void commandMoveTo(string &s, int playerId, int index) {
    if (s.back() == '\n') s.pop_back();
    s.append("moveto ").append(to_string(playerId)).append(" ").append(to_string(index)).append("/\n");    
}
void commandRoll(string &s, int d1, int d2) {
    if (s.back() == '\n') s.pop_back();
    s.append("roll ").append(to_string(d1)).append(" ").append(to_string(d2)).append("/\n");    
}
void commandAskToBuy(string &s) {
    if (s.back() == '\n') s.pop_back();
    s.append("asktobuy/\n");    
}
void commandAskToSell(string &s) {
    if (s.back() == '\n') s.pop_back();
    s.append("asktosell/\n");    
}
void commandRemPlayer(string &s, int playerId) {
    if (s.back() == '\n') s.pop_back();
    s.append("remplayer ").append(to_string(playerId).append("/\n"));    
}
void commandOwnProp(string &s, int playerId, int propertyId) {
    if (s.back() == '\n') s.pop_back();
    s.append("ownprop ").append(to_string(playerId).append(" ").append(to_string(propertyId)).append("/\n"));    
}
void commandAddProp(string &s, string &str) {
    if (s.back() == '\n') s.pop_back();
    s.append("addprop ").append(str).append("/\n");    
}
void commandRemProp(string &s, string &str) {
    if (s.back() == '\n') s.pop_back();
    s.append("remprop ").append(str).append("/\n");    
}
void commandUnownProp(string &s, int propertyId) {
    if (s.back() == '\n') s.pop_back();
    s.append("unownprop ").append(to_string(propertyId)).append("/\n");    
}




class Card {    // 機會 or 命運卡
    public:
        Card(): cardType(0), effect(0), keep(0), desciption("a card") {}
        Card(int cardType, int effect, int keep, string descr): cardType(cardType), effect(effect), keep(keep), desciption(descr) {}
        void execute(Player *player);
        int canKeep() const {return this->keep;}
        string getDesciption() const {return this->desciption;}
    private:
        int cardType = 0; // 0:機會 1:命運
        int effect = 0; // 0:Empty
        int keep = 0; // 0:不可保留 1:no jail 2:no rent
        string desciption = "a card";
};

#define CHANCE_CARD_TYPE_NUM 20
#define DESTINY_CARD_TYPE_NUM 20

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
                descr = "You dropped the tofu pudding you just bought. Lose $30";
                break;
            case 2:
                descr = "You were struck by a stray bullet when strolling. Pay $150 in medical expenses.";
                break;
            case 3:
                descr = "Topple the towering walls of capitalism, let the wealth be equally shared among all comrades.";
                break;
            case 4:
                descr = "The typhoon blew your entire iron sheet house away. Lose a random building.";
                break;
            case 5:
                descr = "Your car fell into a sinkhole and needs fixing. Pay $100.";
                break;
            case 6:
                descr = "Your boss got caught selling drugs, so you became the scapegoat. Move to prison.";
                break;
            case 7:
                descr = "You caught the viral male star having a secret date with his girlfriend and demanded $50 hush money.";
                break;
            case 8:
                descr = "During election, your old house was exposed to be illegally built. Lose a random building";
                break;
            case 9:
                descr = "You cultivate some optoelectronics in your farmlands. Earn $15 for every property you own.";
                break;
            case 10:
                descr = "The international situation is quite tense, so your country decided to buy some arms. All players pay $50.";
                break;
            case 11:
                descr = "A wild mag 9 earthquake appeared! All buildings on a randomly chosen tile are destroyed.";
                break;
            case 12:
                descr = "Your father is in the construction business and has been cooperating with the ruling party for many years. If something goes wrong, your dad's got your back. (Avoid prison once)";
                keep = 1;
                break;
            case 13:
                descr = "You got scammed for $200";
                break;
            case 14:
                descr = "You tried to expose a game company's false advertisement but was accused of defamation. Hire a lawyer for $50.";
                break;
            case 15:
                descr = "It's raining and you forgot to bring an umbrella, so you paid $20 to get one from the nearest convenience store.";
                break;
            case 16:
                descr = "You caused a power outage during construction. Pay all players $20 as compensation.";
                break;
            case 17:
                descr = "Taipower experiences consecutive losses, electric prices increase. Move to the Taiwan Power Company tile and pay up";
                break;
            case 18:
                descr = "You represented your country for the Olympics and earned a medal. Receive $200.";
                break;
            case 19:
                descr = "You suck at mahjong and went on a losing streak during Chinese New Year. Pay a random player $50.";
                break;
            case 20:
                descr = "You are reported as a scalper and fined $100.";
                break;
            default:
                descr = "How tf did you get this? (Report a bug if you see this)";
                break;
        }
        
    } else {
        effect = rand();
        effect %= CHANCE_CARD_TYPE_NUM;
        effect += 1;
        switch (effect) {
            case 1:
                descr = "You dug up some grand treasure in Hsinchu baseball field. Receive $100.";
                break;
            case 2:
                descr = "You reported illegal parking and got $50.";
                break;
            case 3:
                descr = "You were tipped $50 during your street performance.";
                break;
            case 4:
                descr = "Your car was towed. Advance 3 steps.";
                break;
            case 5:
                descr = "The bank disburses interest. Receive $5 for every $100 you own.";
                break;
            case 6:
                descr = "You were forced sold some charity pens. Lose $50.";
                break;
            case 7:
                descr = "You flipped some sought-after pokemon cards and earned $200.";
                break;
            case 8:
                descr = "Mistaken as enemies by a gang, your car gets vandalized. Pay $100 in repair fees.";
                break;
            case 9:
                descr = "You travel on the Puyuma express. Move to the closest train station.";
                break;
            case 10:
                descr = "You went to Taipei 101 to see fireworks during New Year's Eve. Move to the Taipei City tile.";
                break;
            case 11:
                descr = "You are sent to jail for being the wheelman for a scam syndicate.";
                break;
            case 12:
                descr = "You made connections with a local faction leader. Big bro's got you covered. (Avoid prison once)";
                keep = 1;
                break;
            case 13:
                descr = "Today is my birthday! Receive up to $20 from all players. (Until they run out of balance)";
                break;
            case 14:
                descr = "Returning to your hometown for Chinese New Year, you move to your closest property. (Or back to start if there is None)";
                break;
            case 15:
                descr = "The multi-house tax 2.0 is now in effect. Pay $1 for every house and $5 for every hotel you own.";
                break;
            case 16:
                descr = "Your rent subsidy application got approved. You do not need to pay rent for the next property you encounter.";
                keep = 2;
                break;
            case 17:
                descr = "You won $200 from some scratch-offs.";
                break;
            case 18:
                descr = "You received $100 for winning the lottery.";
                break;
            case 19:
                descr = "The company you're in is doing quite well this season, so the boss gave your salary a boost. Receive $10.";
                break;
            case 20:
                descr = "You went to an overpriced Airbnb on vacation. Lose $150.";
                break;
            default:
                descr = "Man how do you keep getting these? (Report a bug if you see this)";
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
    srand(time(0));
    int d1, d2;
    d1 = rand();
    d2 = rand();
    d1 = (d1 % 6) + 1;
    d2 = (d2 % 6) + 1;
    cout << "骰子擲出 " << d1 << ", " << d2 << "\n";
    return {d1, d2};
}

struct Price {
    int fieldNum;
    int fieldType;
    string fieldName;
    int house;
    int hotel;
    int fieldPrice;
    int housePrice;
};

class Player {
    public:
        Player(): id(0), name(""), sockfd(0) {this->cards = vector<Card>();}
        Player(int id, string name, int sockfd, Gameboard *board): id(id), name(name), sockfd(sockfd), gameboard(board) {this->cards = vector<Card>();}
        void keepCard(Card card) {
            this->cards.push_back(card);
        }
        void useCard(int n) {
            this->cards[n].execute(this);
            this->cards[n] = this->cards[this->cards.size()-1];
            this->cards.pop_back();
        }
        int getNoJailCard() {   // return location in cards if exist, -1 otherwise
            for (int i = 0; i < (int)(this->cards.size()); i++) {
                if (this->cards[i].canKeep() == 1) {
                    return i;
                }
            }
            return -1;
        }
        int getNoRentCard() {   // return location in cards if exist, -1 otherwise
            for (int i = 0; i < (int)(this->cards.size()); i++) {
                if (this->cards[i].canKeep() == 2) {
                    return i;
                }
            }
            return -1;
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
            if (this->position > 39) {
                this->position %= 40;
                this->passedStart = 1;
            }
        }
        int getPassedStart() const {return this->passedStart;}
        void resetPassedStart() {this->passedStart = 0;}
        void earn(int n) {
            this->money += n;
            string s = "";
            commandBalance(s, n);
            sendToUser(this->sockfd, s);
        }
        void pay(int n);
        void payToPlayer(Player *other, int n) {
            other->earn(n);
            string s = "", msg = "";
            msg.append(this->name).append(" paid ").append(other->name).append(" $").append(to_string(n));
            commandLog(s, msg);
            sendToAllLivePlayer(this->gameboard, s);
            cout << this->name << " 付給 " << other->name << " " << n << "$\n";
            this->pay(n);
        }
        void sendToJail() {
            int num = this->getNoJailCard();
            if (num >= 0) {
                this->useCard(num);
            } else {
                this->inJail = 1;
                this->lastMove = {(this->position + 10)%40, 0};
                this->position = 10;
                string s = "", msg = "";
                commandMoveTo(s, this->id, 10);
                msg.append(this->name).append(" was sent to the jail, 3 rounds remaining.");
                commandLog(s, msg);
                sendToAllLivePlayer(this->gameboard, s);
                cout << this->name << " 被關進監獄了，剩餘 3 回合\n";
            }
        }
        void tryEscapeJail();
        
        int isInJail() const {return this->inJail;}
        void declareBankrupt();
        int sellForMoney();
        int getPriceList (vector<Price> &v); // return 可以賣的東西的總價
        void demolishRandHouse();
        void printMove();
        void triggerCurrentField();
        void checkPassStart();
        void getOtherLivePlayers(vector<Player*> &v);
        int getNearestOwnFieldDistance();
        int getHouseNum();
        int getFieldNum();
        void earthquake();
        Gameboard *getBoard() {return this->gameboard;}
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
        int inJail = 0;
        int getUserInput(char *buf);
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
        Field(): type(0), name(""), index(0) {fill_n(this->siblings, 3, nullptr);}
        Field(int type, string name, int index, Gameboard *board): type(type), name(name), index(index), gameboard(board) {fill_n(this->siblings, 3, nullptr);}
        Field(int type, string name, int index, int tax, Gameboard *board): type(type), name(name), index(index), rentInfo({0,tax,0,0,0,0,0,0}), gameboard(board) {fill_n(this->siblings, 3, nullptr);}
        Field(int type, string name, int index, int color, RentInfo rentInfo, Gameboard *board): type(type), name(name), index(index), color(color), rentInfo(rentInfo), gameboard(board) {fill_n(this->siblings, 3, nullptr);}
        void setSibling(Field *sib) {this->siblings[this->siblingNum++] = sib;}
        RentInfo getRentInfo() const {return this->rentInfo;}
        int getTax() const {return ((this->type == 9) ? this->rentInfo.rent : 0);}
        string getName() const {return this->name;}
        Player* getOwner() {return this->owner;}
        int getHouse() const {return this->house;}
        int getType() const {return this->type;}
        void execute(Player *player) {
            // 執行動作
            switch (this->type) {
                // 0:Empty 1:起點 2:土地 3:車站 4:公共事業 5:機會 6:命運 7:入獄 8:監獄 9:稅
                case 1:
                    if (player->getPassedStart()) {
                        player->earn(200);
                        cout << player->getName() << " 經過起點, 獲得$200\n";
                        string s = "", msg = "";
                        msg.append(player->getName()).append(" passed START and earned $200.");
                        commandLog(s, msg);
                        sendToAllLivePlayer(this->gameboard, s);
                        player->resetPassedStart();
                    }
                    break;
                case 2: {
                    if (this->owner == nullptr) {
                        checkBuy(player);
                    } else if (this->owner == player) {
                        checkBuildHouse(player);
                    } else {
                        int num = player->getNoRentCard();
                        if (num >= 0) {
                            player->useCard(num);
                        } else {
                            player->payToPlayer(this->owner, this->calcRent());
                        }
                    }
                    break;
                }
                case 3: {
                    if (this->owner == nullptr) {
                        checkBuy(player);
                    } else if (this->owner != player) {
                        int num = player->getNoRentCard();
                        if (num >= 0) {
                            player->useCard(num);
                        } else {
                            player->payToPlayer(this->owner, this->calcRailwayRent());
                        }
                    }
                    break;
                }
                case 4: {
                    if (this->owner == nullptr) {
                        checkBuy(player);
                    } else if (this->owner != player) {
                        int num = player->getNoRentCard();
                        if (num >= 0) {
                            player->useCard(num);
                        } else {
                            player->payToPlayer(this->owner, this->calcUtilityRent(player));
                        }
                    }
                    break;
                }
                case 5: {
                    Card card = randCard(0);
                    string s = "", msg = "";
                    msg.append(player->getName()).append(" drew \"").append(card.getDesciption()).append("\"");
                    commandLog(s, msg);
                    sendToAllLivePlayer(this->gameboard, s);
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
                    string s = "", msg = "";
                    msg.append(player->getName()).append(" drew \"").append(card.getDesciption()).append("\"");
                    commandLog(s, msg);
                    sendToAllLivePlayer(this->gameboard, s);
                    cout << player->getName() << "抽到 \"" << card.getDesciption() << "\"\n";
                    if (card.canKeep()) {
                        player->keepCard(card);
                    } else {
                        card.execute(player);
                    }
                    break;
                }
                case 7:
                    player->sendToJail();
                    break;
                case 8:
                    if (player->isInJail()) {
                        player->tryEscapeJail();
                    } else {
                        string s = "", msg = "";
                        msg.append(player->getName()).append(" is just passing by.");
                        commandLog(s, msg);
                        sendToAllLivePlayer(this->gameboard, s);
                        cout << player->getName() << " 只是經過\n";
                    }
                    break;
                case 9: {
                    string s = "", msg = "";
                    msg.append(player->getName()).append(" paid ").append(this->name).append(" $").append(to_string(this->getTax())).append(".");
                    commandLog(s, msg);
                    sendToAllLivePlayer(this->gameboard, s);
                    cout << player->getName() << " 支付 " << this->name << " " << this->getTax() << "$\n";
                    player->pay(this->getTax());
                    break;
                }    
                default:
                    break;
            }
            
        }
        void checkBuy(Player *player);
        void checkBuildHouse(Player *player);
        int calcRent() {
            int sameColor = 0;
            int mult = 3;
            for (int i = 0; i < this->siblingNum; i++) {
                if (this->siblings[i]->owner == this->owner) {
                    sameColor++;
                }
            }

            if (sameColor == this->siblingNum) {
                mult = 6;
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
        void demolish(int n) {
            this->house -= n;
            if (this->house < 0) {
                this->house = 0;
            }
            string s = "";
            commandBuild(s, this->index, -n);
            sendToAllLivePlayer(this->gameboard, s);
        }
        void sell() {
            this->owner = nullptr;
            this->house = 0;
            string s = "";
            commandUnownProp(s, this->index);
            sendToAllLivePlayer(this->gameboard, s);
        }
    private:
        int type; // 0:Empty 1:起點 2:土地 3:車站 4:公共事業 5:機會 6:命運 7:入獄 8:監獄 9:稅
        string name; // for all
        Player *owner = nullptr; // for type 2, 3, 4
        int index;
        int house = 0; // for type 2, if house == 5, it's hotel
        int color = 0; // 0:brown 1:skyblue 2:pink 3:orange 4:red 5:yellow 6:green 7:blue for type 2
        int siblingNum = 0; // 同顏色的地or車站的數量 for type 2, 3
        Field *siblings[3]; // 同顏色的地or車站 for type 2, 3
        RentInfo rentInfo = {0,0,0,0,0,0,0,0};
        Gameboard *gameboard = nullptr;
        int getUserInput(char *buf);
};


class Gameboard {   // 遊戲盤 aka 整個遊戲（包括銀行、玩家、場地）
    public:
        Gameboard(): playerNum(8) {
            this->players = new Player[8];
            this->bankruptStat = new int[8]{0};
            this->fields = new Field[40];
            this->survivors = new vector<Player*>();
            this->initGame();
        }
        Gameboard(WaitingRoom *room): playerNum(room->playerNum) {
            this->players = new Player[room->playerNum];
            this->bankruptStat = new int[room->playerNum];
            this->fields = new Field[40];
            this->survivors = new vector<Player*>();
            for (int i = 0; i < room->playerNum; i++) {
                this->bankruptStat[i] = 0;
                this->players[i] = Player(i, (room->playerNames)[i], (room->sockfds)[i], this);
                cout << (room->playerNames)[i] << " 加入遊戲\n";
                this->survivors->push_back(&(this->players[i]));
            }
            this->initGame();
        }
        ~Gameboard() {
            delete[] this->players;
            delete[] this->bankruptStat;
            delete[] this->fields;
            delete this->survivors;
        }
        void setField(int num, int type, string name) {
            this->fields[num] = Field(type, name, num, this);
        }
        void setField(int num, int type, string name, int tax) {
            this->fields[num] = Field(type, name, num, tax, this);
        }
        void setField(int num, int type, string name, int color, RentInfo rentInfo) {
            this->fields[num] = Field(type, name, num, color, rentInfo, this);
        }
        Field* getField(int num) {
            return &(this->fields[num]);
        }
        int getPlayerNumber() const {return this->playerNum;}
        Player* getPlayer(int id) {return &(this->players[id]);}
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
                if (this->sameTurnPlayer) {
                    this->turnPlayer--;
                    this->sameTurnPlayer = 0;
                }
                do {
                    if (this->turnPlayer >= this->playerNum-1) {
                        this->turnPlayer = 0;
                    } else {
                        this->turnPlayer++;
                    }
                } while (bankruptStat[this->turnPlayer]);
                cout << "現在是 " << this->players[this->turnPlayer].getName() << " 的回合\n";
                char buf[MAXLINE];
                sprintf(buf, "log ----------It\'s %s\'s turn.----------/\n", this->players[this->turnPlayer].getName().c_str());
                //cout << this->survivors->size() << "\n";
                sendToAllLivePlayer(this, buf);
                
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
        int winner() {
            if (this->checkEnd() <= 1) {
                int n;
                for (n = 0; n < this->playerNum; n++) {
                    if (this->bankruptStat[n] == 0) {
                        return n;
                    }
                }
            }
            return -1;
        }
        int waitForTPInput(char *buf) {
            fd_set rset;
            FD_ZERO(&rset);
            timeval tv = {300,0};
            for (;;) {
                int maxfd = 0;
                for(int i = 0; i < (int)(this->survivors->size()); i++) {
                    int fd = this->survivors->at(i)->getSockfd();
                    FD_SET(fd, &rset);
                    if (fd > maxfd) {
                        maxfd = fd;
                    } 
                }
                maxfd++;
                Select(maxfd, &rset, NULL, NULL, &tv);
                for(int i = 0; i < (int)(this->survivors->size()); i++) {
                    int fd = this->survivors->at(i)->getSockfd();
                    if (FD_ISSET(fd, &rset)) {
                        if (this->survivors->at(i) == this->getTurnPlayer()) {
                            int n = Read(fd, buf, MAXLINE);
                            buf[n] = '\0';
                            if (n <= 0) {
                                this->playerLeave(this->survivors->at(i)->getId());
                                return n;
                            } 
                            return n;
                        } else {
                            int n = Read(fd, buf, MAXLINE);
                            buf[n] = '\0';
                            if (n <= 0) {
                                if (errno == EINTR) {
                                    this->playerTimeout(this->getTurnPlayerNum());
                                    return 0;
                                } else {
                                    this->playerLeave(this->survivors->at(i)->getId());
                                }
                            }
                        }
                    }
                }
            }
        }
        void setBankrupt(int id) {
            this->bankruptStat[id] = 1;
            for (int i = 0; i < 40; i++) {
                if (this->fields[i].getOwner() == &(this->players[id])) {
                    this->fields[i].sell();
                }
            }
            string s = "";
            commandRemPlayer(s, id);
            sendToAllLivePlayer(this, s);
            for (int i = 0; i < (int)(this->survivors->size()); i++) {
                if (this->survivors->at(i) == &(this->players[id])) {
                    for (int j = i; j < (int)(this->survivors->size()-1); j++) {
                        this->survivors[j] = this->survivors[j+1];
                    }
                    this->survivors->pop_back();
                }
            }
        }
        void printTrunPlayerMove() {
            this->getTurnPlayer()->printMove();
        }
        void checkPassStart(Player *player) {
            this->fields[0].execute(player);
        }
        void checkTurnPlayerPassStart() {
            this->checkPassStart(this->getTurnPlayer());
        }
        int isTurnPlayerBankrupt() {
            return this->bankruptStat[this->turnPlayer];
        }
        int isPlayerBankrupt(int id) {
            return this->bankruptStat[id];
        }
        void samePlayerNextTurn() {
            this->sameTurnPlayer = 1;
        }
        void getLivePlayers(vector<Player*> &v) {
            v.clear();
            for (int i = 0; i < this->playerNum; i++) {
                if (this->bankruptStat[i] == 0) {
                    v.push_back(&(this->players[i]));
                }
            }
        }
        void getOtherLivePlayers(Player *player, vector<Player*> &v) {
            v.clear();
            for (int i = 0; i < this->playerNum; i++) {
                if ((this->bankruptStat[i] == 0) && (&(this->players[i]) != player)) {
                    v.push_back(&(this->players[i]));
                }
            }
        }
        void getFieldOwnedByPlayer(Player *player, vector<Field*> &v) {
            v.clear();
            for (int i = 0; i < 40; i++) {
                int fType = this->fields[i].getType();
                if ((fType >= 2) && (fType <= 4)) {
                    if (this->fields[i].getOwner() == player) {
                        v.push_back(&(this->fields[i]));
                    }
                }
            }
        }
        void turnPlayerTriggerField() {
            this->getTurnPlayer()->triggerCurrentField();
        }
        int getNearestOwnFieldDistance(Player *player) { // return the distance
            int move = 0;
            int p = player->getPosition();
            for (int i = p; i < p+40; i++) {
                if (this->fields[i].getOwner() == player) {
                    move = i-p;
                    break;
                }
            }
            return move;
        }
        void earthquake() {
            int r = rand() % 22;
            for (int i = 0; i < 40; i++) {
                if (this->fields[i].getType() == 2) {
                    if (r-- == 1) {
                        int n = this->fields[i].getHouse();
                        if (n > 0) {
                            this->fields[i].demolish(n);
                            char buf[MAXLINE];
                            sprintf(buf, "log Earthquake struck %s, all houses were destroyed./\n", this->fields[i].getName().c_str());
                            sendToAllLivePlayer(this, buf);
                            cout << this->fields[i].getName() << " 發生地震，所有房屋都被震毀了\n";
                        } else {
                            char buf[MAXLINE];
                            sprintf(buf, "log Earthquake struck %s, but no houses were destroyed./\n", this->fields[i].getName().c_str());
                            sendToAllLivePlayer(this, buf);
                            cout << this->fields[i].getName() << " 發生地震，所幸沒有房屋被震毀\n";
                        }
                    }
                }
            }
        }
        void turnPlayerTimeout() {
            this->setBankrupt(this->turnPlayer);
            string s = "", msg = "";
            msg.append(this->getTurnPlayer()->getName()).append(" timeout, regarded as bankrupt.");
            commandLog(s, msg);
            sendToAllLivePlayer(this, s);
            cout << this->getTurnPlayer()->getName() << " 超時，已宣告破產\n";
        }
        void playerTimeout(int id) {
            this->setBankrupt(id);
            string s = "", msg = "";
            msg.append(this->getTurnPlayer()->getName()).append(" timeout, regarded as bankrupt.");
            commandLog(s, msg);
            sendToAllLivePlayer(this, s);
            cout << this->players[id].getName() << " 超時，已宣告破產\n";
        }
        void turnPlayerLeave() {
            this->setBankrupt(this->turnPlayer);
            string s = "", msg = "";
            msg.append(this->getTurnPlayer()->getName()).append(" left the game.");
            commandLog(s, msg);
            sendToAllLivePlayer(this, s);
            cout << this->getTurnPlayer()->getName() << " 已離開遊戲\n";
        }
        void playerLeave(int id) {
            this->setBankrupt(id);
            string s = "", msg = "";
            msg.append(this->getTurnPlayer()->getName()).append(" left the game.");
            commandLog(s, msg);
            sendToAllLivePlayer(this, s);
            cout << this->players[id].getName() << " 已離開遊戲\n";
        }
        int getUserInput(char *buf);
        vector<Player*> *getSurvivors() {return this->survivors;}
        void initGame () {
            // 0:Empty 1:起點 2:土地 3:車站 4:公共事業 5:機會 6:命運 7:入獄 8:監獄 9:稅
            // 0:brown 1:skyblue 2:pink 3:orange 4:red 5:yellow 6:green 7:blue
            /*
                台北    87.77
                新北    48.84

                桃園    32.40
                竹市    42.02
                新竹    41.10

                台中    37.51
                彰化    26.09
                南投    22.84

                雲林    20.03
                嘉義    24.58
                嘉市    20.36

                台南    29.74
                高雄    28.54
                屏東    18.30

                宜蘭    24.86
                花蓮    22.91
                台東    17.30

                連江    23.27
                金門    23.20
                澎湖    18.26

                苗栗    27.65
                基隆    24.43
            */
            this->setField(0, 1, "START");
            this->setField(1, 2, "Keelung City", 0, {60,2,10,30,90,160,250,50});
            this->setField(2, 6, "DESTINY");
            this->setField(3, 2, "Miaoli Count(r)y", 0, {60,4,20,60,180,320,450,50});
            this->setField(4, 9, "INCOME TAX", 200);
            this->setField(5, 3, "Taitung Train Station", 0, {200,25,50,100,200,0,0,0});
            this->setField(6, 2, "Penghu County", 1, {100,6,30,90,270,400,550,50});
            this->setField(7, 5, "CHANCE");
            this->setField(8, 2, "Kinmen County", 1, {100,6,30,90,270,400,550,50});
            this->setField(9, 2, "Lianjiang County", 1, {120,8,40,100,300,450,600,50});
            this->setField(10, 8, "JAIL");
            this->setField(11, 2, "Taitung County", 2, {140,10,50,150,450,625,750,100});
            this->setField(12, 4, "Taiwan Power Company", 0, {150,4,10,0,0,0,0,0});
            this->setField(13, 2, "Hualien County", 2, {140,10,50,150,450,625,750,100});
            this->setField(14, 2, "Yilan County", 2, {160,12,60,180,500,700,900,100});
            this->setField(15, 3, "Tainan Train Station", 0, {200,25,50,100,200,0,0,0});
            this->setField(16, 2, "Pingtung County", 3, {180,14,70,200,550,750,950,100});
            this->setField(17, 6, "DESTINY");
            this->setField(18, 2, "Kaohsiung City", 3, {180,14,70,200,550,750,950,100});
            this->setField(19, 2, "Tainan City", 3, {200,16,80,220,600,800,1000,100});
            this->setField(20, 0, "FREE PARKING");
            this->setField(21, 2, "Chiayi City", 4, {220,18,90,250,700,875,1050,150});
            this->setField(22, 5, "CHANCE");
            this->setField(23, 2, "Chiayi County", 4, {220,18,90,250,700,875,1050,150});
            this->setField(24, 2, "Yunlin County", 4, {240,20,100,300,750,925,1100,150});
            this->setField(25, 3, "Taichung Train Station", 0, {200,25,50,100,200,0,0,0});
            this->setField(26, 2, "Nantou County", 5, {260,22,110,330,800,975,1150,150});
            this->setField(27, 2, "Changhua County", 5, {260,22,110,330,800,975,1150,150});
            this->setField(28, 4, "Taiwan Water Corporation", 0, {150,4,10,0,0,0,0,0});
            this->setField(29, 2, "Taichung City", 5, {280,24,120,360,850,1025,1200,150});
            this->setField(30, 7, "GO TO JAIL");
            this->setField(31, 2, "Hsinchu County", 6, {300,26,130,390,900,1100,1275,200});
            this->setField(32, 2, "Hsinchu City", 6, {300,26,130,390,900,1100,1275,200});
            this->setField(33, 6, "DESTINY");
            this->setField(34, 2, "Taoyuan City", 6, {320,28,150,450,1000,1200,1400,200});
            this->setField(35, 3, "Taipei Train Station", 0, {200,25,50,100,200,0,0,0});
            this->setField(36, 5, "CHANCE");
            this->setField(37, 2, "New Taipei City", 7, {350,35,175,500,1100,1300,1500,200});
            this->setField(38, 9, "LUXURY TAX", 100);
            this->setField(39, 2, "Taipei City", 7, {400,50,200,600,1400,1700,2000,200});

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
        int *bankruptStat; // 0:alive 1:bankrupt
        Field *fields;
        int turnPlayer = -1;
        int end = 0;
        int sameTurnPlayer = 0;
        vector<Player*> *survivors;
};




void Player::pay(int n) {
    if (!(this->gameboard->isPlayerBankrupt(this->id))) {
        this->money -= n;
        string s = "";
        commandBalance(s, -n);
        sendToUser(this->sockfd, s);
        if ((this->money < 0)) {
            if (!(this->sellForMoney())) {
                this->declareBankrupt();
            }
        }
    }
}
void Player::declareBankrupt() {
    this->gameboard->setBankrupt(this->id);
    string s = "", msg = "";
    msg.append(this->name).append(" has gone bankrupt.");
    commandLog(s, msg);
    sendToAllLivePlayer(this->gameboard, s);
    cout << this->name << " 破產了\n";
}
void printPriceList(vector<Price> &v, string &s) {
    for (int i = 0; i < (int)(v.size()); i++) {
        if (v[i].fieldType == 2) {
            string msg = "";
            msg.append("\t").append(v[i].fieldName).append(": $").append(to_string(v[i].fieldPrice+v[i].housePrice*(v[i].house+v[i].hotel*5)));
            commandLog(s, msg);
            cout << "\t" << i+1 << ". " << v[i].fieldName << ":\n\t\t";
            if (v[i].house > 0) {
                cout << "房屋*" << v[i].house << "(單價" << v[i].housePrice << "$)   ";
            }
            if (v[i].hotel > 0) {
                cout << "旅館*" << v[i].house << "(單價" << v[i].housePrice*5 << "$)   ";
            }
            cout << "土地(" << v[i].fieldPrice << "$)   總共(" << v[i].fieldPrice+v[i].housePrice*(v[i].house+v[i].hotel*5) << "$)\n";
        } else if ((v[i].fieldType == 3) || (v[i].fieldType == 4)) {
            string msg = "";
            msg.append("\t").append(v[i].fieldName).append(": $").append(to_string(v[i].fieldPrice));
            commandLog(s, msg);
            cout << "\t" << i+1 << ". " << v[i].fieldName << ":\n\t\t土地(" << v[i].fieldPrice << "$)\n";
        }
    }
}
int Player::getPriceList(vector<Price> &v) {
    v.clear();
    int sum = 0;
    for (int i = 0; i < 40; i++) {
        if (this->gameboard->getField(i)->getOwner() == this) {
            Field *f = this->gameboard->getField(i);
            string name = f->getName();
            int fType = f->getType();
            int fPrice = f->getRentInfo().cost / 2;
            if (fType == 2) {
                int house = f->getHouse();
                int hotel = 0;
                if (house > 4) {
                    house = 0;
                    hotel = 1;
                }
                int hPrice = f->getRentInfo().houseCost / 2;
                v.push_back({i, fType, name, house, hotel, fPrice, hPrice});
                sum += fPrice + house*hPrice + hotel*hPrice*5;
            } else if ((fType == 3) || (fType == 4)) {
                v.push_back({i, fType, name, 0, 0, fPrice, 0});
                sum += fPrice;
            }
        }
    }
    return sum;
}
void Player::printMove() {
    char buf[MAXLINE];
    int m = this->lastMove.d1 + this->lastMove.d2;
    sprintf(buf, "move %d %d/log %s moved %d steps to %s./\n", this->id, m, this->name.c_str(), m, this->gameboard->getField(this->position)->getName().c_str());
    sendToAllLivePlayer(this->gameboard, buf);
    cout << this->name << " 走了 " << this->lastMove.d1 + this->lastMove.d2 << " 步，來到 " << this->gameboard->getField(this->position)->getName() << "\n";
}
void Player::triggerCurrentField() {
    this->gameboard->getField(this->position)->execute(this);
}
int Player::sellForMoney() { // return 0:money not enough -> bankrupt  1:money no more negative
    vector<Price> priceList = vector<Price>();
    if ((this->money <= 0) && (this->getPriceList(priceList) <= -this->money)) {
        return 0;
    }
    while ((this->money < 0) && !(priceList.empty())) {
        //cout << "現在可販售(缺" << -this->money << "$):\n";
        string s = "";
        commandAskToSell(s);
        printPriceList(priceList, s);
        sendToUser(this->sockfd, s);
        char buf[MAXLINE];
        int propIndex, fNum = 0;
        //cout << "選擇要販售的土地(編號): ";
        if (this->gameboard->waitForTPInput(buf) <= 0) return 1;
        cout << buf << "|" << strlen(buf) << "\n";
        sscanf(buf, "%d", &propIndex);
        for (int i = 0; i < (int)(priceList.size()); i++) {
            if (priceList[i].fieldNum == propIndex) {
                fNum = i;
                break;
            }
        }
        if (priceList[fNum].fieldType == 2) {
            
            if (priceList[fNum].house > 0) {
                //int onlyHouse;
                //cout << "只販售房屋? 0:no 1:yes :";
                //if (this->getUserInput(onlyHouse) <= 0) return 1;
                //while ((onlyHouse != 0) && (onlyHouse != 1)) {
                //    cout << "invalid input\n";
                //    if (this->getUserInput(onlyHouse) <= 0) return 1;
                //}
                /*if (onlyHouse) {
                    int hNum;
                    if (priceList[fNum-1].house == 1) {
                        hNum = 1;
                    } else {
                        cout << "要販售的房屋數(1~" << priceList[fNum-1].house <<"): ";
                        while ((hNum <= 0) || (hNum > priceList[fNum-1].house)) {
                            cout << "invalid input\n";
                            if (this->getUserInput(hNum) <= 0) return 1;
                        }
                    }
                    this->earn(priceList[fNum-1].housePrice * hNum);
                    cout << this->name << " 販售 " << hNum << " 間房子，得到 " << priceList[fNum-1].housePrice * hNum << "$\n";
                    priceList[fNum-1].house -= hNum;
                    this->gameboard->getField(priceList[fNum-1].fieldNum)->demolish(hNum);
                } else {*/
                    this->earn(priceList[fNum].housePrice * priceList[fNum].house + priceList[fNum].fieldPrice);
                    string s = "", msg = "";
                    msg.append(this->name).append(" sold ").append(priceList[fNum].fieldName).append(" for $").append(to_string(priceList[fNum].housePrice * priceList[fNum].house + priceList[fNum].fieldPrice));
                    commandLog(s, msg);
                    cout << this->name << " 販售 " << priceList[fNum].fieldName << " ，得到 " << priceList[fNum].housePrice * priceList[fNum].house + priceList[fNum].fieldPrice << "$\n";
                    this->gameboard->getField(priceList[fNum].fieldNum)->sell();
                    for (int i = fNum; i < (int)(priceList.size()-1); i++) {
                        priceList[i] = priceList[i+1]; 
                    }
                    priceList.pop_back();
                    sendToAllLivePlayer(this->gameboard, s);
                //}
            } else if (priceList[fNum-1].hotel > 0) {
                /*
                int onlyHotel;
                cout << "只販售旅館? 0:no 1:yes :";
                if (this->getUserInput(onlyHotel) <= 0) return 1;
                while ((onlyHotel != 0) && (onlyHotel != 1)) {
                    cout << "invalid input\n";
                    if (this->getUserInput(onlyHotel) <= 0) return 1;
                }
                if (onlyHotel) {
                    this->earn(priceList[fNum-1].housePrice * 5);
                    cout << this->name << " 販售 1 間旅館，得到 " << priceList[fNum-1].housePrice * 5 << "$\n";
                    priceList[fNum-1].hotel = 0;
                    this->gameboard->getField(priceList[fNum-1].fieldNum)->demolish(5);
                } else {*/
                    this->earn(priceList[fNum].housePrice * 5 + priceList[fNum].fieldPrice);
                    string s = "", msg = "";
                    msg.append(this->name).append(" sold ").append(priceList[fNum].fieldName).append(" for $").append(to_string(priceList[fNum].housePrice * 5 + priceList[fNum].fieldPrice));
                    commandLog(s, msg);
                    cout << this->name << " 販售 " << priceList[fNum].fieldName << " ，得到 " << priceList[fNum].housePrice * 5 + priceList[fNum].fieldPrice << "$\n";
                    this->gameboard->getField(priceList[fNum].fieldNum)->sell();
                    for (int i = fNum; i < (int)(priceList.size()-1); i++) {
                        priceList[i] = priceList[i+1]; 
                    }
                    priceList.pop_back();
                    sendToAllLivePlayer(this->gameboard, s);
                //}
            } else {
                this->earn(priceList[fNum].fieldPrice);
                string s = "", msg = "";
                msg.append(this->name).append(" sold ").append(priceList[fNum].fieldName).append(" for $").append(to_string(priceList[fNum].fieldPrice));
                commandLog(s, msg);
                cout << this->name << " 販售 " << priceList[fNum].fieldName << " ，得到 " << priceList[fNum].fieldPrice << "$\n";
                this->gameboard->getField(priceList[fNum].fieldNum)->sell();
                for (int i = fNum; i < (int)(priceList.size()-1); i++) {
                    priceList[i] = priceList[i+1]; 
                }
                priceList.pop_back();
                sendToAllLivePlayer(this->gameboard, s);
            }
        } else {
            this->earn(priceList[fNum-1].fieldPrice);
            string s = "", msg = "";
            msg.append(this->name).append(" sold ").append(priceList[fNum].fieldName).append(" for $").append(to_string(priceList[fNum].fieldPrice));
            commandLog(s, msg);
            cout << this->name << " 販售 " << priceList[fNum-1].fieldName << " ，得到 " << priceList[fNum].fieldPrice << "$\n";
            this->gameboard->getField(priceList[fNum-1].fieldNum)->sell();
            for (int i = fNum; i < (int)(priceList.size()); i++) {
                priceList[i-1] = priceList[i]; 
            }
            priceList.pop_back();
            sendToAllLivePlayer(this->gameboard, s);
        }
    }

    return 1;
}
void Player::demolishRandHouse() {
    vector<Field*> v = vector<Field*>();
    this->gameboard->getFieldOwnedByPlayer(this, v);
    vector<Field*> f = vector<Field*>();
    for (int i = 0; i < (int)(v.size()); i++) {
        if ((v[i]->getType() == 2) && (v[i]->getHouse() > 0)) {
            f.push_back(v[i]);
        }
    }
    if (f.size() > 0) {
        int r = rand() % (int)(f.size());
        if (f[r]->getHouse() == 5) {
            f[r]->demolish(5);
            string s = "", msg = "";
            msg.append("hotel at ").append(f[r]->getName()).append("was demolished.");
            commandLog(s, msg);
            sendToAllLivePlayer(this->gameboard, s);
            cout << f[r]->getName() << " 的旅館被拆除了\n";
        } else {
            f[r]->demolish(1);
            string s = "", msg = "";
            msg.append("house at ").append(f[r]->getName()).append("was demolished.");
            commandLog(s, msg);
            sendToAllLivePlayer(this->gameboard, s);
            cout << f[r]->getName() << " 的房屋被拆除了\n";
        }
    } else {
        string s = "", msg = "There were no houses that were demolished.";
        commandLog(s, msg);
        sendToAllLivePlayer(this->gameboard, s);
        cout << "沒有任何房屋被拆除\n";
    }
}
void Player::checkPassStart() {
    this->gameboard->checkPassStart(this);
}
void Player::getOtherLivePlayers(vector<Player*> &v) {
    this->gameboard->getOtherLivePlayers(this, v);
}
int Player::getNearestOwnFieldDistance() {
    return this->gameboard->getNearestOwnFieldDistance(this);
}
int Player::getHouseNum() { // hotel as 5
    vector<Field*> v = vector<Field*>();
    this->gameboard->getFieldOwnedByPlayer(this, v);
    int num = 0;
    for (int i = 0; i < (int)(v.size()); i++) {
        if ((v[i]->getType() == 2)) {
            num += v[i]->getHouse();
        }
    }
    return num;
}
int Player::getFieldNum() { // hotel as 5
    vector<Field*> v = vector<Field*>();
    this->gameboard->getFieldOwnedByPlayer(this, v);
    return (int)(v.size());
}
void Player::earthquake() {
    this->gameboard->earthquake();
}


void Card::execute(Player *player) {
    if (this->cardType) {
        switch (this->effect) {
            // 命運
            case 1:
                // "剛買的豆花掉到地上，損失30$";
                player->pay(30);
                break;
            case 2:
                // "走在路上被流彈擊中，醫藥費150$";
                player->pay(150);
                break;
            case 3: {
                // "推倒資本主義的高牆，所有人現金平分";
                int sum = 0;
                sum += player->getMoney();
                player->pay(player->getMoney());
                vector<Player*> v = vector<Player*>();
                player->getOtherLivePlayers(v);
                for (int i = 0; i < (int)(v.size()); i++) {
                    int m = v[i]->getMoney();
                    sum += m;
                    v[i]->pay(m);
                }
                int avr = sum / ((int)(v.size()) + 1);
                player->earn(avr);
                break;
            }
            case 4:
                // "颱風肆虐，將鐵皮屋整棟吹飛，隨機損失1棟房屋";
                player->demolishRandHouse();
                break;
            case 5:
                // "車子掉進天坑，維修費100$";
                player->pay(100);
                break;
            case 6:
                // "大哥販毒被抓，成為替罪羔羊，入獄";
                player->sendToJail();
                break;
            case 7:
                // "抓到當紅男星與女友密會，索取封口費50$";
                player->earn(50);
                break;
            case 8:
                // "參選期間被抓到老家違建，隨機損失1棟房屋";
                player->demolishRandHouse();
                break;
            case 9: {
                // "在農地上種光電，每持有一塊地獲得15$";
                int num = player->getFieldNum();
                num *= 15;
                string s = "", msg = "";
                msg.append("total: $").append(to_string(num));
                commandLog(s, msg);
                sendToUser(player->getSockfd(), s);
                cout << "總共獲得 " << num << "$\n";
                player->earn(num);
                break;
            }
            case 10: {
                // "國際情勢緊張，被迫購買軍火，由全民買單，所有人繳最多50$ (除了回合玩家以外，最多繳到沒有現金)";
                vector<Player*> v = vector<Player*>();
                player->getOtherLivePlayers(v);
                for (int i = 0; i < (int)(v.size()); i++) {
                    int m = v[i]->getMoney();
                    if (m > 50) {
                        m = 50;
                    }
                    v[i]->pay(m);
                }
                player->pay(50);
                break;
            }
            case 11:
                // "發生規模9.0強震，地圖上隨機一塊地上房屋全毀 (所有土地都有可能)";
                player->earthquake();
                break;
            case 12: {
                // "投胎投得好，父親身為建商，與執政黨合作多年，出事了有老爸罩 (免刑1次，可保留)";
                string s = "", msg = "";
                msg.append(player->getName()).append("was released from jail after one day due to the \"special\" relationship.");
                commandLog(s, msg);
                sendToAllLivePlayer(player->getBoard(), s);
                cout << player->getName() << " 靠關係在外役監待了1天就出來了\n";
                break;
            }
            case 13:
                // "慘遭投資詐騙，血本無歸，損失200$";
                player->pay(200);
                break;
            case 14:
                // "揭露遊戲公司廣告不實，被告侵害名譽，律師費50$";
                player->pay(50);
                break;
            case 15:
                // "下雨忘記帶傘，去便利商店買一把20$";
                player->pay(20);
                break;
            case 16: {
                // "施工不慎造成停電，對所有人支付賠償金20$";
                vector<Player*> v = vector<Player*>();
                player->getOtherLivePlayers(v);
                for (int i = 0; i < (int)(v.size()); i++) {
                    player->payToPlayer(v[i], 20);
                }
                break;
            }
            case 17:
                // "台電連年虧損，電價上漲，移動到台灣電力公司並支付電費";
                player->move({(52-player->getPosition())%40,0});
                player->printMove();
                player->checkPassStart();
                player->triggerCurrentField();
                break;
            case 18:
                // "代表國家參加奧運，為國爭光，獲得獎金200$";
                player->earn(200);
                break;
            case 19: {
                // "過年打麻將連敗，付給隨機一位玩家50$";
                vector<Player*> v = vector<Player*>();
                player->getOtherLivePlayers(v);
                if (v.size() > 0) {
                    int n = rand() % (int)(v.size());
                    player->payToPlayer(v[n], 50);
                    break;
                }
            }
            case 20:
                // "當黃牛被檢舉，罰款100$";
                player->pay(100);
                break;
            default:
                // "在演藝圈闖出名號，開始被狗仔跟蹤 (你不該看到這張卡，如果看到了，請回報bug)";
                break;
        }
    } else {
        // 機會
        switch (this->effect) {
            case 1:
                // "在新竹棒球場發現大秘寶，獲得100$";
                player->earn(100);
                break;
            case 2:
                // "發現違規停車，獲得檢舉獎金50$";
                player->earn(50);
                break;
            case 3:
                // "街頭演出獲得打賞50$";
                player->earn(50);
                break;
            case 4:
                // "車子被拖吊，往前走3步";
                player->move({3,0});
                player->printMove();
                player->triggerCurrentField();
                break;
            case 5: 
                // "銀行發放利息，每持有100$獲得5$";
                player->earn((player->getMoney() / 100) * 5);
                break;
            case 6:
                // "路上被強迫推銷愛心筆，損失50$";
                player->pay(50);
                break;
            case 7:
                // "轉賣高人氣的寶口夢卡牌，收入200$";
                player->earn(200);
                break;
            case 8:
                // "被黑道誤認為是仇家，車子被砸，維修費100$";
                player->pay(100);
                break;
            case 9:
                // "乘坐普悠瑪號旅行，移動到最近的車站";
                player->move({(45-player->getPosition())%10,0});
                player->printMove();
                player->checkPassStart();
                player->triggerCurrentField();
                break;
            case 10:
                // "跨年到101看煙火，移動到台北";
                player->move({(39-player->getPosition()),0});
                player->printMove();
                player->triggerCurrentField();
                break;
            case 11:
                // "當詐騙集團車手被抓，入獄";
                player->sendToJail();
                break;
            case 12:
                // "結識地方派系大佬，出事了有大哥罩 (免刑1次，可保留)";
                cout << player->getName() << " 受到地方派系大佬的保護，嫁禍給代罪羔羊\n";
                break;
            case 13: {
                // "今天我生日，向所有玩家索取最多20$ (其他玩家最多支付到沒有現金)";
                vector<Player*> v = vector<Player*>();
                player->getOtherLivePlayers(v);
                for (int i = 0; i < (int)(v.size()); i++) {
                    int m = v[i]->getMoney();
                    if (m > 20) {
                        m = 20;
                    }
                    v[i]->payToPlayer(player, m);
                }
                break;
            }
            case 14: {
                // "過年返鄉，移動到最近的所有地 (如果沒有的話就移動到起點)";
                int d = player->getNearestOwnFieldDistance();
                if (d == 0) {
                    player->move({(40-player->getPosition())%40,0});
                } else {
                    player->move({d,0});
                }
                player->printMove();
                player->checkPassStart();
                player->triggerCurrentField();
                break;
            }
            case 15: {
                // "囤房稅2.0上線，每持有一棟房屋繳1$，每持有一棟旅館繳5$";
                int num = player->getHouseNum();
                cout << "總共須繳 " << num << "$\n";
                player->pay(num);
                break;
            }
            case 16:
                // "獲得租金補貼 (免繳過路費1次，可保留)";
                cout << "租金補貼已入帳， " << player->getName() << " 免付過路費一次\n";
                break;
            case 17:
                // "刮刮樂中獎，獲得200$";
                player->earn(200);
                break;
            case 18:
                // "發票中獎，獲得100$";
                player->earn(100);
                break;
            case 19:
                // "公司賺錢，老闆加薪，獲得10$";
                player->earn(10);
                break;
            case 20:
                // "出門旅遊住民宿被當盤子，損失150$";
                player->pay(150);
                break;
            default:
                // "揭露官商勾結，被查水表 (你不該看到這張卡，如果看到了，請回報bug)";
                break;
        }
    }
            
}


void Field::checkBuy(Player *player) {
            if (player->getMoney() >= this->rentInfo.cost) {
                string s = "", msg = "";
                msg.append("Do you want to buy ").append(this->name).append(" ($").append(to_string(this->rentInfo.cost)).append(")?");
                commandAskToBuy(s);
                commandLog(s, msg);
                sendToUser(player->getSockfd(), s);
                //cout << "是否購買 " << this->name << " (" << this->rentInfo.cost << "$) ? 0:no 1:yes\n";
                char buf[MAXLINE];
                //this->getUserInput(buf);
                this->gameboard->waitForTPInput(buf);
                cout << "BUF " << buf << strlen(buf) << "\n";
                if ((strcmp(buf, "NO\n") == 0) || (strcmp(buf, "NO") == 0)) {
                    return;
                } else if ((strcmp(buf, "YES\n") == 0) || (strcmp(buf, "YES") == 0)) {
                    player->pay(this->rentInfo.cost);
                    this->owner = player;
                    
                    string s = "", msg = "";
                    msg.append(this->name).append(" $").append(to_string(this->rentInfo.cost/2)).append(" | $").append(to_string(this->rentInfo.houseCost/2));
                    commandAddProp(s, msg);
                    sendToUser(player->getSockfd(), s);
                    s = "";
                    msg = "";
                    msg.append(player->getName()).append(" bought ").append(this->name).append(".");
                    commandOwnProp(s, player->getId(), this->index);
                    commandLog(s, msg);
                    sendToAllLivePlayer(this->gameboard, s);
                    cout << player->getName() << " 購買了 " << this->name << "\n";
                }
                
            } else {
                //cout << "金錢不足，無法購買\n";
                string s = "", msg = "You don\'t have enough money to buy this property.";
                commandLog(s, msg);
                sendToUser(player->getSockfd(), s);
            }
        }
        void Field::checkBuildHouse(Player *player) {
            if (this->house < 5) {
                if (player->getMoney() >= this->rentInfo.houseCost) {
                    char buf[MAXLINE];
                    sprintf(buf, "asktobuy/log Do you want to build a %s on %s ($%d)?/\n", ((this->house == 4) ? "hotel" : "house"), this->name.c_str(), this->rentInfo.houseCost);
                    sendToUser(player->getSockfd(), buf);
                    //cout << "是否在 " << this->name << ((this->house == 4) ? " 蓋旅館 (" : " 蓋房子 (") << this->rentInfo.houseCost << "$) ? 0:no 1:yes\n";
                    //this->getUserInput(buf);
                    this->gameboard->waitForTPInput(buf);
                    cout << buf << "|" << strlen(buf) << "\n";
                    if ((strcmp(buf, "NO\n") == 0) || (strcmp(buf, "NO") == 0)) {
                        return;
                    } else if ((strcmp(buf, "YES\n") == 0) || (strcmp(buf, "YES") == 0)) {
                        player->pay(this->rentInfo.houseCost);
                        this->house++;
                        sprintf(buf, "build %d 1/log %s built a %s on %s./\n", this->index, player->getName().c_str(), ((this->house == 4) ? "hotel" : "house"), this->name.c_str());
                        sendToAllLivePlayer(this->gameboard, buf);
                        cout << player->getName() << " 在 " << this->name << ((this->house == 5) ? " 蓋了旅館\n" : " 蓋了房子\n");
                    }
                } else {
                    char buf[MAXLINE];
                    sprintf(buf, "log You don\'t have enough money to build a %s./\n", ((this->house == 4) ? "hotel" : "house"));
                    sendToUser(player->getSockfd(), buf);
                    //cout << "金錢不足，無法蓋" << ((this->house == 4) ? "旅館\n" : "房子\n");
                }
            }
            
        }


void Player::tryEscapeJail() {
            if (this->inJail >= 3) {
                this->inJail = 0;
                string s = "", msg = "";
                msg.append(this->name).append(" was released from jail.");
                commandLog(s, msg);
                cout << this->name << " 從監獄中被釋放了\n";
                sendToAllLivePlayer(this->gameboard, s);
                this->lastMove = roll();
                string st = "";
                commandRoll(st, this->lastMove.d1, this->lastMove.d2);
                sendToUser(this->sockfd, st);
                char buf[MAXLINE];
                if (this->gameboard->waitForTPInput(buf) <= 0) {
                    return;
                }
                cout << buf << "|" << strlen(buf) << "\n";
            } else {
                Dice dice = roll();
                string s = "", msg = "";
                commandRoll(s, dice.d1, dice.d2);
                sendToUser(this->sockfd, s);
                char buf[MAXLINE];
                if (this->gameboard->waitForTPInput(buf) <= 0) {
                    return;
                }
                cout << buf << "|" << strlen(buf) << "\n";
                msg.append(this->name).append(" rolled ").append(to_string(dice.d1)).append(" and ").append(to_string(dice.d2)).append(".");
                s = "";
                commandLog(s, msg);
                msg = "";
                if (dice.d1 == dice.d2) {
                    this->inJail = 0;
                    this->lastMove = dice;
                    msg.append(this->name).append(" was released from jail.");
                    commandLog(s, msg);
                    sendToAllLivePlayer(this->gameboard, s);
                } else {
                    cout << this->name << " 逃獄失敗，剩餘 " << 3 - this->inJail << " 回合\n";
                    string s = "", msg = "";
                    msg.append(this->name).append(" failed to escape from jail, ").append(to_string(3 - this->inJail)).append(" rounds left.");
                    commandLog(s, msg);
                    sendToAllLivePlayer(this->gameboard, s);
                    this->inJail++;
                }
                
            }
        }

int Player::getUserInput(char *buf) {
    sleep_for(seconds(1));
    int n = Readline(this->sockfd, buf, MAXLINE); //scanf("%d", &input);
    if (n <= 0) {
        if (errno == EINTR) {
            this->gameboard->playerTimeout(this->id);
        } else if (n == EOF) {
            this->gameboard->playerLeave(this->id);
        } else {
            this->gameboard->playerLeave(this->id);
        }
    }
    return n;
}
int Field::getUserInput(char *buf) {
    sleep_for(seconds(1));
    int n = Readline(this->gameboard->getTurnPlayer()->getSockfd(), buf, MAXLINE);
    if (n <= 0) {
        if (errno == EINTR) {
            this->gameboard->turnPlayerTimeout();
        } else if (n == EOF) {
            this->gameboard->turnPlayerLeave();
        } else {
            this->gameboard->turnPlayerLeave();
        }
    }
    return n;
}
int Gameboard::getUserInput(char *buf) {
    sleep_for(seconds(1));
    int n = Readline(this->getTurnPlayer()->getSockfd(), buf, MAXLINE);
    if (n <= 0) {
        if (errno == EINTR) {
            this->turnPlayerTimeout();
        } else if (n == EOF) {
            this->turnPlayerLeave();
        } else {
            this->turnPlayerLeave();
        }
    }
    return n;
}

void sendToAllLivePlayer(Gameboard *board, char *str) {
    vector<Player*> *v = board->getSurvivors();
    for (int i = 0; i < (int)(v->size()); i++) {
        sendToUser((*v)[i]->getSockfd(), str);
    }
    sleep_for(seconds(1));
}

void sendToAllLivePlayer(Gameboard *board, string &str) {
    vector<Player*> *v = board->getSurvivors();
    for (int i = 0; i < (int)(v->size()); i++) {
        sendToUser((*v)[i]->getSockfd(), str);
    }
    sleep_for(seconds(1));
}


void commandSetPlayers(string &s, Gameboard *board) {
    s = "setplayers ";
    s.append(to_string(board->getPlayerNumber()));
    for (int i = 0; i < board->getPlayerNumber(); i++) {
        s.append(" ").append(board->getPlayer(i)->getName());
    }
    s.append("/\n");
}



void game(WaitingRoom *room) {
    Gameboard board = Gameboard(room);
    string s = "";
    commandSetPlayers(s, &board);
    sendToAllLivePlayer(&board, s);
    //int maxfdp1;
    fd_set rset;
    FD_ZERO(&rset);
    
    int turnNum = 1;
    Signal(SIGALRM, sig_alrm);
    while (true) {
        alarm(60);
        cout << "\n\n\n\n- - - - - - - - - - - - - Round " << turnNum++ << " - - - - - - - - - - - - -\n\n";
        board.nextTurn();
        if (board.isTurnPlayerBankrupt()) goto TurnEnd;
        if (board.getTurnPlayer()->isInJail()) {
            board.getField(10)->execute(board.getTurnPlayer());
            if (board.isTurnPlayerBankrupt()) goto TurnEnd;
            if (board.getTurnPlayer()->isInJail()) {
                if (board.isTurnPlayerBankrupt()) goto TurnEnd;
                continue;
            } else {
                if (board.isTurnPlayerBankrupt()) goto TurnEnd;
                board.getTurnPlayer()->move(board.getTurnPlayer()->getLastMove());
            }
        } else {
            Dice dice = roll();
            string s = "", msg = "";
            commandRoll(s, dice.d1, dice.d2);
            sendToUser(board.getTurnPlayer()->getSockfd(), s);
            char buf[MAXLINE];
            board.waitForTPInput(buf);
            cout << buf << "|" << strlen(buf) << "\n";
            if ((strcmp(buf, "OK") == 0) || (strcmp(buf, "OK\n") == 0)) {
                cout << "roll\n";
            }
            s = "";
            msg.append(board.getTurnPlayer()->getName()).append(" rolled ").append(to_string(dice.d1)).append(" and ").append(to_string(dice.d2)).append(".");
            commandLog(s, msg);
            sendToAllLivePlayer(&board, s);
            board.getTurnPlayer()->move(dice);
            if (dice.d1 == dice.d2) {
                board.samePlayerNextTurn();
            }
        }
        board.printTrunPlayerMove();
        board.checkTurnPlayerPassStart();
        if (board.isTurnPlayerBankrupt()) goto TurnEnd;
        board.turnPlayerTriggerField();
        if (board.isTurnPlayerBankrupt()) goto TurnEnd;

        //char buf[MAXLINE];
        //board.waitForTPInput(buf);
TurnEnd:
        alarm(0);
        board.checkEnd();
        if ((board.isEnded())) {
            break;
        }
    }

    if ((board.isEnded())) {
        int id = board.winner();
        if (id != -1) {
            string win = "win/\n";
            sendToUser(board.getPlayer(id)->getSockfd(), const_cast<char *>(win.c_str()));
            cout << board.getPlayer(id)->getName() << " 贏了！\n";
        }
    }
    
    sleep_for(seconds(10));
    
}

/*
int main () {
    
        unsigned int seed;
        seed = (unsigned int)time(NULL);
        srand(seed);
    */
    
   /*
    srand(42); // for test
    WaitingRoom room = {3, {"Explosion0w0", "kwkwkwkak", "LIAN26880912"}, {3000, 3001, 3002}}; // for test
    game(&room);
}
*/

int main(int argc, char **argv)
{
    
	int					listenfd, connfd, nready, maxfdp1; //connfd1, connfd2;
	pid_t				childpid;
	struct sockaddr_in	servaddr, cliaddr; //cliaddr1, cliaddr2, servaddr;
	socklen_t			clilen;
	void				sig_chld(int);
    fd_set				rset;

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERV_PORT);

	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
	Listen(listenfd, LISTENQ);
	Signal(SIGCHLD, sig_chld);	

    int pNum = 0;
    WaitingRoom room = WaitingRoom();

    if (!pNum) {
        if ( (connfd = accept(listenfd, (SA *) &cliaddr, &clilen)) < 0) {
            if (errno == EINTR);
                //continue;		/* back to for() */
            else
                err_sys("accept error");
        } else {
            pNum += 1;
            char buf[MAXLINE];
            int n = Read(connfd, buf, MAXLINE-1);
            buf[n-1] = '\0';
            room.addPlayer(buf, connfd);
            
            Writen(connfd, const_cast<char*>("isfirst/\n"), 9);
            cout << buf << " is First\n";
        }

        for (;;){

            FD_ZERO(&rset);
            FD_SET(listenfd, &rset);
            FD_SET(room.sockfds[0], &rset);
            maxfdp1 = max(listenfd, room.sockfds[0]) + 1;

            if ( (nready = select(maxfdp1, &rset, NULL, NULL, NULL)) < 0) {
                if (errno == EINTR)
                    continue;		
                else
                    err_sys("select error");
            }

            if (FD_ISSET(listenfd, &rset)){
                clilen = sizeof(cliaddr);
                bzero(&connfd, sizeof(connfd));
                if ( (connfd = accept(listenfd, (SA *) &cliaddr, &clilen)) < 0) {
                    if (errno == EINTR);
                        //continue;		
                    else
                        err_sys("accept error");
                } 

                /*
                //阻止更多人加入
                else if(pNum >= 8){
                    char sendline[MAXLINE];
                    snprintf(sendline, MAXLINE, "You are the #%d player. Wait for game starting.\n", pNum);
                    Write(connfd, sendline, MAXLINE);
                    Close(connfd);
                }
                */
                else {
                    pNum += 1;
                    char buf[MAXLINE];
                    int n = Readline(connfd, buf, MAXLINE-1);
                    buf[n-1] = '\0';
                    room.addPlayer(buf, connfd);
                    if(pNum > 0) FD_SET(connfd, &rset);
                    cout << buf << " is #" << pNum << "\n";
                }

            }
            
            //按下開始
            if(FD_ISSET(room.sockfds[0], &rset)) {
                char recvline[MAXLINE];
                int n = Readline(room.sockfds[0], recvline, MAXLINE-1);
                if (n == 0){
                    for (int i = 0; i < room.playerNum; i++){
                        Close(room.sockfds[i]);
                    }
                    cout << "Disconnect\n";
                    return 0;
                }
                recvline[n-1] = '\0';
                if (strcmp(recvline, "START") == 0){

                    if ((childpid = Fork()) == 0) {    
                        Close(listenfd);	
                        fputs("game start\n", stdout);
                        srand(42); // for test
                        WaitingRoom r = room;
                        int n = r.playerNum;
                        game(&r);
                        for (int i = 0; i < n; i++){
                            Close(r.sockfds[i]);
                        }
                        exit(0);
                    }
                    wait(0);
                    Close(listenfd);
                    return 0;
                }
            }
        }    
    }
}


