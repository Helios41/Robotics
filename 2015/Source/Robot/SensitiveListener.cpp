#include "SensitiveListener.h"

SensitiveListener::SensitiveListener(std::function<bool (void)> callback, int sensitivity) :
	Callback(callback),
	Sensitivity(sensitivity),
	Counter(sensitivity)
{

}

SensitiveListener::~SensitiveListener(void) { }

bool SensitiveListener::GetValue(void)
{
	if((this->Counter >= this->Sensitivity) && this->Callback())
	{
		this->Counter = 0;
		return true;
	}

	if(this->Counter < this->Sensitivity)
	{
		this->Counter++;
	}

	return false;
}
