#include<fileWatcherandpub.hpp>
int main(){
    using namespace EpollInternals;
    using namespace FileWatcherPubSub;
    Watcher newwth(Watchfs::init::nonBLockMode);
    std::string path="/home/abhay/project/multifileeventlistener/demonstration/hello.txt";
    File fs(path, ios::readWrite, S_IRWXU);
    newwth.addFile(std::move(fs), Watchfs::file::accessForRead |
      Watchfs::file::modified |
      Watchfs::file::fileDeleted |
      Watchfs::file::closed | Watchfs::file::opende);
    while(true)newwth.eventListener(nullptr);

}