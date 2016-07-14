#ifndef _COMMON_LOGGER_H_
#define _COMMON_LOGGER_H_

#include <stdio.h>
#include <stdint.h>

#ifdef USE_LOG4CPP_MACROS

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
// to do

#else

#include <glog/logging.h>

#if defined(LOGGER_DEBUG_LEVEL)
#define LOG_DEBUG(a)   LOG(INFO) << "DEBUG:" << a
#define LOG_INFO(a)    LOG(INFO) << a
#define LOG_WARNING(a) LOG(WARNING) << a
#define LOG_ERROR(a)   LOG(ERROR) << a
#define LOG_FATAL(a)   LOG(FATAL) << a

#elif defined(LOGGER_INFO_LEVEL)
#define LOG_DEBUG(a)
#define LOG_INFO(a)    LOG(INFO) << a
#define LOG_WARNING(a) LOG(WARNING) << a
#define LOG_ERROR(a)   LOG(ERROR) << a
#define LOG_FATAL(a)   LOG(FATAL) << a

#elif defined(LOGGER_WARNING_LEVEL)
#define LOG_DEBUG(a)
#define LOG_INFO(a)
#define LOG_WARNING(a) LOG(WARNING) << a
#define LOG_ERROR(a)   LOG(ERROR) << a
#define LOG_FATAL(a)   LOG(FATAL) << a

#elif defined(LOGGER_ERROR_LEVEL)
#define LOG_DEBUG(a)
#define LOG_INFO(a)
#define LOG_WARNING(a)
#define LOG_ERROR(a)   LOG(ERROR) << a
#define LOG_FATAL(a)   LOG(FATAL) << a

#elif defined(LOGGER_FATAL_LEVEL)
#define LOG_DEBUG(a)
#define LOG_INFO(a)
#define LOG_WARNING(a)
#define LOG_ERROR(a)
#define LOG_FATAL(a)   LOG(FATAL) << a

#elif defined(LOGGER_CLOSE)
#define LOG_DEBUG(a)
#define LOG_INFO(a)
#define LOG_WARNING(a)
#define LOG_ERROR(a)
#define LOG_FATAL(a)

#else
#define LOG_DEBUG(a)   LOG(INFO) << "DEBUG:" << a
#define LOG_INFO(a)    LOG(INFO) << a
#define LOG_WARNING(a) LOG(WARNING) << a
#define LOG_ERROR(a)   LOG(ERROR) << a
#define LOG_FATAL(a)   LOG(FATAL) << a

#endif // LOGGER_DEBUG_LEVEL

#endif // USE_LOG4CPP_MACROS

#endif // _COMMON_LOGGER_H_
