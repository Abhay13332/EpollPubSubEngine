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
                logging::print("unable to create socket");
                return;
            }
            serverAddress.sin_family = AF_INET;
            serverAddress.sin_port = htons(8080);
            if(inet_pton(AF_INET, ip.data(), &serverAddress.sin_addr) <= 0){
                logging::print("unknown address");
                return;
            }
            if (::connect(fd.get(), (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
                logging::print("Connection Failed!");
                return ;
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
                if(bytesWrite==-1 && (errno == EPIPE || errno == ECONNRESET))throw std::runtime_error("server disconnected");
                pos+=bytesWrite;
            }
        }
        std::string  readUntilSize(){
            std::string res;
            std::string buffer(4096,0);
            int remaining=4;
            while(remaining ){
                int bytesRead=::read(fd.get(), buffer.data(),remaining);
                if(bytesRead==-1 && (errno == EINTR))continue;
                if(bytesRead==-1 && (errno == EPIPE || errno == ECONNRESET))throw std::runtime_error("server disconnected");
                remaining-=bytesRead;
            };
            int totalSize=CommandConstruct::getSize(std::string_view(buffer).substr(0,4));
            remaining=totalSize;
            while(remaining){
                 int bytesRead=::read(fd.get(), buffer.data(),std::min(remaining,4096));
                if(bytesRead==-1 && (errno == EINTR))continue;
                if(bytesRead==-1 && (errno == EPIPE || errno == ECONNRESET))throw std::runtime_error("server disconnected");
                remaining-=bytesRead;
            }
            return res;
        }

};
class AppSubsCriber{
    BlSocketClient cl;
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
                logging::print("getting response status:",status->status);
                
            }else if(ListTpc* list=dynamic_cast<ListTpc*>(resp.get())){
                logging::print("topics",list->list);
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
        AppSubsCriber(std::string &&ip="127.0.0.1",int port=3000):cl(std::move(ip),port){}
        AppSubsCriber(AppSubsCriber&)=delete;
        AppSubsCriber(AppSubsCriber&&)=delete;
        AppSubsCriber& operator=(AppSubsCriber&)=delete;
        AppSubsCriber& operator=(AppSubsCriber&&)=delete;


        void connect(){
        cl.connect();
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
                logging::print("try to run again pub thread");
            }
            BgThread=std::thread(&AppSubsCriber::readLoop,this);
        }
        ~AppSubsCriber(){
            if(BgThread.joinable()){
                BgThread.join();
            }
        }

};
class AppPublisher{
    BlSocketClient cl;
    AppPublisher(std::string &&ip="127.0.0.1",int port=3000):cl(std::move(ip),port){}
    AppPublisher(AppPublisher&)=delete;
    AppPublisher(AppPublisher&&)=delete;
    AppPublisher& operator=(AppPublisher&)=delete;
    AppPublisher& operator=(AppPublisher&&)=delete;
    void connect(){
        cl.connect();
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
