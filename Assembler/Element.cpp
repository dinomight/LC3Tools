//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "Element.h"

using namespace std;
using namespace JMT;

namespace Assembler	{

char Element::sMessageBuffer[130 + 2*MAX_IDENTIFIER_CHAR];
char Element::sElement[129 + MAX_IDENTIFIER_CHAR];


Element::Element(const LocationVector &LS)
{
	LocationStack = LS;
}

Element::~Element()
{
}

}	//namespace Assembler
