#ifndef SIMPLE_BLOCK_SDK_HPP
#define SIMPLE_BLOCK_SDK_HPP
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include<fdObject.hpp>
#include<util.hpp>
#include <cmdprocessor.hpp>
#include <thread> 
namespace PubSubSdk{

class BlSocketClient{
    int port;
    const std::string ip;
    sockaddr_in serverAddress;
    FDEPOLLRL::FileDesc fd;
    public:
        BlSocketClient(std::string &&ip,int port):port(port),ip(std::move(ip)){}
        BlSocketClient(BlSocketClient&)=delete;
        BlSocketClient(BlSocketClient&&)=delete;
        BlSocketClient& operator=(BlSocketClient&)=delete;
        BlSocketClient& operator=(BlSocketClient&&)=delete;
        void connect(){
            fd=socket(AF_INET, SOCK_STREAM, 0);
            if (fd.get() < 0) {
                switch (errno) {
                    case EAFNOSUPPORT:
                        throw std::runtime_error("The implementation does not support the specified address family: " + std::string(std::to_string(errno)));
                    case EPROTONOSUPPORT:
                        throw std::runtime_error("The protocol type or the specified protocol is not supported within this domain: " + std::string(std::to_string(errno)));
                    case ENFILE:
                        throw std::runtime_error("The system-wide limit on the total number of open files has been reached: " + std::string(std::to_string(errno)));
                    case EMFILE:
                        throw std::runtime_error("The per-process limit on the number of open file descriptors has been reached: " + std::string(std::to_string(errno)));
                    case EACCES:
                        throw std::runtime_error("Permission to create a socket of the specified type and/or protocol is denied: " + std::string(std::to_string(errno)));
                    case ENOBUFS:
                    case ENOMEM:
                        throw std::runtime_error("Insufficient memory or buffer space is available to create the socket: " + std::string(std::to_string(errno)));
                    case EINVAL:
                        throw std::runtime_error("Unknown protocol, or protocol family not available: " + std::string(std::to_string(errno)));
                    default:
                        throw std::runtime_error("Fatal socket creation error: " + std::string(std::to_string(errno)));
                }
            }
            serverAddress.sin_family = AF_INET;
            serverAddress.sin_port = htons(port);
            if(inet_pton(AF_INET, ip.data(), &serverAddress.sin_addr) <= 0){
                switch (errno) {
                    case EAFNOSUPPORT:
                        throw std::runtime_error("The specified address family is not supported: " + std::string(std::to_string(errno)));
                    default:
                        throw std::runtime_error("Invalid IP address string format or network error: " + std::string(std::to_string(errno)));
                }
            }
            if (::connect(fd.get(), (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
                switch (errno) {
                    case ECONNREFUSED:
                        throw std::runtime_error("Connection refused by the remote host: " + std::string(std::to_string(errno)));
                    case ETIMEDOUT:
                        throw std::runtime_error("Connection timed out before establishing a link: " + std::string(std::to_string(errno)));
                    case ENETUNREACH:
                        throw std::runtime_error("The network is currently unreachable from this host: " + std::string(std::to_string(errno)));
                    case EADDRNOTAVAIL:
                        throw std::runtime_error("The requested remote address is not available or valid: " + std::string(std::to_string(errno)));
                    case EINPROGRESS:
                    case EALREADY:
                        throw std::runtime_error("The socket is non-blocking and a connection attempt is already underway: " + std::string(std::to_string(errno)));
                    case EISCONN:
                        throw std::runtime_error("The socket is already connected: " + std::string(std::to_string(errno)));
                    case EBADF:
                    case ENOTSOCK:
                        throw std::runtime_error("The file descriptor is invalid or does not refer to a socket: " + std::string(std::to_string(errno)));
                    case EINTR:
                        throw std::runtime_error("The connection attempt was interrupted by a signal: " + std::string(std::to_string(errno)));
                    default:
                        throw std::runtime_error("Fatal connection error: " + std::string(std::to_string(errno)));
                }

            }
            

        }
        void write(std::string& cmd){
            std::string_view cmdPart(cmd);
            int pos=0;
            ssize_t bytesWrite=0;
            std::string_view contentView(cmd);
            while(pos+bytesWrite<cmd.size()  ){
                bytesWrite=::write(fd.get(), contentView.substr(pos).data(), contentView.substr(pos).size());
                if(bytesWrite==-1 && (errno == EINTR))continue;
                if(bytesWrite==-1 && (errno == EPIPE || errno == ECONNRESET)){
                    Logging::errorDef("server disconnect unexpectedly");
                    throw std::runtime_error("server disconnected ");
                }
                pos+=bytesWrite;
            }
            
        }
        std::string  readUntilSize(){
            std::string res;
            std::string buffer(4096,0);
            int remaining=4;
            while(remaining ){
                int bytesRead=::read(fd.get(), buffer.data(),remaining);
                if(bytesRead==0){
                    Logging::infoDef("server disconnect");
                    throw std::runtime_error("server disconnected ");
                }
                if(bytesRead==-1 && (errno == EINTR))continue;
                if(bytesRead==-1 && (errno == EPIPE || errno == ECONNRESET)){
                    Logging::errorDef("server disconnect unexpectedly");
                    throw std::runtime_error("server disconnected ");
                }
                remaining-=bytesRead;
            };
            int totalSize=CommandConstruct::getSize(std::string_view(buffer).substr(0,4));
            remaining=totalSize;
            while(remaining){
                int bytesRead=::read(fd.get(), buffer.data(),std::min(remaining,4096));
                if(bytesRead==0)throw std::runtime_error("server disconnected ");
                if(bytesRead==-1 && (errno == EINTR))continue;
                if(bytesRead==-1 && (errno == EPIPE || errno == ECONNRESET))throw std::runtime_error("server disconnected");
                remaining-=bytesRead;
                res+=buffer.substr(0,bytesRead);
            }
            return res;
        }

};
template<typename T>
concept AppPubSubIFReq=requires (T obj) {
    {obj.connect()}->std::same_as<Status>;
    
};
template<typename T>
class AppPubSubIF{
   protected:
        BlSocketClient cl;
        AppPubSubIF(std::string &&ip="127.0.0.1",int port=3000):cl(std::move(ip),port){
            static_assert(AppPubSubIFReq<T>,"does not implement connect" );
        };
        AppPubSubIF(AppPubSubIF&)=delete;
        AppPubSubIF(AppPubSubIF&&)=delete;
        AppPubSubIF& operator=(AppPubSubIF&)=delete;
        AppPubSubIF& operator=(AppPubSubIF&&)=delete;
    public:
        Status checkStatus(){
            auto resp=ResponseProcessor::getCmd(cl.readUntilSize());

            if(!resp.has_value()){
                Logging::infoDef("unexpected server response");
                return Status(500);
            }
            if(Status* st=dynamic_cast<Status*>(resp->get())){
                return st->status;
            }
            Logging::infoDef("unexpected server response");
            return Status(500);
            
        }
};
class AppSubsCriber:public AppPubSubIF<AppSubsCriber>{
    
    std::thread BgThread;
    std::atomic_flag isBgSt=ATOMIC_FLAG_INIT;
    phmap::parallel_flat_hash_map_m<std::string,std::shared_ptr<std::move_only_function<void(std::string)>>> threadSfMap;
    void readLoop(){
        while(true){
            std::string res=cl.readUntilSize();
            auto tklExp=TokenList::getTkList(res);
            if(!tklExp.has_value())continue;
            auto tklist=tklExp.value();
            auto respExp=ResponseProcessor::getCmd(tklist);
            if(!respExp.has_value())continue;
            auto resp=std::move(respExp.value());
            
            if(Status* status=dynamic_cast<Status*>(resp.get())){
                std::cout << "getting response status:"<<status->status<<"\n";
                
            }else if(ListTpc* list=dynamic_cast<ListTpc*>(resp.get())){
                std::cout << "getting response status:"<<status->status<< "\n";
            }else if(TopicMsgCmd* msgCmd=dynamic_cast<TopicMsgCmd*>(resp.get())){
                std::shared_ptr<std::move_only_function<void(std::string)>> cb=nullptr;
                threadSfMap.if_contains(msgCmd->topic, [&cb](const auto& pair){
                    cb=pair.second;
                });
                if(cb){
                    (*cb)(msgCmd->msg);
                }
            }
        }
    }
    public:
        AppSubsCriber(std::string &&ip="127.0.0.1",int port=3000):AppPubSubIF(std::move(ip),port){}
        Status connect(){
            cl.connect();
            auto cmd=CommandConstruct::initSub();
            cl.write(cmd);
            return checkStatus();
        }
        void addTopic(std::string_view st,std::move_only_function<void(std::string)> &&task){
            auto sharedPtr=std::make_shared<std::move_only_function<void(std::string)>>(std::move(task));
            threadSfMap.insert_or_assign(st,std::move(sharedPtr));
            std::string cmd=CommandConstruct::subTopic(st);
            cl.write(cmd);
        }
        void removeTopic(std::string_view st){
            threadSfMap.erase(st);
            std::string cmd=CommandConstruct::unSubTopic(st);
            cl.write(cmd);
        }
        void runBgThread(){
            if(isBgSt.test_and_set()){
                Logging::infoDef("try to run again pub thread");
            }
            Logging::infoDef("thread started successfully");
            BgThread=std::thread(&AppSubsCriber::readLoop,this);
        }
        ~AppSubsCriber(){
            if(BgThread.joinable()){
                BgThread.join();
            }
        }

};
class AppPublisher:public AppPubSubIF<AppPublisher>{
    public:
        AppPublisher(std::string &&ip="127.0.0.1",int port=3000):AppPubSubIF(std::move(ip),port){}
        Status connect(){
            cl.connect();
            auto cmd=CommandConstruct::initPub();
            cl.write(cmd);
            return checkStatus();
        }
        void publish(const  std::string &topic,const std::string &msg){
            std::string cmd=CommandConstruct::publishMsg(topic,msg);
            cl.write(cmd);
        }
        void addTpc(const std::string &topic){
            std::string cmd=CommandConstruct::addTopic(topic);
            cl.write(cmd);
        };
        void remTpc(const std::string &topic){
            std::string cmd=CommandConstruct::removeTopic(topic);
            cl.write(cmd);
        };
        

};
}
#endif
