#ifndef MAIN_WINDOW_PTR_INCLUDED
#define MAIN_WINDOW_PTR_INCLUDED

#include <QMainWindow>

// Not sure why Qt global variables or QApplication doesn't already have this, but this is what we got.
extern QMainWindow * qMainWindow;

#endif // MAIN_WINDOW_PTR_INCLUDED
