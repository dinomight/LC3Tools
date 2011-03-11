//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef LABEL_H
#define LABEL_H

#pragma warning (disable:4786)
#include <string>
#include "Segment.h"
#include "Element.h"
#include "Number.h"
#include "Base.h"

using namespace std;
using namespace JMT;

namespace Assembler
{
	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		Label

		An instance of this class is a symbolic representation of a label
		element in an assembly program.
		
		In addition to construction of the Data, you must also set the address
		and line number (see Element).
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class Label : public Element
	{
	public:
		//The segment this label is in
		Segment *pSeg;
		//What this label is pointing to (either a segment or an element)
		Segment *pSegment;
		Element *pElement;
		//The Symbol that points to this label
		string sLabel;

		/**********************************************************************\
			Label( [in] location stack, [in] label name,
				[in] the segment this label is in )
		\******/
		Label(const LocationVector &, const string &, Segment *);

		virtual Element *Copy() const;
		virtual void AssignValues(vector<Number *>::iterator &, const vector<Number *>::iterator &);
		virtual bool GetImage(RamVector &, bool, CallBackFunction) const;

		/**********************************************************************\
			Prints the label
		\******/
		virtual operator const char *() const;
	};
}

#endif
