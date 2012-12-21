/* include/linux/lge/lge_mhl.h
*
* This header is for LGE specific input
*/

#ifndef __LGE_MHL_H__

#define __LGE_MHL_H__
#include <linux/string.h>
#include <linux/types.h>

#undef DBG
static inline const char *xxxx(const char *path)
{
		const char *tail = strrchr(path, '/');
			return tail ? tail+1 : path;
}

#if !defined (HDCP_DEBUG) && !defined (MHL_DEBUG)
  #define DBG(format, ...)
  #define DBG_E(format, ...)
  #define MHL_DBG_S(...)         ((void)0)
  #define MHL_DBG_E(...)         ((void)0)
  #define HDCP_DBG_S(...)         ((void)0)
  #define HDCP_DBG_E(...)         ((void)0)
#else
  #define DBG(format, ...) do { \
  		printk(KERN_DEBUG format "\n", ## __VA_ARGS__); \
  } while (0)
  #define DBG_E(format, ...) do { \
  		printk(KERN_DEBUG format "\n", ## __VA_ARGS__); \
  } while (0)
  #if defined (MHL_DEBUG)
    #define MHL_DBG_S() do { \
    		printk("MHL : +++ %s:%d [%s()] +++\n", xxxx(__FILE__), __LINE__, __func__); \
    } while (0)
    #define MHL_DBG_E() do { \
    		printk("MHL : --- %s:%d [%s()] ---\n", xxxx(__FILE__), __LINE__, __func__); \
    } while (0)
  #else
    #define MHL_DBG_S(...)		   ((void)0)
    #define MHL_DBG_E(...)		   ((void)0)  
  #endif  
  #if defined (HDCP_DEBUG)
    #define HDCP_DBG_S() do { \
    		printk("HDCP : +++ %s:%d [%s()] +++\n", xxxx(__FILE__), __LINE__, __func__); \
    } while (0)
    #define HDCP_DBG_E() do { \
    		printk("HDCP : --- %s:%d [%s()] ---\n", xxxx(__FILE__), __LINE__, __func__); \
    } while (0)
  #else
    #define HDCP_DBG_S(...) 		((void)0)
    #define HDCP_DBG_E(...) 		((void)0)  
  #endif
#endif

#endif
