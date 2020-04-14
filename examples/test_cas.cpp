#include <string>
#include <sstream>
#include <iterator>
#include <iostream>
#include <unistd.h>

#if defined(__CLANG_ATOMICS)
#define atomic_xchange(object, object_value, value) \
    __sync_swap(object, value)
#elif defined(__GNUC_ATOMICS)
#define atomic_xchange(object, object_value, value) \
    __sync_bool_compare_and_swap( object, object_value, value)
#elif __has_builtin(__sync_swap)
/* Clang provides a full-barrier atomic exchange - use it if available. */
#define atomic_xchange(object, object_value, value) \
    __sync_swap(object, value)
#endif

static inline bool compare_and_swap(volatile int * cond, int old_, int new_) {
        if( (*cond) != old_ ) { return false; }
        (*cond) = new_;
        return true;
}

template <typename Type1, typename Type2>
static inline Type1 ipow(Type1 a, Type2 ex)
{
        // Return a**ex
        //
        if ( 0==ex )  return 1;
        else
        {
            Type1 z = a;
            Type1 y = 1;
            while ( 1 )
            {
                if ( ex & 1 )  y *= z;
                ex /= 2;
                if ( 0==ex )  break;
                z *= z;
            }
            return y;
        }
}

static inline int backoff(const int attempt, const int base, const int cap) {
        return std::min(cap, ipow(base * 2, 2));
}

int main() {
    std::int32_t a = 0, b = 0;
//std::cout << ( atomic_xchange(&a, 0, 1) && atomic_xchange(&b, 0, 1) ) << std::endl;
    //std::cout << b << std::endl;
    //std::cout << !atomic_xchange(&a, 0, 1) << std::endl;
    //std::cout << !atomic_xchange(&b, 0, 1) << std::endl;
    //std::cout << b << std::endl;
std::cout << a << ' ' << b << std::endl;
//std::cout << atomic_xchange(&b, 1, 0) << std::endl;
//std::cout << atomic_xchange(&a, 0, 1) << std::endl;
//std::cout << atomic_xchange(&a, 0, 1) << std::endl;
    while(!atomic_xchange(&a, 0, 1)) {
    }

std::cout << "1st\t" << a << ' ' << b << std::endl;

    while(!atomic_xchange(&b, 1, 0)) {
    }

std::cout << "2nd\t" << a << ' ' << b << std::endl;

}
