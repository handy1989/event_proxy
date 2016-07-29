#include "rw_file.h"
#include "logger.h"

#include <sys/stat.h>

#include <boost/filesystem.hpp>

using std::string;

static const int32_t kMaxTextFileSize = 1024 * 1024 * 16;

RWFile::RWFile() : fd_(-1) {
}

RWFile::~RWFile() {
    f_close();
}

bool RWFile::f_open(const string& parent_dir, const string& file_name) {
    if (fd_ >= 0) {
        return true;
    }

    if (parent_dir.size() == 0 || file_name.size() == 0) {
        LOG_WARNING("invaild, parent_dir:" << parent_dir << " file_name:" << file_name);
        return false;
    }

    if (!boost::filesystem::exists(parent_dir)) {
        try {
            boost::filesystem::create_directories(parent_dir);
        } catch (boost::filesystem::filesystem_error& e) {
            LOG_ERROR("create_dir failed, strPath:" << parent_dir << " error:" << e.what());
            return false;
        }
    }

    path_ = parent_dir + "/" + file_name;

    return f_open();
}

bool RWFile::f_open(const string& full_file_path) {
    if (fd_ >= 0) {
        return true;
    }

    boost::filesystem::path path_tmp(full_file_path);
    string parent_path = path_tmp.parent_path().string();
    string file_name = path_tmp.filename().string();

    return f_open(parent_path, file_name);
}

bool RWFile::f_write(const void* buffer, uint32_t size, uint64_t offset) {
    uint32_t total = 0;
    if ((NULL == buffer) || (size == 0)) {
        LOG_WARNING(" invail,size:" << size << " offset:" << offset);
        return false;
    }

    uint32_t retry_times = MAX_DISK_TIMES;
    while ((total < size) && (retry_times-- >= 0)) {
        if (!check_file()) {
            return false;
        } else {
            int32_t length = ::pwrite64(fd_, ((char*)buffer + total), (size - total), (offset + total));
            if (length < 0) {
                LOG_WARNING("length < 0 ,errno:" << errno);
                if ((EINTR == errno) || (EAGAIN == errno)) {
                    continue;
                } else if (EBADF == errno) {
                    fd_ = -1;
                } else {
                    break;
                }
            } else if (0 == length) {
                LOG_WARNING("length == 0");
                break;
            } else {
                total += length;
            }
        }
    }

    return (total == size) ? true : false;
}

bool RWFile::f_read(string& buffer, uint32_t size, uint64_t offset) {
    uint32_t total = 0;
    if ((size == 0)) {
        LOG_WARNING(" invail,size:" << size << " offset:" << offset);
        return false;
    }

    char* read_buf = new char[size];

    uint32_t retry_times = MAX_DISK_TIMES;
    while ((total < size) && (retry_times-- >= 0)) {
        if (!check_file()) {
            delete [] read_buf;
            return false;
        } else {
            int32_t length = ::pread64(fd_, (read_buf + total), (size - total), (offset + total));
            if (length < 0) {
                LOG_WARNING("length < 0 ,errno:" << errno);
                if ((EINTR == errno) || (EAGAIN == errno)) {
                    continue;
                } else if (EBADF == errno) {
                    fd_ = -1;
                } else {
                    break;
                }
            } else if (0 == length) {
                LOG_WARNING("length == 0");
                break;
            } else {
                total += length;
            }
        }
    }

    bool ret = false;
    if (total == size) {
        buffer.assign(read_buf, size);
        ret = true;
    }

    delete [] read_buf;

    return ret;
}

bool RWFile::f_open() {
    if (fd_ >= 0) {
        f_close();
    }

    if (path_.empty()) {
        LOG_WARNING("path empty");
        return false;
    }

    fd_ = ::open(path_.c_str(), O_RDWR | O_CREAT, OPEN_MODE);
    if (fd_ < 0) {
        LOG_WARNING("open fail,path:" << path_);
        fd_ = -1;
        return false;
    }
    return true;
}

bool RWFile::f_close() {
    if (fd_ < 0) {
        return true;
    }

    int ret = ::close(fd_);

    fd_ = -1;

    if (0 != ret) {
        LOG_ERROR("disk fault");
        return false;
    }

    return true;

}

bool RWFile::check_file() {
    if ((fd_ < 0) && (!f_open())) {
        return false;
    }

    return true;
}

uint64_t RWFile::get_file_size() {
    if (!check_file()) {
        return 0;
    }

    struct stat statbuf;

    if (fstat(fd_, &statbuf) != 0) {
        return 0;
    }

    return statbuf.st_size;
}

bool RWFile::ReadFile(const string& parent_dir, const std::string& file_name, int64_t offset, uint32_t read_size, std::string& buf) {
    bool bret = false;

    do {
        RWFile rw;
        if (!rw.f_open(parent_dir.c_str(), file_name)) {
            break;
        }

        if (!rw.f_read(buf, read_size, offset)) {
            break;
        }

        if (buf.size() != read_size) {
            break;
        }

        bret = true;
    } while (0);

    return bret;
}

bool RWFile::WriteFile(const string& parent_dir, const std::string& file_name, int64_t offset, const std::string& buf) {
    bool bret = false;

    do {
        RWFile rw;
        if (!rw.f_open(parent_dir.c_str(), file_name)) {
            break;
        }

        if (!rw.f_write(buf.c_str(), buf.size(), offset)) {
            break;
        }

        bret = true;
    } while (0);

    return bret;
}

bool RWFile::GetFileContent(const std::string& full_file_path, std::string& buf) {
    bool bret = false;
    do {
        RWFile rw;
        if (!rw.f_open(full_file_path)) {
            break;
        }

        uint64_t file_size = rw.get_file_size();
        if (file_size > kMaxTextFileSize) {
            LOG_WARNING("file exceed max text file size, file_size:" << file_size);
            break;
        }

        if (!rw.f_read(buf, file_size)) {
            break;
        }

        bret = true;
    } while (0);
    
    return bret;
}
