#include <istat/test.h>
#include <istat/istattime.h>
#include <istat/Mmap.h>
#include "../daemon/StatCounterFactory.h"
#include "../daemon/StatStore.h"
#include <boost/filesystem.hpp>

using namespace istat;

RetentionPolicy rp("10s:1d");
RetentionPolicy xrp("");

class FakeProtectedDiskMmap : public Mmap
{
public:
    FakeProtectedDiskMmap(int64_t freeSpace)
        : freeSpace_(freeSpace)
    {
    }

    virtual int open(char const *name, int flags)
    {
        return -1;
    }

    virtual int close(int fd)
    {
        return -1;
    }

    virtual ssize_t read(int fd, void *ptr, ssize_t amt)
    {
        return -1;
    }

    virtual ssize_t write(int fd, void *ptr, ssize_t amt)
    {
        return -1;
    }

    virtual ptrdiff_t seek(int fd, ptrdiff_t offset, int whence)
    {
        return -1;
    }

    virtual ssize_t tell(int fd)
    {
        return -1;
    }

    virtual int truncate(int fd, ssize_t size)
    {
        return -1;
    }

    virtual void *map(int fd, int64_t offset, size_t size, bool writable)
    {
        return 0;
    }

    virtual bool unmap(void const *ptr, size_t size)
    {
        return false;
    }

    virtual bool flush(void const *ptr, size_t size, bool immediate)
    {
        return false;
    }

    virtual int64_t availableSpace(char const *path)
    {
        return freeSpace_;
    }

    virtual void dispose()
    {
    }

    virtual void counters(int64_t *oMaps, int64_t *oUnmaps, int64_t *oOpens, int64_t *oCloses)
    {
    }
private:
    int64_t freeSpace_;
};

void run_tests(void)
{
    Mmap *mm;
    
    mm = NewMmap();
    {
        boost::asio::io_service service;

        std::string storepath("/tmp/test/statstore");
        boost::filesystem::remove_all(storepath);
        boost::filesystem::create_directories(storepath);
        boost::shared_ptr<IStatCounterFactory> statCounterFactory(new StatCounterFactory(storepath, mm, rp));
        StatStore store(storepath, getuid(), service, statCounterFactory, mm);

        store.record("taco", 42.42);
        std::list<std::pair<std::string, bool> > oList;
        store.listMatchingCounters("bbq", oList);
        assert_equal(0, oList.size());
        store.listMatchingCounters("taco is delicious!", oList);
        assert_equal(0, oList.size());
        store.listMatchingCounters("taco", oList);
        assert_equal(1, oList.size());
    }
    mm->dispose();

    // Ensure full disk does not have available space.
    mm = new FakeProtectedDiskMmap(0);
    {
        boost::asio::io_service service;
        std::string storepath("/tmp/test/statstore");
        boost::filesystem::remove_all(storepath);
        boost::filesystem::create_directories(storepath);
        boost::shared_ptr<IStatCounterFactory> statCounterFactory(new StatCounterFactory(storepath, mm, rp));
        StatStore store(storepath, getuid(), service, statCounterFactory, mm);

        assert_equal(store.hasAvailableSpace(), false);
    }
    mm->dispose();

    // Ensure disk with 1GB free still has available space.
    mm = new FakeProtectedDiskMmap(1024L * 1024L * 1024L);
    {
        boost::asio::io_service service;
        std::string storepath("/tmp/test/statstore");
        boost::filesystem::remove_all(storepath);
        boost::filesystem::create_directories(storepath);
        boost::shared_ptr<IStatCounterFactory> statCounterFactory(new StatCounterFactory(storepath, mm, rp));
        StatStore store(storepath, getuid(), service, statCounterFactory, mm);

        assert_equal(store.hasAvailableSpace(), true);
    }
    mm->dispose();
}

int main(int argc, char const *argv[])
{
    return istat::test(run_tests, argc, argv);
}
