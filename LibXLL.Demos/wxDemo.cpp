// wxDemo.cpp
// Minimal wxWidgets smoke-test: opens a window with a label and a button.
// Build it and run it; if the window appears and the button works, wxWidgets
// is properly linked on this development machine.

#include <wx/wx.h>

// ---------------------------------------------------------------------------
// Application class
// ---------------------------------------------------------------------------
class DemoApp : public wxApp
{
public:
    bool OnInit() override;
};

wxIMPLEMENT_APP(DemoApp); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)

// ---------------------------------------------------------------------------
// Main window
// ---------------------------------------------------------------------------
class DemoFrame : public wxFrame
{
public:
    explicit DemoFrame();

private:
    void OnButtonClick(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    wxPanel*      m_panel  = nullptr;
    wxStaticText* m_label  = nullptr;
    wxButton*     m_button = nullptr;
    int           m_clicks = 0;
};

// ---------------------------------------------------------------------------
// DemoApp implementation
// ---------------------------------------------------------------------------
bool DemoApp::OnInit()
{
    auto* frame = new DemoFrame();
    frame->Show(true);
    return true;
}

// ---------------------------------------------------------------------------
// DemoFrame implementation
// ---------------------------------------------------------------------------
DemoFrame::DemoFrame()
    : wxFrame(nullptr, wxID_ANY, "wxWidgets Demo",
              wxDefaultPosition, wxSize(360, 200))
{
    m_panel = new wxPanel(this);
    auto* vbox  = new wxBoxSizer(wxVERTICAL);

    m_label = new wxStaticText(m_panel, wxID_ANY,
                               "wxWidgets " wxVERSION_NUM_DOT_STRING " is working!",
                               wxDefaultPosition, wxDefaultSize,
                               wxALIGN_CENTRE_HORIZONTAL);

    m_button = new wxButton(m_panel, wxID_ANY, "Click me");

    vbox->AddStretchSpacer(1);
    vbox->Add(m_label,  0, wxALIGN_CENTER | wxALL, 8);
    vbox->Add(m_button, 0, wxALIGN_CENTER | wxALL, 8);
    vbox->AddStretchSpacer(1);

    m_panel->SetSizer(vbox);
    Centre();

    m_button->Bind(wxEVT_BUTTON, &DemoFrame::OnButtonClick, this);
    Bind(wxEVT_CLOSE_WINDOW,    &DemoFrame::OnClose,       this);
}

void DemoFrame::OnButtonClick(wxCommandEvent& /*event*/)
{
    ++m_clicks;
    m_label->SetLabel(wxString::Format("Button clicked %d time(s)!", m_clicks));
    m_panel->Layout();   // reflow so the new text fits
}

void DemoFrame::OnClose(wxCloseEvent& event)
{
    Destroy();
}






