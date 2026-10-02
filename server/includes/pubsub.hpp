#ifndef PUB_SUB_HPP
#define PUB_SUB_HPP
#include "socket.hpp"
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
    int idx;

    Skcl(){};
    Skcl(EpollInternals::EpollEvent* ev,int idx=-1):epollEv(ev),idx(idx){}
    Skcl(std::unique_ptr<EpollInternals::EpollEvent> ev,int idx=-1):epollEv(std::move(ev)),idx(idx){}
    Skcl(socketIO::SocketClient* skc,int idx=-1):skc(skc),idx(idx){}
    Skcl(std::unique_ptr<socketIO::SocketClient>  skc,int idx=-1):skc(std::move(skc)),idx(idx){}

    Skcl(Skcl&)=delete;
    Skcl(Skcl&&oth)noexcept:epollEv(std::move(oth.epollEv)){
      oth.idx=-1;
    };
    Skcl& operator=(Skcl&)=delete;
    Skcl& operator=(Skcl&&oth)noexcept{
        if(&oth==this)
        return *this;
        epollEv=std::move(oth.epollEv);
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
  };
  class SkClientCentralSt{
    std::vector<std::unique_ptr<Skcl>> cls;
    EpollInternals::EpollMan* epollMan=nullptr;
    public:
    SkClientCentralSt(EpollInternals::EpollMan* epollMan,int maxClients=20):epollMan(epollMan){
        cls.reserve(maxClients);
    };
    Skcl* addCl(Skcl&& cl){
        cl.idx=cls.size();
        cl.epollMan=epollMan;
        cls.push_back(std::make_unique<Skcl>(std::move(cl)));
        return cls.back().get();
    }
    
    void removeCl(Skcl* cl){
        if(cl->idx==-1)return;
        std::swap(cls.back(),cls[cl->idx]);
        cls.pop_back();
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
        void addTpc(const std::string &topic);
        void remTpc(const std::string &topic);
        std::string topicListShow();
        void write(std::string& st){
            auto res=skcl->skc->write(st);
            if(!res.has_value()){
                EpollInternals::EpollEventListenerModifier(skcl->epollEv.get(),skcl->epollMan).enableWriteEvent()
                ->modifyinEpoll();
            }
        }

  };
  class Publisher:public PubsubIF{
      public:
        Publisher(Skcl* cl,PubSubMan* pubSubMgr):PubsubIF(cl,pubSubMgr){

        };
        Publisher(Publisher&)=delete;
        Publisher(Publisher&&)=default;
        Publisher& operator=(Publisher&)=delete;
        Publisher& operator=(Publisher&&)=default;
        void addTpc(const std::string &topic);
        void remTpc(const std::string &topic);
        void distributeMsg(const std::string &topic,const std::string &msg);
        
  };
  class TopicsSubsController:public FDEPOLLRL::Controller{
      public:
        std::vector<Subscriber*> subs;

        TopicsSubsController(int initsize=20){
          subs.reserve(initsize);
        }
        void addSub(Subscriber* sb){
                subs.push_back(sb);
        }
        void removeSub(Subscriber* sb){
                int targetIdx=-1;
                for(int i=0;i<subs.size();i++){
                  if(subs[i]==sb){
                    targetIdx=i;
                  }
                }
                if(targetIdx==-1)return;
                std::swap(subs[targetIdx],subs.back());
                subs.pop_back();
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
        void addTopic(const std::string& topic){
          subs.insert({topic,TopicsSubsController()});
        }
        void removeTopic(const std::string& topic){
          subs.erase(topic);
        }
        void addSubtoTopic(const std::string& topic,Subscriber* sub){
          subs[topic].addSub(sub);
        }
        void removeSubfromTopic(const std::string& topic,Subscriber* sub){
            subs[topic].removeSub(sub);
        };
        void removeSub(Subscriber* sub){
            for(auto &[key,Tpctl]:subs){
                  Tpctl.removeSub(sub);
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
        void publish(const std::string& topic,const std::string& msg);
        

  };
  void Subscriber::addTpc(const std::string &topic){
    pubSubMgr->addSubtoTopic(topic, this);
  }
  void Subscriber::remTpc(const std::string &topic){
    pubSubMgr->removeSubfromTopic(topic, this);
  }
  std::string Subscriber::topicListShow(){
    return pubSubMgr->getTopicList();
  }
  void Publisher::addTpc(const std::string &topic){
    pubSubMgr->addTopic(topic);
      
  }
  void Publisher::remTpc(const std::string &topic){
    pubSubMgr->removeTopic(topic);
  }
  void Publisher::distributeMsg(const std::string& topic,const std::string& msg){
    pubSubMgr->publish(topic, msg);
  }
};
#endif