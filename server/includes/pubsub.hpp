#ifndef PUB_SUB_HPP
#define PUB_SUB_HPP
#include <socket.hpp>
#include <parallel_hashmap/phmap.h>
#include <epoll.hpp>
namespace PubSubEngine{
    
    class PubSubMan;
    class Skcl;
    class PubsubIF{
      protected:
        PubSubMan* pubSubMgr=nullptr; 
        Skcl* skcl=nullptr; 
        
      public:
          PubsubIF(Skcl* skcl,PubSubMan* pubSubMgr):skcl(skcl),pubSubMgr(pubSubMgr){
            if(skcl==nullptr || pubSubMgr==nullptr){
              throw std::runtime_error("Skcl and pubSubMan pointer cannot be null");
            }
          };
          virtual ~PubsubIF()=default;
    };
    class Skcl:public FDEPOLLRL::EpollSatisfyRDH<Skcl>{
    friend class SkClientCentralSt;
    public:
    EpollInternals::EpollMan* epollMan=nullptr;
    std::unique_ptr<EpollInternals::EpollEvent> epollEv=nullptr;
    std::unique_ptr<socketIO::SocketClient> skc=nullptr;
    std::unique_ptr<PubsubIF> pubSubObj=nullptr;
    int idx=-1;

    Skcl(){};
    Skcl(EpollInternals::EpollEvent* ev,int idx=-1):epollEv(ev),idx(idx){}
    Skcl(std::unique_ptr<EpollInternals::EpollEvent> ev,int idx=-1):epollEv(std::move(ev)),idx(idx){}
    Skcl(socketIO::SocketClient* skc,int idx=-1):skc(skc),idx(idx){}
    Skcl(std::unique_ptr<socketIO::SocketClient>  skc,int idx=-1):skc(std::move(skc)),idx(idx){}

    Skcl(Skcl&)=delete;
    Skcl(Skcl&&oth)noexcept:epollEv(std::move(oth.epollEv)),skc(std::move(oth.skc)),
    pubSubObj(std::move(oth.pubSubObj)),epollMan(oth.epollMan){
        oth.idx=-1;
    };
    Skcl& operator=(Skcl&)=delete;
    Skcl& operator=(Skcl&&oth)noexcept{
        if(&oth==this)
        return *this;
        epollEv=std::move(oth.epollEv);
        pubSubObj=std::move(oth.pubSubObj);
        skc=std::move(skc);
        epollMan=oth.epollMan;
        oth.idx=-1;
        return *this;
    };
    virtual ~Skcl()=default;
    std::pair<int,EpollInternals::epollFlags::EpollModFlags>getEpollInfo(){
        return skc->getEpollInfo();
    }
    bool isWriteComplete(){
        return skc->isWriteComplete();
    }
    bool terminationStatus(){
      return skc->terminationStatus();
    }
    void writeAsync(std::string_view content){
        skc->writeAsync(content);
    }
  };
  class SkClientCentralSt{
    std::vector<std::unique_ptr<Skcl>> cls;
    EpollInternals::EpollMan* epollMan=nullptr;
    public:
    SkClientCentralSt(EpollInternals::EpollMan* epollMan,int maxClients=20):epollMan(epollMan){
        cls.reserve(maxClients);
    };
    Skcl* addCl(Skcl&& cl){
        debug::print("in addCl");
        cl.idx=cls.size();
        cl.epollMan=epollMan;
        cls.push_back(std::make_unique<Skcl>(std::move(cl)));
        return cls.back().get();
    }
    
    void removeCl(Skcl* cl){
        debug::print("in RemoveCl");
        if(cl->idx==-1)return;
        int currIdx=cl->idx;
        cls.back()->idx=currIdx;
        cl->idx=-1;
        std::swap(cls.back(),cls[currIdx]);
        cls.pop_back();
      
    }
    

  };
  class CmdProcessStatus{
    std::string cmdInfo;
    int status;
    public:
      CmdProcessStatus(int status,std::string_view cmdInfo=""):status(status),cmdInfo(cmdInfo){};
      int getStatus(){
        return status;
      }
      std::string getcmdInfo(){
        return cmdInfo;
      }
      static CmdProcessStatus cmdSucess(std::string_view cmdInfo="command success"){
        return CmdProcessStatus(200,cmdInfo);
      }
      static CmdProcessStatus cmdFailed(std::string_view cmdInfo="command failed"){
        return CmdProcessStatus(400,cmdInfo);
      }
      static CmdProcessStatus tpcSubSuccess(){
        return CmdProcessStatus(200,"topic successfully subscribe");
      }
      static CmdProcessStatus tpcSubNA(){
        return CmdProcessStatus(404,"topic unavailble to subscribe");
      }
      static CmdProcessStatus tpcSubFailed(){
        return CmdProcessStatus(400,"topic  subscribe failed ");
      }
      static CmdProcessStatus tpcunSubSuccess(){
        return CmdProcessStatus(200,"topic successfully unsubscribe");
      }
       static CmdProcessStatus tpcUnsubFailed(){
        return CmdProcessStatus(400,"topic  unsubscribe failed ");
      }
      static CmdProcessStatus tpcUnsubNA(){
        return CmdProcessStatus(404,"topic not availbie to  unsubscribe");
      }
       static CmdProcessStatus tpcAddSuccess(){
        return CmdProcessStatus(200,"topic successfully added");
      }
      static CmdProcessStatus tpcAddFailed(){
        return CmdProcessStatus(400,"topic add Failed");
      }
      static CmdProcessStatus tpcRemoveFailedNA(){
        return CmdProcessStatus(404,"topic remove failed as topic is not in server database");
      }
      static CmdProcessStatus tpcRemoveFailed(){
        return CmdProcessStatus(400,"topic remove failed ");
      }
      static CmdProcessStatus tpcRemoveSuccess(){
        return CmdProcessStatus(200,"topic successfully removed ");
      }
      static CmdProcessStatus tpcPublishFailed(){
        return CmdProcessStatus(400,"topic publish failed ");
      }
      static CmdProcessStatus tpcPublishFailedNA(){
        return CmdProcessStatus(404,"topic publish failed as topic is not availible in list");
      }
      static CmdProcessStatus tpcPublishSuccess(){
        return CmdProcessStatus(200,"topic publish success ");
      }
  };
  class Subscriber:public PubsubIF{
      friend class PubSubMan;
      public:
        Subscriber(Skcl* skcl,PubSubMan* pubSubMgr):PubsubIF(skcl,pubSubMgr){};
        Subscriber(Subscriber&)=delete;
        Subscriber& operator=(Subscriber&)=delete;
        Subscriber(Subscriber&&)=default;
        Subscriber& operator=(Subscriber&&)=default;
        CmdProcessStatus addTpc(const std::string &topic);
        CmdProcessStatus remTpc(const std::string &topic);
        std::string topicListShow();
        void write(std::string& st){
            auto res=skcl->skc->write(st);
            if(!res.has_value()){
                EpollInternals::EpollEventListenerModifier(skcl->epollEv.get(),skcl->epollMan).enableWriteEvent()
                ->modifyinEpoll();
            }
        }
        void cleanUpUnsub();

  };
  class Publisher:public PubsubIF{
      public:
        Publisher(Skcl* cl,PubSubMan* pubSubMgr):PubsubIF(cl,pubSubMgr){

        };
        Publisher(Publisher&)=delete;
        Publisher(Publisher&&)=default;
        Publisher& operator=(Publisher&)=delete;
        Publisher& operator=(Publisher&&)=default;
        CmdProcessStatus addTpc(const std::string &topic);
        CmdProcessStatus remTpc(const std::string &topic);
        CmdProcessStatus distributeMsg(const std::string &topic,const std::string &msg);
        
  };
  class TopicsSubsController:public FDEPOLLRL::Controller{
      public:
        std::vector<Subscriber*> subs;

        TopicsSubsController(int initsize=20){
          subs.reserve(initsize);
        }
        CmdProcessStatus addSub(Subscriber* sb){
                subs.push_back(sb);
                return CmdProcessStatus::tpcSubSuccess();
        }
        CmdProcessStatus removeSub(Subscriber* sb){
                int targetIdx=-1;
                for(int i=0;i<subs.size();i++){
                  if(subs[i]==sb){
                    targetIdx=i;
                    break;
                  }
                }
                if(targetIdx==-1){
                  return CmdProcessStatus::tpcUnsubNA();
                };
                std::swap(subs[targetIdx],subs.back());
                subs.pop_back();
                return  CmdProcessStatus::tpcunSubSuccess();


        };
        void write( std::string data)override{
            
            for(Subscriber* sb:subs){
              sb->write(data);
            }
        };
      private:
        std::string read()override{
            std::runtime_error("should not read from subs");
            return "";
        }

  };
  class PubSubMan{
    public:
      PubSubMan(){};
      PubSubMan(PubSubMan&)=delete;
      PubSubMan(PubSubMan&&)=default;
      PubSubMan& operator=(PubSubMan&)=delete;
      PubSubMan& operator=(PubSubMan&&)=default;
    private:
      phmap::flat_hash_map<std::string, TopicsSubsController> subs;
      friend class Subscriber;
      friend class Publisher;
        CmdProcessStatus addTopic(const std::string& topic){
          subs.insert({topic,TopicsSubsController()});
          return CmdProcessStatus::tpcAddSuccess();
        }
        CmdProcessStatus removeTopic(const std::string& topic){
          if(subs.contains(topic)){
            subs.erase(topic);
            return CmdProcessStatus::tpcRemoveSuccess();
          }
          return CmdProcessStatus::tpcRemoveFailedNA();
        }
        CmdProcessStatus addSubtoTopic(const std::string& topic,Subscriber* sub){
          if(subs.contains(topic)){
            return subs[topic].addSub(sub);
          }
          return CmdProcessStatus::tpcSubNA();
        }
        CmdProcessStatus removeSubfromTopic(const std::string& topic,Subscriber* sub){
            if(subs.contains(topic)){

             return subs[topic].removeSub(sub);
            }
            return CmdProcessStatus::tpcUnsubNA();
        };
        void removeSub(Subscriber* sub){
            for(auto &[key,Tpctl]:subs){
                  Tpctl.removeSub(sub);
                  debug::print("in remove Sub");
            }
            
        }
        std::string getTopicList(){
          std::string temp;
          for(auto &[key,Tpctl]:subs){
                  temp+=key+",";
            }
            temp.pop_back();
            return temp;
        }
        CmdProcessStatus publish(const std::string& topic,const std::string& msg);
        

  };
  CmdProcessStatus Subscriber::addTpc(const std::string &topic){
    return pubSubMgr->addSubtoTopic(topic, this);
  }
  CmdProcessStatus Subscriber::remTpc(const std::string &topic){
  return  pubSubMgr->removeSubfromTopic(topic, this);
  }
  std::string Subscriber::topicListShow(){
    return pubSubMgr->getTopicList();
  }
  CmdProcessStatus Publisher::addTpc(const std::string &topic){
   return  pubSubMgr->addTopic(topic);
      
  }
  CmdProcessStatus Publisher::remTpc(const std::string &topic){
   return pubSubMgr->removeTopic(topic);
  }
  CmdProcessStatus Publisher::distributeMsg(const std::string& topic,const std::string& msg){
    return pubSubMgr->publish(topic, msg);
  }
  void Subscriber::cleanUpUnsub(){
    pubSubMgr->removeSub(this);
  }
};
#endif