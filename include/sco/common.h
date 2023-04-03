#pragma once

#ifdef SCO_COMPILED_LIB
# ifdef SCO_SHARED_LIB
#   ifdef _WIN32
#     ifdef SCO_EXPORTS
#       define SCO_API __declspec(dllexport)
#     else // !SCO_EXPORTS
#       define SCO_API __declspec(dllimport)
#     endif
#   else // !defined(_WIN32)
#     define SCO_API __attribute__((visibility("default")))
#   endif
# else // !defined(SCO_SHARED_LIB)
#   define SCO_API
# endif
# define SCO_INLINE
# undef SCO_HEADER_ONLY
#else // !defined(SCO_COMPILED_LIB)
# define SCO_API
# define SCO_INLINE inline
# define SCO_HEADER_ONLY
#endif // #ifdef SCO_COMPILED_LIB

#if __has_include(<coroutine>)
# include <coroutine>
# define COSTD std
#elif __has_include(<experimental/coroutine>)
# include <experimental/coroutine>
# define COSTD std::experimental
#else
  warning <coroutine> not found
#endif
