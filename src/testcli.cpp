#include <string>
#include <map>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <cstdio>
#include <vector>
#include <sstream>
#include <wx/wxprec.h>
#include <wx/thread.h>
#include <wx/event.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

extern "C" { 
    #include <unp.h>
} 

using namespace std;

#define INIT_BALANCE 1500

string boardImageSrc = "../assets/board.jpg",
    playerImageSrc[8] = {"../assets/01.png", "../assets/02.png", "../assets/03.png", "../assets/04.png", "../assets/05.png", "../assets/06.png", "../assets/07.png", "../assets/08.png"},
    propertyImageSrc[8] = {"../assets/house00.png", "../assets/house01.png", "../assets/house02.png", "../assets/house03.png", "../assets/house04.png", "../assets/hotel.png"},
    vertPropertyImageSrc[6] = {"../assets/house00vertical.png", "../assets/house01vertical.png", "../assets/house02vertical.png", "../assets/house03vertical.png", "../assets/house04vertical.png", "../assets/hotelvertical.png"};

vector<string> eventPrefixes = {"move ", "build ", "log ", "balance ", "roll", "moveto ", "addprop ", "remprop ", "asktobuy", "asktosell", "isfirst", "setplayers ", "remplayer ", "ownprop "};

//usage: (split commands with /, for example: move 3 7/log player 3 moved for 7 steps)
//       move playerId steps    (moves playerId for a number of steps)
//       build propertyId diff  (upgrades or downgrades property at propertyId by diff level)
//       log message            (displays message in the text box)
//       balance number         (if number is negative, number will be deducted from balance)
//       roll                   (client will return OK to server once button is clicked)
//       moveto playerId index  (moves player directly to index)
//       addprop string         (string should be in the format of propertyName $price)
//       remprop prefix         (remove the first option that starts with prefix within the combo box)
//       asktobuy               (asks player to buy a property, player will return "YES\n" or "NO\n")
//       asktosell              (asks player to sell one of their properties. player will return "nameofproperty\n")
//       isfirst                (allows the user to start the game, when they do so the returned message is "START\n")
//       setplayers playerCount playername0 playername1 playername2..... (tells the client how many players to render and their player names)
//       remplayer playerId     (removes player from the game (unrender))
//       ownprop playerId propertyId    (set property to be owned by playerId)

wxPoint coords[40] = 
{
    wxPoint(782, 782), wxPoint(708, 782), wxPoint(634, 782), wxPoint(561, 782),
    wxPoint(487, 782), wxPoint(413, 782), wxPoint(339, 782), wxPoint(265, 782),
    wxPoint(192, 782), wxPoint(118, 782), wxPoint(0, 782), wxPoint(0, 708),
    wxPoint(0, 634), wxPoint(0,561), wxPoint(0, 487), wxPoint(0, 413),
    wxPoint(0, 339), wxPoint(0,265), wxPoint(0, 192), wxPoint(0, 118),
    wxPoint(0, 0), wxPoint(118, 0), wxPoint(192, 0), wxPoint(265, 0),
    wxPoint(339, 0), wxPoint(413, 0), wxPoint(487, 0), wxPoint(561, 0),
    wxPoint(634, 0), wxPoint(708, 0), wxPoint(782, 0), wxPoint(782, 118),
    wxPoint(782, 192), wxPoint(782, 265), wxPoint(782, 339), wxPoint(782, 413),
    wxPoint(782, 487), wxPoint(782, 561), wxPoint(782, 634), wxPoint(782, 708)
}, 
blockOffset[4] = 
{
    wxPoint(3, 59), wxPoint(10, 37), wxPoint(3, 40), wxPoint(30, 37)
}, 
squareOffset = wxPoint(25, 59),
playerOffset[8] = 
{
    wxPoint(0, -17), wxPoint(17, -17), wxPoint(34, -17), wxPoint(51, -17),
    wxPoint(0, 0), wxPoint(17, 0), wxPoint(34, 0), wxPoint(51, 0)
}, propertyOffset = wxPoint(3, 3),
ownLabelOffset[4] = 
{
    wxPoint(6, -8), wxPoint(117, 6), wxPoint(6, 117), wxPoint(-8, 6)
};

wxColour playerColors[8] = 
{
    wxColour(236, 28, 36), wxColour(255, 127, 39), wxColour(255, 242, 0), wxColour(101, 246, 38),
    wxColour(0, 168, 243), wxColour(184, 61, 186), wxColour(255, 255, 255), wxColour(88, 88, 88)
};

map<int, wxPoint> propertyCoord =
{
    {1, wxPoint(708, 782)}, {3, wxPoint(561, 782)}, {6, wxPoint(339, 782)}, {8, wxPoint(192, 782)},
    {9, wxPoint(118, 782)}, {11, wxPoint(91, 708)}, {13, wxPoint(91, 561)}, {14, wxPoint(91, 487)},
    {16, wxPoint(91, 339)}, {18, wxPoint(91, 192)}, {19, wxPoint(91, 118)}, {21, wxPoint(118, 91)},
    {23, wxPoint(265, 91)}, {24, wxPoint(339, 91)}, {26, wxPoint(487, 91)}, {27, wxPoint(561, 91)},
    {29, wxPoint(708, 91)}, {31, wxPoint(782, 118)}, {32, wxPoint(782, 192)}, {34, wxPoint(782, 339)},
    {37, wxPoint(782, 561)}, {39, wxPoint(782, 708)}
};

map<string, int> propertyIndices =
{
    {"Keelung City", 1}, {"Miaoli Count(r)y", 3}, {"Taitung Train Station", 5}, {"Penghu County", 6},
    {"Kinmen County", 8}, {"Lianjian County", 9}, {"Taitung County", 11}, {"Taiwan Power Company", 12},
    {"Hualien County", 13}, {"Yilan County", 14}, {"Tainan Train Station", 15}, {"Pingtung County", 16},
    {"Kaohsiung City", 18}, {"Tainan City", 19}, {"Chiayi City", 21}, {"Chiayi County", 23}, {"Yunlin County", 24},
    {"Taichung Train Station", 25}, {"Nantou County", 26}, {"Changhua County", 27}, {"Taiwan Water Corporation", 28},
    {"Taichung City", 29}, {"Hsinchu County", 31}, {"Hsinchu City", 32}, {"Taoyuan City", 34}, {"Taipei Train Station", 35},
    {"New Taipei City", 37}, {"Taipei City", 39}
};


enum
{
    ID_TIMER = 1,
    ID_BUTTONDICE = 2,
    ID_BUTTONBUY = 3,
    ID_BUTTONDONTBUY = 4,
    ID_BUTTONSELL = 5,
    ID_BUTTONSTART = 6
};

wxDECLARE_EVENT(wxEVT_THREAD_COMPLETE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_THREAD_MOVE_PLAYER, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_THREAD_LOG, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_THREAD_BUILD_PROPERTY, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_THREAD_MODIFY_BALANCE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_THREAD_MOVE_TO, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_THREAD_ROLL, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_THREAD_ADD_PROPERTY, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_THREAD_REMOVE_PROPERTY, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_THREAD_ASK_TO_BUY, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_THREAD_ASK_TO_SELL, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_THREAD_IS_FIRST, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_THREAD_SET_PLAYERS, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_THREAD_REMOVE_PLAYER, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_THREAD_OWN_PROPERTY, wxCommandEvent);

class MyFrame;

class MyThread : public wxThread
{
public:
    MyThread(MyFrame *handler, int connfd)
        : wxThread(wxTHREAD_DETACHED)
        { m_pHandler = handler;
          sockfd = connfd;}
    ~MyThread();
protected:
    int sockfd;
    virtual ExitCode Entry();
    MyFrame *m_pHandler;
};

class MyApp : public wxApp
{
public:
    bool OnInit() override;
};

class MyFrame : public wxFrame
{
public:
    MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size, int sockfd);

    void DoStartThread();
    void OnClose(wxCloseEvent& event);
    void OnThreadCompletion(wxCommandEvent& event);

    void logAction(wxCommandEvent& event);
    void movePlayer(wxCommandEvent& event);
    void buildProperty(wxCommandEvent& event);
    void modifyBalance(wxCommandEvent& event);
    void moveTo(wxCommandEvent& event);
    void roll(wxCommandEvent& event);
    void addProperty(wxCommandEvent& event);
    void removeProperty(wxCommandEvent& event);
    void askToBuy(wxCommandEvent& event);
    void askToSell(wxCommandEvent& event);
    void isFirst(wxCommandEvent& event);
    void setPlayers(wxCommandEvent& event);
    void removePlayer(wxCommandEvent& event);
    void ownProperty(wxCommandEvent& event);

    MyThread *m_pThread;
    wxCriticalSection m_pThreadCS;

protected:
    
    wxButton *buttonDice;
    wxButton *buttonBuy;
    wxButton *buttonDontBuy;
    wxButton *buttonSell;
    wxButton *buttonStart;
    wxStaticBitmap *imageCtrl, *imgPlayers[8], *imgProperty[40];
    wxTimer *timer;
    wxTextCtrl *textDisplay, *balanceDisplay;
    wxControl *ownLabel[40];
    wxComboBox *ownedProperties;
    wxStaticText *playerNames[8];

    void OnButtonDiceClick(wxCommandEvent& event);
    void OnButtonBuyClick(wxCommandEvent& event);
    void OnButtonDontBuyClick(wxCommandEvent& event);
    void OnButtonSellClick(wxCommandEvent& event);
    void OnButtonStartClick(wxCommandEvent& event);

    int sockfd, playerLocations[8], propertyState[40], balance,
        pendingRoll1, pendingRoll2;

    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_BUTTON(ID_BUTTONDICE, MyFrame::OnButtonDiceClick)
    EVT_BUTTON(ID_BUTTONBUY, MyFrame::OnButtonBuyClick)
    EVT_BUTTON(ID_BUTTONDONTBUY, MyFrame::OnButtonDontBuyClick)
    EVT_BUTTON(ID_BUTTONSELL, MyFrame::OnButtonSellClick)
    EVT_BUTTON(ID_BUTTONSTART, MyFrame::OnButtonStartClick)
    EVT_CLOSE(MyFrame::OnClose)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD_LOG, MyFrame::logAction)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD_COMPLETE, MyFrame::OnThreadCompletion)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD_MOVE_PLAYER, MyFrame::movePlayer)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD_BUILD_PROPERTY, MyFrame::buildProperty)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD_MODIFY_BALANCE, MyFrame::modifyBalance)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD_MOVE_TO, MyFrame::moveTo)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD_ROLL, MyFrame::roll)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD_ADD_PROPERTY, MyFrame::addProperty)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD_REMOVE_PROPERTY, MyFrame::removeProperty)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD_ASK_TO_BUY, MyFrame::askToBuy)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD_ASK_TO_SELL, MyFrame::askToSell)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD_IS_FIRST, MyFrame::isFirst)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD_SET_PLAYERS, MyFrame::setPlayers)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD_REMOVE_PLAYER, MyFrame::removePlayer)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD_OWN_PROPERTY, MyFrame::ownProperty)
wxEND_EVENT_TABLE()

wxDEFINE_EVENT(wxEVT_THREAD_COMPLETE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_THREAD_LOG, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_THREAD_MOVE_PLAYER, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_THREAD_BUILD_PROPERTY, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_THREAD_MODIFY_BALANCE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_THREAD_MOVE_TO, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_THREAD_ROLL, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_THREAD_ADD_PROPERTY, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_THREAD_REMOVE_PROPERTY, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_THREAD_ASK_TO_BUY, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_THREAD_ASK_TO_SELL, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_THREAD_IS_FIRST, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_THREAD_SET_PLAYERS, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_THREAD_REMOVE_PLAYER, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_THREAD_OWN_PROPERTY, wxCommandEvent);

bool MyApp::OnInit()
{
    if (argc != 3) {
        wxLogError("Usage: testcli IP username");
        return false;
    }

    int connfd = Socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in	servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

    if(connect(connfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
    {
        wxLogError("Failed to connect to local host");
        return(-1);
    }

    wxCharBuffer buffer = wxString::Format("%s\n", argv[2]).ToUTF8();
    Writen(connfd, buffer.data(), strlen(buffer.data()));

    MyFrame *frame = new MyFrame("My wxWidgets App", wxPoint(50, 50),  wxSize(1600, 990), connfd);
    frame->Show(true);
    return true;
}

void MyFrame::DoStartThread()
{
    m_pThread = new MyThread(this, sockfd);
    if ( m_pThread->Run() != wxTHREAD_NO_ERROR )
    {
        wxLogError("Can't create the thread!");
        delete m_pThread;
        m_pThread = NULL;
    }
}

wxThread::ExitCode MyThread::Entry()
{
    //wxCommandEvent *evt;
    char buff[MAXLINE] = {0};
    fd_set fds, rset;
    FD_ZERO(&fds);
    FD_SET(sockfd, &fds); 
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 50000;
    
    while (!TestDestroy())
    {
        rset = fds;
        int ready = Select(sockfd + 1, &rset, NULL, NULL, &timeout);

        bzero(&buff, MAXLINE);

        if (ready == -1)
        {
            if(errno == EINTR)
                continue;
            else
            {
                wxLogError("Select error, closing thread...");
                break;
            }
        }
        else if (ready == 1)
        {
            ssize_t bytesRead = Readline(sockfd, buff, MAXLINE);

            string buffStr(buff);
            wxMessageOutputDebug().Printf("input: %s", buffStr);

            if (bytesRead == -1)
            {
                wxLogError("Readline error");
                continue;
            }
            else if (bytesRead == 0)
            {
                wxLogError("Server closed prematurely, closing thread...");
                break;
            }

            
            //split string by /
            size_t pos = 0;
            vector<string> arguments;

            while ((pos = buffStr.find('/')) != string::npos) 
            {
                arguments.push_back(buffStr.substr(0, pos));
                buffStr.erase(0, pos + 1);
            }

            arguments.push_back(buffStr);

            for(string argument : arguments)
            {
                for(unsigned int i = 0 ; i < eventPrefixes.size(); ++i)
                {
                    if(argument.size() >= eventPrefixes[i].size() && argument.compare(0, eventPrefixes[i].size(), eventPrefixes[i]) == 0)
                    {
                        argument.erase(0, eventPrefixes[i].size());
                        if(argument.back() != '\n')
                            argument += '\n';

                        switch(i)
                        {
                            case 0: //move
                            {
                                istringstream iss(argument);

                                int playerId, steps;

                                if (iss >> playerId >> steps) 
                                {
                                    wxCommandEvent evt(wxEVT_THREAD_MOVE_PLAYER, wxID_ANY);
                                    evt.SetInt(playerId);
                                    for(int j = 0 ; j < steps ; ++j)
                                    {
                                        m_pHandler->GetEventHandler()->AddPendingEvent(evt);    
                                        this->Sleep(75);
                                    }
                                } 
                                else 
                                    wxMessageOutputDebug().Printf("input: %s", "ERROR: Invalid move command argument: " + argument);

                                break;
                            }
                            case 1: //build
                            {
                                wxCommandEvent evt(wxEVT_THREAD_BUILD_PROPERTY, wxID_ANY);
                                evt.SetString(argument);

                                m_pHandler->GetEventHandler()->AddPendingEvent(evt); 
                                break;
                            }
                            case 2: //log
                            {
                                auto currentTime = chrono::system_clock::to_time_t(chrono::system_clock::now());

                                tm* localTime = std::localtime(&currentTime);

                                ostringstream oss;
                                oss << put_time(localTime, "%H:%M");
                                string currentTimeStr = oss.str();

                                wxCommandEvent evt(wxEVT_THREAD_LOG, wxID_ANY);
                                evt.SetString("[" + currentTimeStr + "] " + argument);

                                m_pHandler->GetEventHandler()->AddPendingEvent(evt);  

                                break;
                            }
                            case 3: //balance
                            {
                                try
                                {
                                    int modifyValue = stoi(argument);
                                    wxCommandEvent evt(wxEVT_THREAD_MODIFY_BALANCE, wxID_ANY);

                                    evt.SetInt(modifyValue);
                                    m_pHandler->GetEventHandler()->AddPendingEvent(evt);   
                                }
                                catch (const invalid_argument& e) 
                                {
                                    wxMessageOutputDebug().Printf("input: %s", "ERROR: Invalid balance command argument: " + argument);
                                }
                                catch (const out_of_range& e) 
                                {
                                    wxMessageOutputDebug().Printf("input: %s", "ERROR: Invalid balance command argument: " + argument);
                                }

                                break;
                            }
                            case 4: //roll
                            {
                                wxCommandEvent evt(wxEVT_THREAD_ROLL, wxID_ANY);
                                // evt.SetString(argument);

                                m_pHandler->GetEventHandler()->AddPendingEvent(evt); 

                                break;
                            }
                            case 5: //moveto
                            {
                                wxCommandEvent evt(wxEVT_THREAD_MOVE_TO, wxID_ANY);
                                evt.SetString(argument);

                                m_pHandler->GetEventHandler()->AddPendingEvent(evt);    
                                break;
                            }
                            case 6: //add property
                            {
                                wxCommandEvent evt(wxEVT_THREAD_ADD_PROPERTY, wxID_ANY);
                                evt.SetString(argument);

                                m_pHandler->GetEventHandler()->AddPendingEvent(evt); 

                                break; 
                            }
                            case 7: //remove property
                            {
                                wxCommandEvent evt(wxEVT_THREAD_REMOVE_PROPERTY, wxID_ANY);
                                evt.SetString(argument);

                                m_pHandler->GetEventHandler()->AddPendingEvent(evt); 

                                break; 
                            }
                            case 8: //ask to buy
                            {
                                wxCommandEvent evt(wxEVT_THREAD_ASK_TO_BUY, wxID_ANY);
                                m_pHandler->GetEventHandler()->AddPendingEvent(evt); 

                                break;
                            }
                            case 9: //ask to sell
                            {
                                wxCommandEvent evt(wxEVT_THREAD_ASK_TO_SELL, wxID_ANY);
                                m_pHandler->GetEventHandler()->AddPendingEvent(evt);

                                break;
                            }
                            case 10: //is first user
                            {
                                wxCommandEvent evt(wxEVT_THREAD_IS_FIRST, wxID_ANY);
                                m_pHandler->GetEventHandler()->AddPendingEvent(evt);

                                break;
                            }
                            case 11: //set player count
                            {
                                wxCommandEvent evt(wxEVT_THREAD_SET_PLAYERS, wxID_ANY);
                                evt.SetString(argument);
                                
                                m_pHandler->GetEventHandler()->AddPendingEvent(evt);

                                break;
                            }
                            case 12: //remove player
                            {
                                try
                                {
                                    int playerId = stoi(argument);
                                    wxCommandEvent evt(wxEVT_THREAD_REMOVE_PLAYER, wxID_ANY);

                                    evt.SetInt(playerId);
                                    m_pHandler->GetEventHandler()->AddPendingEvent(evt);   
                                }
                                catch (const invalid_argument& e) 
                                {
                                    wxMessageOutputDebug().Printf("input: %s", "ERROR: Invalid remplayer command argument: " + argument);
                                }
                                catch (const out_of_range& e) 
                                {
                                    wxMessageOutputDebug().Printf("input: %s", "ERROR: Invalid remplayer command argument: " + argument);
                                }

                                break;
                            }
                            case 13: //own property
                            {
                                wxCommandEvent evt(wxEVT_THREAD_OWN_PROPERTY, wxID_ANY);
                                evt.SetString(argument);
                                
                                m_pHandler->GetEventHandler()->AddPendingEvent(evt);

                                break;                               
                            }
                        }

                        break;
                    }
                }
            } 
        }
    }

    wxQueueEvent(m_pHandler, new wxCommandEvent(wxEVT_THREAD_COMPLETE));
    return (wxThread::ExitCode)0;
}

void MyFrame::OnThreadCompletion(wxCommandEvent&)
{
    wxMessageOutputDebug().Printf("MYFRAME: MyThread exited!\n");
}

void MyFrame::logAction(wxCommandEvent& event)
{
    wxString s = event.GetString();
    textDisplay->AppendText(s);
    textDisplay->ShowPosition(textDisplay->GetLastPosition());
}

void MyFrame::movePlayer(wxCommandEvent& event)
{
    int playerId = event.GetInt();

    playerLocations[playerId]++;
    playerLocations[playerId]%=40;
    
    int newLocation = playerLocations[playerId];

    if(newLocation % 10 == 0) //square
        imgPlayers[playerId]->Move(coords[newLocation] + squareOffset + playerOffset[playerId]);
    else
        imgPlayers[playerId]->Move(coords[newLocation] + blockOffset[newLocation / 10] + playerOffset[playerId]);

    imgPlayers[playerId]->Refresh();
}

void MyFrame::buildProperty(wxCommandEvent& event)
{
    string argument = event.GetString().ToStdString();
    istringstream iss(argument);

    int propertyId, diff;

    if (iss >> propertyId >> diff) 
    {
        if((propertyCoord[propertyId].x == 0 && propertyCoord[propertyId].y == 0)) 
            return;

        propertyState[propertyId] += diff;

        if(propertyCoord[propertyId].y >= 782 || propertyCoord[propertyId].y < 118)
        {
            wxBitmap bmp(propertyImageSrc[propertyState[propertyId]], wxBITMAP_TYPE_PNG);
            imgProperty[propertyId]->SetBitmap(bmp);
        }
        else
        {
            wxBitmap bmp(vertPropertyImageSrc[propertyState[propertyId]], wxBITMAP_TYPE_PNG);
            imgProperty[propertyId]->SetBitmap(bmp);
        }
    }
    else 
        wxMessageOutputDebug().Printf("input: %s", "ERROR: Invalid build command argument: " + argument);

}

void MyFrame::modifyBalance(wxCommandEvent& event)
{
    int val = event.GetInt();
    balance += val;
    balanceDisplay->SetValue("$" + to_string(balance));
}

void MyFrame::roll(wxCommandEvent& event)
{
    // string argument = event.GetString().ToStdString();
    // istringstream iss(argument);

    // if (iss >> pendingRoll1 >> pendingRoll2) 
    //     buttonDice->Show(true); 
    // else 
    //     wxMessageOutputDebug().Printf("input: %s", "ERROR: Invalid roll command argument: " + argument);
    buttonDice->Show(true);
}

void MyFrame::moveTo(wxCommandEvent& event)
{
    string argument = event.GetString().ToStdString();
    istringstream iss(argument);

    int playerId, index;

    if (iss >> playerId >> index) 
    {
        playerLocations[playerId] = index;

        int newLocation = playerLocations[playerId];

        if(newLocation % 10 == 0) //square
            imgPlayers[playerId]->Move(coords[newLocation] + squareOffset + playerOffset[playerId]);
        else
            imgPlayers[playerId]->Move(coords[newLocation] + blockOffset[newLocation / 10] + playerOffset[playerId]);

        imgPlayers[playerId]->Refresh();  
    } 
    else 
        wxMessageOutputDebug().Printf("input: %s", "ERROR: Invalid moveto command argument: " + argument);
}

void MyFrame::addProperty(wxCommandEvent& event)
{
    wxString property = event.GetString();
    ownedProperties->Append(property);
}

void MyFrame::removeProperty(wxCommandEvent& event)
{
    wxString property = event.GetString();
    if(property.EndsWith('\n'))
        property.RemoveLast();

    for (unsigned int i = 0; i < ownedProperties->GetCount(); ++i) 
    {
        wxString itemText = ownedProperties->GetString(i);
        if (itemText.StartsWith(property)) 
        {
            ownedProperties->Delete(i);
            break;
        }
    }
}

void MyFrame::askToBuy(wxCommandEvent& event)
{
    buttonBuy->Show(true);
    buttonDontBuy->Show(true);
}

void MyFrame::askToSell(wxCommandEvent& event)
{
    buttonSell->Show(true);
}

void MyFrame::isFirst(wxCommandEvent& event)
{
    buttonStart->Show(true);
}

void MyFrame::setPlayers(wxCommandEvent& event)
{
    string argument = event.GetString().ToStdString();
    istringstream iss(argument);

    int playerCount;

    iss >> playerCount;

    for(int i = 0 ; i < playerCount; ++i)
    {
        string tmpname;
        iss >> tmpname;
        playerNames[i]->SetLabel(tmpname);
        imgPlayers[i]->Show(true);
    }
}

void MyFrame::removePlayer(wxCommandEvent& event)
{
    int playerId = event.GetInt();
    imgPlayers[playerId]->Show(false);
}

void MyFrame::ownProperty(wxCommandEvent& event)
{
    string argument = event.GetString().ToStdString();
    int playerId, propertyId;

    istringstream iss(argument);

    if(iss >> playerId >> propertyId)
        ownLabel[propertyId]->SetBackgroundColour(playerColors[playerId]);

}

MyThread::~MyThread()
{
    wxCriticalSectionLocker enter(m_pHandler->m_pThreadCS);

    m_pHandler->m_pThread = NULL;
}
 
MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size, int sockfd)
    : wxFrame(NULL, wxID_ANY, title, pos, size, wxDEFAULT_FRAME_STYLE & ~wxRESIZE_BORDER)
{
    this->sockfd = sockfd;

    bzero(&playerLocations, sizeof(playerLocations));
    bzero(&propertyState, sizeof(propertyState));

    balance = INIT_BALANCE;
    pendingRoll1 = pendingRoll2 = 0;

    DoStartThread();
    wxPanel* panel = new wxPanel(this);
        
    wxBitmap bmp(boardImageSrc, wxBITMAP_TYPE_JPEG);
    imageCtrl = new wxStaticBitmap(panel, wxID_ANY, bmp, wxPoint(0, 0));


    textDisplay = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxPoint(950, 150), wxSize(500, 500),
                                  wxTE_MULTILINE | wxTE_READONLY | wxHSCROLL | wxVSCROLL);
    textDisplay->SetValue("Welcome!\n");

    wxStaticText* staticText = new wxStaticText(panel, wxID_ANY, "Balance", wxPoint(1140, 60), wxSize(100, 50));
    wxFont font(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    staticText->SetFont(font);

    balanceDisplay = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxPoint(1250, 50), wxSize(200, 50),
                                  wxTE_READONLY);
    balanceDisplay->SetValue("$1500");

    ownedProperties = new wxComboBox(panel, wxID_ANY, wxEmptyString, wxPoint(950, 700), wxSize(150, 50), wxArrayString(), wxCB_DROPDOWN);

    for(int i = 0 ; i < 8; ++i)
    {
        wxBitmap bmp(playerImageSrc[i], wxBITMAP_TYPE_PNG);
        imgPlayers[i] = new wxStaticBitmap(panel, wxID_ANY, bmp, coords[0] + squareOffset + playerOffset[i]);
        imgPlayers[i]->Show(false);
    }

    for(int i = 0 ; i < 40; ++i)
    {
        wxPoint tmpCoord = propertyCoord[i];

        if(tmpCoord.y > 782 || tmpCoord.y < 118)
        {
            wxBitmap bmp(propertyImageSrc[0], wxBITMAP_TYPE_PNG);
            imgProperty[i] = new wxStaticBitmap(panel, wxID_ANY, bmp, propertyCoord[i] + propertyOffset);
        }
        else
        {
            wxBitmap bmp(vertPropertyImageSrc[0], wxBITMAP_TYPE_PNG);
            imgProperty[i] = new wxStaticBitmap(panel, wxID_ANY, bmp, propertyCoord[i] + propertyOffset);
        }
        if(i % 10 != 0)
            ownLabel[i] = new wxControl(panel, wxID_ANY, coords[i] + ownLabelOffset[i / 10], ((i / 10) % 2 == 0 ? wxSize(62, 8) : wxSize(8, 62)));
    }


    playerNames[0] = new wxStaticText(panel, wxID_ANY, "", wxPoint(215, 185), wxSize(100, 20));
    playerNames[1] = new wxStaticText(panel, wxID_ANY, "", wxPoint(215, 210), wxSize(100, 20));
    playerNames[2] = new wxStaticText(panel, wxID_ANY, "", wxPoint(215, 235), wxSize(100, 20));
    playerNames[3] = new wxStaticText(panel, wxID_ANY, "", wxPoint(215, 260), wxSize(100, 20));
    playerNames[4] = new wxStaticText(panel, wxID_ANY, "", wxPoint(215, 285), wxSize(100, 20));
    playerNames[5] = new wxStaticText(panel, wxID_ANY, "", wxPoint(215, 310), wxSize(100, 20));
    playerNames[6] = new wxStaticText(panel, wxID_ANY, "", wxPoint(215, 335), wxSize(100, 20));
    playerNames[7] = new wxStaticText(panel, wxID_ANY, "", wxPoint(215, 360), wxSize(100, 20));
    
    buttonDice = new wxButton(panel, ID_BUTTONDICE, "Roll Dice", wxPoint(950, 800), wxSize(150, 50));
    buttonBuy = new wxButton(panel, ID_BUTTONBUY, "Buy", wxPoint(950, 800), wxSize(150, 50));
    buttonDontBuy = new wxButton(panel, ID_BUTTONDONTBUY, "Don't Buy", wxPoint(1150, 800), wxSize(150, 50));
    buttonSell = new wxButton(panel, ID_BUTTONSELL, "Sell", wxPoint(950, 800), wxSize(150, 50));
    buttonStart = new wxButton(panel, ID_BUTTONSTART, "START GAME", wxPoint(350, 350), wxSize(200, 200));

    buttonDice->Show(false);
    buttonBuy->Show(false);
    buttonDontBuy->Show(false);
    buttonSell->Show(false);
    buttonStart->Show(false);
    
}

void MyFrame::OnClose(wxCloseEvent&)
{
    {
        wxCriticalSectionLocker enter(m_pThreadCS);
        if (m_pThread)         // does the thread still exist?
        {
            wxMessageOutputDebug().Printf("MYFRAME: deleting thread");
            if (m_pThread->Delete() != wxTHREAD_NO_ERROR )
                wxLogError("Can't delete the thread!");
        }
    }       
    while (1)
    {
        { 
            wxCriticalSectionLocker enter(m_pThreadCS);
            if (!m_pThread) break;
        }
        wxThread::This()->Sleep(1);
    }
    Destroy();
}

void MyFrame::OnButtonDiceClick(wxCommandEvent& event) 
{
    buttonDice->Show(false);

    Writen(sockfd, const_cast<char*>("OK\n"), 4);
}

void MyFrame::OnButtonBuyClick(wxCommandEvent& event) 
{
    buttonBuy->Show(false);
    buttonDontBuy->Show(false);
    Writen(sockfd, const_cast<char*>("YES\n"), 5);
}

void MyFrame::OnButtonDontBuyClick(wxCommandEvent& event) 
{
    buttonBuy->Show(false);
    buttonDontBuy->Show(false);
    Writen(sockfd, const_cast<char*>("NO\n"), 4);
}

void MyFrame::OnButtonSellClick(wxCommandEvent& event) 
{
    wxString selectedProperty = ownedProperties->GetValue();
    if(!selectedProperty.IsEmpty())
    {
        buttonSell->Show(false);

        ownedProperties->Delete(ownedProperties->FindString(selectedProperty));

        string strSelectedProperty = selectedProperty.ToStdString();
        string name = strSelectedProperty.substr(0, strSelectedProperty.find(" $"));

        int index = propertyIndices[name];
        char tmp[5];
        snprintf(tmp, 5, "%d\n", index);

        Writen(sockfd, tmp, strlen(tmp));

        ownedProperties->SetValue("");
    }
}

void MyFrame::OnButtonStartClick(wxCommandEvent& event) 
{
    buttonStart->Show(false);
    Writen(sockfd, const_cast<char*>("START\n"), 7);
}


wxIMPLEMENT_APP(MyApp);




