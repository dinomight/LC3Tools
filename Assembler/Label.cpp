//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "Label.h"
#include <cstdio>

using namespace std;
using namespace JMT;

namespace Assembler	{

Label::Label(const LocationVector &LocationStack, const string &sL, Segment *pseg) : Element(LocationStack)
{
	ElementType = LabelElement;
	sLabel = sL;
	if(!pseg)
		throw "NULL Segment given to Label!";
	pSeg = pseg;
	pSegment = NULL;
	pElement = NULL;
	Size = 0;
	Length = 1;
}

Element *Label::Copy() const
{
	return new Label(*this);
}

void Label::AssignValues(vector<Number *>::iterator &StartIter, const vector<Number *>::iterator &EndIter)
{
}

bool Label::GetImage(RamVector &vRam, bool fLittleEndian, CallBackFunction) const
{
	vRam.clear();
	return true;
}

Label::operator const char *() const
{
	string::size_type Period = sLabel.find('.');
	if(Period == string::npos)
		sprintf(sElement, "%.63s:", sLabel.substr(Period+1).c_str());
	else
		sprintf(sElement, "%.63s:", sLabel.substr(Period+1).c_str());
	return sElement;
}

}	//namespace Assembler
