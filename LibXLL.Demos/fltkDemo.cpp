// fltkDemo.cpp
//
// Minimal FLTK GUI executable — verifies that the library links and works.

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/fl_ask.H>

static void on_greet(Fl_Widget*, void*)
{
    fl_message("Hello from FLTK!");
}

int main()
{
    Fl_Window window(340, 160, "FLTK inside LibXLL");
    Fl_Box    box(20, 20, 300, 60, "Hello from FLTK!");
    box.box(FL_UP_BOX);
    box.labelfont(FL_BOLD);
    box.labelsize(18);
    Fl_Button btn(120, 100, 100, 30, "Greet");
    btn.callback(on_greet);
    window.end();
    window.show();
    return Fl::run();
}
