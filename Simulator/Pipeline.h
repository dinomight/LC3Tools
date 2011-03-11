//	Copyright 2003 Ashley Wise
//	University Of Illinois Urbana-Champaign
//	awise@crhc.uiuc.edu

#ifndef PIPELINE_H
#define PIPELINE_H

#pragma warning (disable:4786)
#include <vector>
#include <string>
#include "../Assembler/Base.h"

using namespace std;
using namespace JMT;

namespace Simulator
{
	class Architecture;
	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		MemoryLocation

		An instance of this class represents one location in a memory array.

	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class PipelineStage
	{
	public:
		//Name of the pipeline stage
		string sName;

		//Construct (name, run function)
		PipelineStage(const string &, bool (*Run)(Architecture *));
		//Function which executes the pipeline stage.
		//The parameter is a pointer to the architecture class
		bool (*Run)(Architecture *);
	};

	/*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*\
		Pipeline

		An instance of this class represents one pipeline.
	\*_,-=~~""``^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^``""~~==-,_*/
	class Pipeline
	{
	public:
		//Array of pipeline stages
		typedef vector<PipelineStage> PipelineStageVector;
		//List of pipeline stages
		PipelineStageVector PipelineStages;
		//Name of the pipeline
		string sName;

		//Construct (name, start byte index, end byte index, bit divisions)
		Pipeline(const string &);
		//Adds a stage to the pipeline. Stages must be added in order of propagation.
		//Stages are run in reverse order.
		bool AddStage(const string &, bool (*Run)(Architecture *));
		//Execute the pipeline
		//The parameter is a pointer to the architecture class
		bool Run(Architecture *);
	};
}

#endif
