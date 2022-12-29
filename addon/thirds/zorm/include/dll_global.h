#ifdef __MSVC__ 
	#ifdef ZORM_LIB
		#define ZORM_API __declspec(dllexport) 
	#else
		#define ZORM_API __declspec(dllimport) 
	#endif
#else
	#ifdef __LINUX__
		#ifdef ZORM_LIB
			#define ZORM_API __attribute__ ((visibility ("default"))) 
		#else
			#define ZORM_API
		#endif
	#else
        #define ZORM_API
	#endif
#endif