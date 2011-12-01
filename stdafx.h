#pragma once

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>

#include <algorithm>
#include <cstring>
#include <direct.h>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <shellapi.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <time.h>
#include <vector>

#include <wx/wx.h>
#include <wx/msw/private.h>
#include <wx/artprov.h>
#include <wx/bitmap.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/clipbrd.h>
#include <wx/cmdline.h>
#include <wx/dnd.h>
#include <wx/evtloop.h>
#include <wx/image.h>
#include <wx/msw/dialog.h>
#include <wx/msw/private.h>
#include <wx/panel.h>
#include <wx/ptr_scpd.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/taskbar.h>
#include <wx/toplevel.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>

#include <tinyxml/tinyxml.h>

#define TO_STRING(x) #x
#define TODO_STRING(x) TO_STRING(x)
#define TODO(msg) message (__FILE__ "(" TODO_STRING(__LINE__) ") : TODO: " msg)
