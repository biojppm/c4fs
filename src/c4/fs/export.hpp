#ifndef C4_FS_EXPORT_HPP_
#define C4_FS_EXPORT_HPP_

#ifdef _WIN32
    #ifdef C4FS_SHARED
        #ifdef C4FS_EXPORTS
            #define C4FS_EXPORT __declspec(dllexport)
        #else
            #define C4FS_EXPORT __declspec(dllimport)
        #endif
    #else
        #define C4FS_EXPORT
    #endif
#else
    #define C4FS_EXPORT
#endif

#endif /* C4_FS_EXPORT_HPP_ */
