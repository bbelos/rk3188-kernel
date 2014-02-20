#ifndef __NMI_LOG_H__
#define __NMI_LOG_H__

/* Errors will always get printed */
#define NMI_ERROR(...) do {  NMI_PRINTF("(ERR)(%s:%d) ",__NMI_FUNCTION__,__NMI_LINE__);\
	NMI_PRINTF(__VA_ARGS__); } while(0)

/* Wraning only printed if verbosity is 1 or more */	
#if (NMI_LOG_VERBOSITY_LEVEL > 0)
#define NMI_WARN(...) do { NMI_PRINTF("(WRN)");\
	NMI_PRINTF(__VA_ARGS__);  } while(0)
#else
#define NMI_WARN(...) (0)
#endif

/* Info only printed if verbosity is 2 or more */
#if (NMI_LOG_VERBOSITY_LEVEL > 1)
#define NMI_INFO(...) do {  NMI_PRINTF("(INF)");\
	NMI_PRINTF(__VA_ARGS__);  } while(0)
#else
#define NMI_INFO(...) (0)
#endif

/* Debug is only printed if verbosity is 3 or more */
#if (NMI_LOG_VERBOSITY_LEVEL > 2)
#define NMI_DBG(...) do { NMI_PRINTF("(DBG)(%s:%d) ",__NMI_FUNCTION__,__NMI_LINE__);\
	NMI_PRINTF(__VA_ARGS__);  } while(0)

#else
#define NMI_DBG(...) (0)
#endif

/* Function In/Out is only printed if verbosity is 4 or more */
#if (NMI_LOG_VERBOSITY_LEVEL > 3)
#define NMI_FN_IN do { NMI_PRINTF("(FIN) (%s:%d) \n", __NMI_FUNCTION__, __NMI_LINE__);  } while(0)
#define NMI_FN_OUT(ret) do { NMI_PRINTF("(FOUT) (%s:%d) %d.\n",__NMI_FUNCTION__,__NMI_LINE__,(ret));  } while(0)
#else
#define NMI_FN_IN (0)
#define NMI_FN_OUT(ret) (0)
#endif


#endif