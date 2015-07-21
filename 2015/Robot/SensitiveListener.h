#ifndef SENSITIVE_LISTENER_H_
#define SENSITIVE_LISTENER_H_

#include <functional>

class SensitiveListener
{
private:
	std::function<bool (void)> Callback;
	int Sensitivity;
	int Counter;

public:
	SensitiveListener(std::function<bool (void)> callback, int sensitivity);
	~SensitiveListener(void);
	bool GetValue(void);
};

#endif
