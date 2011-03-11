//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#include "Pipeline.h"

using namespace std;
using namespace JMT;

namespace Simulator	{

PipelineStage::PipelineStage(const string &sname, bool (*Run)(Architecture *))
{
	sName = sname;
	this->Run = Run;
}


Pipeline::Pipeline(const string &sname)
{
	sName = sname;
}

bool Pipeline::AddStage(const string &sStageName, bool (*Run)(Architecture *))
{
	for(PipelineStageVector::iterator PipeIter = PipelineStages.begin(); PipeIter != PipelineStages.end(); PipeIter++)
	{
		if(PipeIter->sName == sStageName)
			throw "Pipeline duplicate stage name!";
	}

	PipelineStages.push_back(PipelineStage(sStageName, Run));
	return true;
}

bool Pipeline::Run(Architecture *pArch)
{
	for(PipelineStageVector::iterator PipeIter = PipelineStages.begin(); PipeIter != PipelineStages.end(); PipeIter++)
		if(!PipeIter->Run(pArch))
			return false;
	return true;
}

}	//namespace Simulator
