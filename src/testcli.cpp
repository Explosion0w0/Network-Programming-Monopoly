//#include	"unp.h"
#include <string>
#include <map>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <wx/wxprec.h>
#include <wx/thread.h>
#include <wx/event.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

using namespace std;

#define SERV_PORT 9877

int playerCount = 8;
int asd = 0;
string boardImageSrc = "../assets/board.jpg",
    playerImageSrc[8] = {"../assets/01.png", "../assets/02.png", "../assets/03.png", "../assets/04.png", "../assets/05.png", "../assets/06.png", "../assets/07.png", "../assets/08.png"},
    propertyImageSrc[8] = {"../assets/house00.png", "../assets/house01.png", "../assets/house02.png", "../assets/house03.png", "../assets/house04.png", "../assets/hotel.png"},
    vertPropertyImageSrc[6] = {"../assets/house00vertical.png", "../assets/house01vertical.png", "../assets/house02vertical.png", "../assets/house03vertical.png", "../assets/house04vertical.png", "../assets/hotelvertical.png"};

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
    ID_BUTTON1 = 2,
    ID_BUTTON2 = 3
};


wxDECLARE_EVENT(wxEVT_THREAD_COMPLETE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_THREAD_HANDLER, wxCommandEvent);

class MyFrame;

class MyThread : public wxThread
{
public:
    MyThread(MyFrame *handler)
        : wxThread(wxTHREAD_DETACHED)
        { m_pHandler = handler;}
    ~MyThread();
protected:
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
    MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
    void DoStartThread();
    void OnClose(wxCloseEvent& event);
    void OnThreadCompletion(wxCommandEvent& event);
    void handler(wxCommandEvent& event);
    MyThread *m_pThread;
    wxCriticalSection m_pThreadCS;
protected:
    
    wxButton* button1;
    wxButton* button2;
    wxStaticBitmap* imageCtrl, *imgPlayers[8], *imgProperty[22];
    wxTimer* timer;

    void OnButton1Click(wxCommandEvent& event);
    void OnButton2Click(wxCommandEvent& event);


    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_BUTTON(ID_BUTTON1, MyFrame::OnButton1Click)
    EVT_BUTTON(ID_BUTTON2, MyFrame::OnButton2Click)
    EVT_CLOSE(MyFrame::OnClose)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD_HANDLER, MyFrame::handler)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD_COMPLETE, MyFrame::OnThreadCompletion)
wxEND_EVENT_TABLE()


wxDEFINE_EVENT(wxEVT_THREAD_COMPLETE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_THREAD_HANDLER, wxCommandEvent);

bool MyApp::OnInit()
{
    // int connfd = socket(AF_INET, SOCK_STREAM, 0);
    // struct sockaddr_in	servaddr;
	// bzero(&servaddr, sizeof(servaddr));
	// servaddr.sin_family = AF_INET;
	// servaddr.sin_port = htons(SERV_PORT);
	// inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
	// connect(connfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    MyFrame *frame = new MyFrame("My wxWidgets App", wxPoint(50, 50),  wxSize(1600, 990));
    frame->Show(true);
    return true;
}

void MyFrame::DoStartThread()
{
    m_pThread = new MyThread(this);
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
    while (!TestDestroy())
    {
        //evt = new wxCommandEvent(wxEVT_THREAD_HANDLER);
        wxCommandEvent evt(wxEVT_THREAD_HANDLER, wxID_ANY);
        evt.SetString("joe mama\n");
        
        m_pHandler->GetEventHandler()->AddPendingEvent(evt);
        this->Sleep(50);
    }

    wxQueueEvent(m_pHandler, new wxCommandEvent(wxEVT_THREAD_COMPLETE));
    return (wxThread::ExitCode)0;     // success
}

void MyFrame::OnThreadCompletion(wxCommandEvent&)
{
    wxMessageOutputDebug().Printf("MYFRAME: MyThread exited!\n");
}

void MyFrame::handler(wxCommandEvent& event)
{
    wxString s = event.GetString();
    wxMessageOutputDebug().Printf(s);
}

MyThread::~MyThread()
{
    wxCriticalSectionLocker enter(m_pHandler->m_pThreadCS);

    m_pHandler->m_pThread = NULL;
}
 
MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
    : wxFrame(NULL, wxID_ANY, title, pos, size, wxDEFAULT_FRAME_STYLE & ~wxRESIZE_BORDER)
{
    DoStartThread();
    wxPanel* panel = new wxPanel(this);
        
    wxBitmap bmp(boardImageSrc, wxBITMAP_TYPE_JPEG);
    imageCtrl = new wxStaticBitmap(panel, wxID_ANY, bmp, wxPoint(0, 0));
    
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
    
    button1 = new wxButton(panel, ID_BUTTON1, "Roll Dice", wxPoint(950, 50), wxSize(100, 30));
    button2 = new wxButton(panel, ID_BUTTON2, "Buy", wxPoint(950, 100), wxSize(100, 30));
    
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

void MyFrame::OnButton1Click(wxCommandEvent& event) 
{
    int i = asd % 40;
    for (int j = 0 ; j < 8 ; ++j){
        if(i % 10 == 0) //square
            imgPlayers[j]->Move(topLeft[i] + squareOffset + playerOffset[j]);
        else
            imgPlayers[j]->Move(topLeft[i] + blockOffset[i / 10] + playerOffset[j]);

        imgPlayers[j]->Refresh();
    }
    asd++;
    
}

void MyFrame::OnButton2Click(wxCommandEvent& event) 
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


// int main(int argc, char **argv){
//     wxInitializer initializer(argc, argv);
//     if (!initializer.IsOk()) {
//         wxLogError("Failed to initialize wxWidgets.");
//         return -1;
//     }

//     MyApp app;
//     wxApp::SetInstance(&app);
//     wxEntryStart(argc, argv);
//     app.OnInit();
//     wxEntryCleanup();
    
//}


// using namespace std;
// int
// main(int argc, char **argv)
// {
// 	int					i, sockfd[5];
// 	struct sockaddr_in	servaddr;

// 	if (argc != 2)
// 		err_quit("usage: tcpcli <IPaddress>");
		
		

// 	for (i = 0; i < 5; i++) {
// 		sockfd[i] = Socket(AF_INET, SOCK_STREAM, 0);

// 		bzero(&servaddr, sizeof(servaddr));
// 		servaddr.sin_family = AF_INET;
// 		servaddr.sin_port = htons(SERV_PORT);
// 		Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

// 		Connect(sockfd[i], (SA *) &servaddr, sizeof(servaddr));
// 	}

// 	app(stdin, sockfd[0]);		/* do it all */

// 	exit(0);
// }
