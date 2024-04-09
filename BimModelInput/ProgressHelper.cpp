


#include "stdafx.h"
#include "BimModelInput.h"
//#include <TaskReporter.h>


namespace XBSJ {

	ProgressHelper::ProgressHelper(string name, double step, double percent)
	{

#ifdef XBSJ_TASK
		if (percent > 0) {
			//TS_BEGINSTAGE(name, step, percent);
		}
		else {
			//TS_BEGINSTAGE2(name, step);
		}
#endif

		LOG(WARNING) << "name:" << name << " step:" << step << " percent:" << percent;
	}
 
	ProgressHelper::~ProgressHelper()
	{
#ifdef XBSJ_TASK
		//TS_ENDSTAGE
#endif
	}

	void ProgressHelper::skip(double step) {
#ifdef XBSJ_TASK
		//TS_SKIP(step);
#endif
	}

	void ProgressHelper::init(string taskserver, string taskname) {
#ifdef XBSJ_TASK
	//	LOG(INFO) << "task erver:" << taskserver << ",taskname:" << taskname;
		//TS_INIT(taskserver, taskname);
#endif	
	}

	void ProgressHelper::finish() {
#ifdef XBSJ_TASK
		//TS_FINISHED;
#endif
	}

}
