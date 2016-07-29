#ifndef CONFIG_H_
#define CONFIG_H_

#include "singleton.h"

#include <stdint.h>

class Config
{
public:
    std::string cache_dir;
    int32_t level1;
    int32_t level2;
};

typedef Singleton<Config> SingletonConfig;

#endif // CONFIG_H_
