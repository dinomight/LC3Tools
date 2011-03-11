//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "Architecture.h"

using namespace std;
using namespace JMT;

namespace Simulator	{

Architecture::Architecture()
{
	KeyboardInterruptVector = 0;
}

bool Architecture::Run()
{
	for(PipelineVector::iterator PipeIter = Pipelines.begin(); PipeIter != Pipelines.end(); PipeIter++)
		if(!PipeIter->Run(this))
			return false;
	return true;
}

Architecture::~Architecture()
{
}

}	//namespace Simulator
