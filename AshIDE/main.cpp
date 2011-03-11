//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include "MainWindow.h"

using namespace AshIDE;
using namespace Assembler;

int main(int argc, char **argv)
{
	try
	{
		sProgramDir = CreateStandardPath(FileName(argv[0]).Path);
		Fl::scheme("plastic");
		MainWindow::pMainWindow = new MainWindow();

		list<string> InputList;
		#if defined DEFAULT_LANG_LC3
		DefaultLang = Assembler::LangLC3;
		#elif defined DEFAULT_LANG_LC3B
		DefaultLang = Assembler::LangLC3b;
		#else
		DefaultLang = Assembler::LangNone;
		#endif
		for(int i = 1; i < argc; i++)
		{
			string sFileName(argv[i]);
			if(sFileName.size() > 0 && sFileName[0] == '-')
			{
				//It's a command-line option
				if(ToLower(sFileName) == "-lc3b")
					DefaultLang = LangLC3b;
				else if(ToLower(sFileName) == "-lc3")
					DefaultLang = LangLC3;
				else
					fl_alert("Unrecognized command-line switch %.31s", sFileName.c_str());
				continue;
			}
			else
				InputList.push_back(sFileName);
		}

		for(list<string>::iterator InputIter = InputList.begin(); InputIter != InputList.end(); InputIter++)
		{
			if(ToLower(InputIter->substr(MIN(InputIter->size(), InputIter->find_last_of('.')))) == ".aprj")
				TheMainWindow.OpenProject(*InputIter);
			else
				TheMainWindow.OpenFile(*InputIter);
		}

		return Fl::run();
	}
	catch(const char *sMsg)
	{	//*NOTE: MSVC allows "char", but GCC requires "const"
		fl_alert("AshIDE caught internal exception:%s", sMsg);
		exit(-1);
	}
	catch(runtime_error e)
	{
		fl_alert("AshIDE caught internal exception:%s", e.what());
		exit(-1);
	}
}

/*
The following were used for debugging FLTK bugs to and reporting them on www.fltk.org

class SpecialWindow : public Fl_Double_Window
{
public:
	Fl_Button *pB;
	Fl_Double_Window *pDWNew;

	SpecialWindow(unsigned int w, unsigned int h) : Fl_Double_Window(w, h)
	{
		pB = new Fl_Button(50,50,200,200);
		pB->callback(ButtonCB, this);
		end();
		callback(CloseCB, this);
		show();
	}
	int handle(int Event)
	{
		int RetVal = Fl_Double_Window::handle(Event);
		switch(Event)
		{
		case FL_FOCUS:
			//*NOTE: Some bug/feature in FLTK causes the Status widget to always get
			//the focus instead of the text editor
			pB->take_focus();
		}
		return RetVal;
	}
	static void CloseCB(Fl_Widget *pW, void *pV)
	{
		if(pW != pV)
			fl_alert("Close baadfood");
		//delete ((SpecialWindow *)pW)->pDWNew;
		((SpecialWindow *)pW)->hide();
		delete (SpecialWindow *)pW;
	}
	static void ButtonCB(Fl_Widget *pW, void *pV)
	{
		((SpecialWindow *)pV)->pDWNew = new Fl_Double_Window(200,200);
		((SpecialWindow *)pV)->pDWNew->end();
		((SpecialWindow *)pV)->pDWNew->callback(CloseNewCB, ((SpecialWindow *)pV)->pDWNew);
		((SpecialWindow *)pV)->pDWNew->show();
	}
	static void CloseNewCB(Fl_Widget *pW, void *pV)
	{
		if(pW != pV)
			fl_alert("CloseNew baadfood");
		((Fl_Double_Window *)pW)->hide();
		delete (Fl_Double_Window *)pW;
	}
};

	SpecialWindow *pDWOrig = new SpecialWindow(300, 300);
	return Fl::run();

class SpecialEditor : public Fl_Text_Editor
{
public:
	Fl_Window *pDlg;
	SpecialEditor(unsigned int x, unsigned int y, unsigned int w, unsigned int h) : Fl_Text_Editor(x, y, w, h)
	{

		pDlg = new Fl_Window(200, 200);
			Fl_Button *pB = new Fl_Button(50,50,100,50);
			pB->callback(ButtonCB, this);
		pDlg->end();
		pDlg->callback(CloseDlgCB, this);
		pDlg->set_non_modal();
		end();
	}
	void ShowDlg()
	{
		pDlg->show();
	}
	static void ButtonCB(Fl_Widget *pW, void *pV)
	{
		fl_alert("");
	}
	static void CloseDlgCB(Fl_Widget *pW, void *pV)
	{
		((SpecialEditor *)pV)->pDlg->hide();
		((SpecialEditor *)pV)->parent()->show();
	}
};

class SpecialWindow : public Fl_Double_Window
{
public:
	Fl_Button *pB;
	SpecialEditor *pTE;

	SpecialWindow(unsigned int w, unsigned int h) : Fl_Double_Window(w, h)
	{
		pB = new Fl_Button(50,50,200,50);
		pB->callback(ButtonCB, this);
		pTE = new SpecialEditor(50, 150, 200, 50);
		end();
		callback(CloseCB, this);
		show();
	}
	int handle(int Event)
	{
		int RetVal = Fl_Double_Window::handle(Event);
		switch(Event)
		{
		case FL_FOCUS:
			pB->take_focus();
		}
		return RetVal;
	}
	static void CloseCB(Fl_Widget *pW, void *pV)
	{
		//delete ((SpecialWindow *)pW)->pDWNew;
		((SpecialWindow *)pW)->hide();
		delete (SpecialWindow *)pW;
	}
	static void ButtonCB(Fl_Widget *pW, void *pV)
	{
		((SpecialWindow *)pV)->pTE->ShowDlg();
	}
};

*/
