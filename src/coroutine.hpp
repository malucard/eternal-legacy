#include <functional>

#define coroutine(...) [&]() {void* _co_state = nullptr;\
	return [=](__VA_ARGS__) mutable { if(_co_state) goto *_co_state;
#define coend _co_state = nullptr;};}()
#define coret _co_state = nullptr; return
#define _yield__(l) _co_state = && _co_l ## l; return; _co_l ## l:
#define _yield_(l) _yield__(l)
#define yield _yield_(__LINE__)
