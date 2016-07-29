#ifndef RWFILE_H_
#define RWFILE_H_

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>

class RWFile {
  public:
    RWFile();
    ~RWFile();

    bool f_open(const std::string& parent_dir, const std::string& file_name);
    bool f_open(const std::string& full_file_path);
    bool f_close();

    bool f_write(const void* buffer, uint32_t size, uint64_t offset = 0);
    bool f_read(std::string& buffer, uint32_t size, uint64_t offset = 0);
    uint64_t get_file_size();

    static bool ReadFile(const std::string& parent_dir, const std::string& file_name, int64_t offset, uint32_t read_size, std::string& buf);
    static bool WriteFile(const std::string& parent_dir, const std::string& file_name, int64_t offset, const std::string& buf);

    static bool GetFileContent(const std::string& full_file_path, std::string& buf);
  private:

    static const int MAX_DISK_TIMES = 5;
    static const mode_t OPEN_MODE = 0644;

    bool f_open();

    bool check_file();

    std::string path_;

    int fd_;
};

#endif // RWFILE_H_
