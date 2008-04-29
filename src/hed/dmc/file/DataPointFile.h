#ifndef __ARC_DATAPOINTFILE_H__
#define __ARC_DATAPOINTFILE_H__

#include <list>

#include <arc/data/DataPointDirect.h>
#include <arc/Thread.h>

namespace Arc {

  class DataPointFile : public DataPointDirect {
   public:
    DataPointFile(const URL& url);
    virtual ~DataPointFile();
    virtual DataStatus StartReading(DataBufferPar& buffer);
    virtual DataStatus StartWriting(DataBufferPar& buffer,
                                    DataCallback *space_cb = NULL);
    virtual DataStatus StopReading();
    virtual DataStatus StopWriting();
    virtual DataStatus Check();
    virtual DataStatus Remove();
    virtual DataStatus ListFiles(std::list<FileInfo>& files,
                                 bool resolve = true);
    virtual bool WriteOutOfOrder();
   private:
    SimpleCondition transfer_cond;
    void read_file();
    void write_file();
    bool reading;
    bool writing;
    int fd;
    bool is_channel;
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_DATAPOINTFILE_H__
