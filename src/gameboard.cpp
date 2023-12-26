#include <string>
#include <vector>

using namespace std;


//this is for server
/*
    To Do:
        不知道field要不要分成好幾個subclass（可以把function區分開來，但是initialize很麻煩）
        ，還是用int type區分就好（省力）

        各類的execute()

        

*/

struct WaitingRoom {
    int playerNum;
    string playerNames[8];
    int sockfds[8];
};



class Card {    // 機會 or 命運卡

};


class Player {
    public:
        Player(): id(0), name(""), money(0), sockfd(0) {};
        Player(int id, string name, int sockfd): id(id), name(name), money(0), sockfd(sockfd) {};
    private:
        int id;
        string name;
        int money;
        int sockfd;
};


class Field {   // 格子
    public:
        Field(): type(-1), name("") {};
        Field(int type, string name): type(type), name(name) {};
        virtual void execute(Player *player) = 0; // 執行動作
    private:
        int type; // -1:Empty 0:起點 1:土地 2:車站 3:公共事業 4:機會 5:命運 6:入獄 7:監獄
        string name;
};


class StartLine : Field {   // 起點
    public:
        StartLine(): Field(0, "起點") {};
    private:
};


class Land : Field {    // 土地
    public:
        Land(): Field(1, ""), color(-1), siblingNum(0), owner(nullptr) {this->siblings = new Land*[2];};
        Land(string name, int color, int siblingNum): Field(1, name), color(color), siblingNum(0), owner(nullptr) {this->siblings = new Land*[2];};
        ~Land() {delete[] this->siblings;};
    private:
        Player *owner;
        int house;
        int color; // -1:Empty
        int siblingNum;
        Land **siblings;// at most 2;
};


class Railway : Field {     // 車站
    public:
        Railway(): Field(2, ""), owner(nullptr) {this->siblings = new Railway*[3];};
        Railway(string name): Field(2, name), owner(nullptr) {this->siblings = new Railway*[3];};
        ~Railway() {delete[] this->siblings;};
    private:
        Player *owner;
        Railway **siblings;// 3;
};


class Utility : Field {     // 公共事業
    public:
        Utility(): Field(3, ""), owner(nullptr) {};
        Utility(string name): Field(3, name), owner(nullptr) {};
    private:
        Player *owner;
};


class Chance : Field {      // 機會
    public:
        Chance(): Field(4, "機會") {};
    private:
};


class Destiny : Field {     // 命運(原版的好像是啥chest之類的，但是中文大部分是命運)
    public:
        Destiny(): Field(5, "命運") {};
    private:
};


class GoToJail : Field {    // 入獄
    public:
        GoToJail(): Field(6, "入獄") {};
    private:
};


class Jail : Field {    // 監獄
    public:
        Jail(): Field(7, "監獄") {};
    private:
};


class Gameboard {   // 遊戲盤 aka 整個遊戲（包括銀行、玩家、場地）
    public:
        Gameboard(): playerNum(8) {
            this->players = new Player[8];
            this->bankruptStat = new int[8]{0};
            this->fields = new Field*[40];
        };
        Gameboard(int playerNum, string *playerNames, int *sockfds): playerNum(playerNum) {
            this->players = new Player[8];
            this->bankruptStat = new int[8]{0};
            this->fields = new Field*[40];
            for (int i = 0; i < playerNum; i++) {
                this->players[i] = Player(i, playerNames[i], sockfds[i]);
            }
        };
        ~Gameboard() {
            delete[] this->players;
            delete[] this->bankruptStat;
            delete[] this->fields;
        }
        void setField(int num, Field *field) {
            this->fields[num] = field;
        }
    private:
        int playerNum;
        Player *players;
        int *bankruptStat;
        Field** fields;
};




Gameboard *createGame (WaitingRoom *room) {
    /*todo*/
    Gameboard* board = new Gameboard(room->playerNum, room->playerNames, room->sockfds);
    //board->setField(0, (Field *) new StartLine());
    return board;    
};