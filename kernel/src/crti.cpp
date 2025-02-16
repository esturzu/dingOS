// Global Constructors from
// https://wiki.osdev.org/Calling_Global_Constructors (Public Domain)

typedef void (*func_ptr)(void);

extern func_ptr _init_array_start[0], _init_array_end[0];
extern func_ptr _fini_array_start[0], _fini_array_end[0];

namespace CRTI {

void _init(void) {
  for (func_ptr* func = _init_array_start; func != _init_array_end; func++)
    (*func)();
}

void _fini(void) {
  for (func_ptr* func = _fini_array_start; func != _fini_array_end; func++)
    (*func)();
}

};  // namespace CRTI

func_ptr _init_array_start[0] __attribute__((used, section(".init_array"),
                                             aligned(sizeof(func_ptr)))) = {};
func_ptr _fini_array_start[0] __attribute__((used, section(".fini_array"),
                                             aligned(sizeof(func_ptr)))) = {};