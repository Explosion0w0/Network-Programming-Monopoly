#include <string>
#include <map>
#include <cstdio>
//#include <unistd.h>
//#include <fcntl.h>
//#include <sys/socket.h>
//#include <sys/types.h>
//#include <arpa/inet.h>
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

#define SERV_PORT 9877
#define MAXLINE 1024

int playerCount = 8;
int asd = 0;
string boardImageSrc = "../assets/board.jpg",
    playerImageSrc[8] = {"../assets/01.png", "../assets/02.png", "../assets/03.png", "../assets/04.png", "../assets/05.png", "../assets/06.png", "../assets/07.png", "../assets/08.png"},
    propertyImageSrc[8] = {"../assets/house00.png", "../assets/house01.png", "../assets/house02.png", "../assets/house03.png", "../assets/house04.png", "../assets/hotel.png"},
    vertPropertyImageSrc[6] = {"../assets/house00vertical.png", "../assets/house01vertical.png", "../assets/house02vertical.png", "../assets/house03vertical.png", "../assets/house04vertical.png", "../assets/hotelvertical.png"},
eventPrefixes[] = {"move ", "build ", "display "};

wxPoint topLeft[40] = 
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
},
propertyCoord[22] =
{
    wxPoint(708, 782), wxPoint(561, 782), wxPoint(339, 782), wxPoint(192, 782),
    wxPoint(118, 782), wxPoint(91, 708), wxPoint(91, 561), wxPoint(91, 487),
    wxPoint(91, 339), wxPoint(91, 192), wxPoint(91, 118), wxPoint(118, 91),
    wxPoint(265, 91), wxPoint(339, 91), wxPoint(487, 91), wxPoint(561, 91),
    wxPoint(708, 91), wxPoint(782, 118), wxPoint(782, 192), wxPoint(782, 339),
    wxPoint(782, 561), wxPoint(782, 708)
}, propertyOffset = wxPoint(3, 3);

enum
{
    ID_TIMER = 1,
    ID_BUTTONDICE = 2,
    ID_BUTTONBUY = 3
};

ssize_t
readLine(int fd, char *buffer, size_t n)
{
    ssize_t numRead;                    /* # of bytes fetched by last read() */
    size_t totRead;                     /* Total bytes read so far */
    char *buf;
    char ch;

    if (n <= 0 || buffer == NULL) {
        errno = EINVAL;
        return -1;
    }

    buf = buffer;                       /* No pointer arithmetic on "void *" */

    totRead = 0;
    for (;;) {
        numRead = read(fd, &ch, 1);

        if (numRead == -1) {
            if (errno == EINTR)         /* Interrupted --> restart read() */
                continue;
            else
                return -1;              /* Some other error */

        } else if (numRead == 0) {      /* EOF */
            if (totRead == 0)           /* No bytes read; return 0 */
                return 0;
            else                        /* Some bytes read; add '\0' */
                break;

        } else {                        /* 'numRead' must be 1 if we get here */
            if (totRead < n - 1) {      /* Discard > (n - 1) bytes */
                totRead++;
                *buf++ = ch;
            }

            if (ch == '\n')
                break;
        }
    }

    *buf = '\0';
    return totRead;
}

wxDECLARE_EVENT(wxEVT_THREAD_COMPLETE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_THREAD_MOVE_PLAYER, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_THREAD_LOG, wxCommandEvent);

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
    MyThread *m_pThread;
    wxCriticalSection m_pThreadCS;
protected:
    
    wxButton* buttonDice;
    wxButton* buttonBuy;
    wxStaticBitmap* imageCtrl, *imgPlayers[8], *imgProperty[22];
    wxTimer* timer;
    wxTextCtrl *textDisplay;
    void OnButtonDiceClick(wxCommandEvent& event);
    void OnButtonBuyClick(wxCommandEvent& event);
    int sockfd;
    int playerLocations[8];
    int propertyState[22];

    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_BUTTON(ID_BUTTONDICE, MyFrame::OnButtonDiceClick)
    EVT_BUTTON(ID_BUTTONBUY, MyFrame::OnButtonBuyClick)
    EVT_CLOSE(MyFrame::OnClose)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD_LOG, MyFrame::logAction)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD_COMPLETE, MyFrame::OnThreadCompletion)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD_MOVE_PLAYER, MyFrame::movePlayer)
wxEND_EVENT_TABLE()

wxDEFINE_EVENT(wxEVT_THREAD_COMPLETE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_THREAD_LOG, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_THREAD_MOVE_PLAYER, wxCommandEvent);

bool MyApp::OnInit()
{
    int connfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in	servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);

    if(connect(connfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
    {
        wxLogError("Failed to connect to local host");
        return(-1);
    }

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
        int ready = select(sockfd + 1, &rset, NULL, NULL, &timeout);

        bzero(&buff, MAXLINE);

        if (ready == -1)
        {
            wxMessageOutputDebug().Printf("-1");
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
            ssize_t bytesRead = readLine(sockfd, buff, MAXLINE);
            wxMessageOutputDebug().Printf(buff);
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

            
            int playerId, steps;

            //case move player
            if(sscanf(buff, "%d %d", &playerId, &steps) == 2)
            {
                wxCommandEvent evt(wxEVT_THREAD_MOVE_PLAYER, wxID_ANY);
                evt.SetInt(playerId);
                for(int i = 0 ; i < steps ; ++i)
                {
                    m_pHandler->GetEventHandler()->AddPendingEvent(evt);    
                    this->Sleep(75);
                }
            }
            
            //case build house

            //case log
            // wxCommandEvent evt(wxEVT_THREAD_LOG, wxID_ANY);
            // evt.SetString(buff);

            m_pHandler->GetEventHandler()->AddPendingEvent(evt);    
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
        imgPlayers[playerId]->Move(topLeft[newLocation] + squareOffset + playerOffset[playerId]);
    else
        imgPlayers[playerId]->Move(topLeft[newLocation] + blockOffset[newLocation / 10] + playerOffset[playerId]);

    imgPlayers[playerId]->Refresh();
    
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

    DoStartThread();
    wxPanel* panel = new wxPanel(this);
        
    wxBitmap bmp(boardImageSrc, wxBITMAP_TYPE_JPEG);
    imageCtrl = new wxStaticBitmap(panel, wxID_ANY, bmp, wxPoint(0, 0));


    textDisplay = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxPoint(950, 50), wxSize(500, 700),
                                  wxTE_MULTILINE | wxTE_READONLY | wxHSCROLL | wxVSCROLL);
    textDisplay->SetValue("Welcome!\n");


    for(int i = 0 ; i < playerCount; ++i)
    {
        wxBitmap bmp(playerImageSrc[i], wxBITMAP_TYPE_PNG);
        imgPlayers[i] = new wxStaticBitmap(panel, wxID_ANY, bmp, topLeft[0] + squareOffset + playerOffset[i]);
    }

    for(int i = 0 ; i < 22; ++i)
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
    }
    
    buttonDice = new wxButton(panel, ID_BUTTONDICE, "Roll Dice", wxPoint(950, 800), wxSize(100, 30));
    buttonBuy = new wxButton(panel, ID_BUTTONBUY, "Buy", wxPoint(1150, 800), wxSize(100, 30));
    
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
    
}

void MyFrame::OnButtonBuyClick(wxCommandEvent& event) 
{
    int tmp = asd%6;
    for(int i = 0 ; i < 22 ; ++i)
    {
        if(propertyCoord[i].y >= 782 || propertyCoord[i].y < 118)
        {
            wxBitmap bmp(propertyImageSrc[tmp], wxBITMAP_TYPE_PNG);
            imgProperty[i]->SetBitmap(bmp);
        }
        else
        {
            wxBitmap bmp(vertPropertyImageSrc[tmp], wxBITMAP_TYPE_PNG);
            imgProperty[i]->SetBitmap(bmp);
        }
    }
    asd++;
}


wxIMPLEMENT_APP(MyApp);


