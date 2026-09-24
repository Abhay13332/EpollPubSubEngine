#ifndef PUB_SUB_HPP
#define PUB_SUB_HPP
#include <fileWatcherandpub.hpp>
#include<parallel_hashmap/phmap.h>
namespace PubSubEngine{
  class Subscriber{
      public:
        Subscriber()=default;
        Subscriber(Subscriber&)=delete;
        Subscriber& operator=(Subscriber&)=delete;
        Subscriber(Subscriber&&)=default;
        Subscriber& operator=(Subscriber&&)=default;

  };
  class Publisher{
      public:
        Publisher()=default;
        Publisher(Publisher&)=delete;
        Publisher(Publisher&&)=default;
        Publisher& operator=(Publisher&)=delete;
        Publisher& operator=(Publisher&&)=default;
        
  };
  class TopicsSubsController:public EpollInternals::Controller{
      public:
        std::vector<Subscriber> subs;
        TopicsSubs(int initsize){
          subs.reserve(initsize);
        }
        void addSub(Subscriber&& sb){
                subs.push_back(std::move(sb));
        }
        void removeSub();

  };
  class PubSubMan{
        phmap::flat_hash_map<std::string, TopicsSubs> subs;
        int defaultCap;
        PubSubMan(int defaultSubCap=20):defaultCap(defaultSubCap){};
        PubSubMan(PubSubMan&)=delete;
        PubSubMan(PubSubMan&&)=default;
        PubSubMan& operator=(PubSubMan&)=delete;
        PubSubMan& operator=(PubSubMan&&)=default;
        void addTopic(std::string& topic){
          subs.insert({topic,std::move(TopicsSubs(defaultCap))});
        }
        void removeTopic(std::string& topic){
          subs.erase(topic);
        }
        void addSub(std::string& topic,Subscriber&& sub){
          subs[topic].addSub(std::move(sub));
        }
        void removeSub();
        void publish(std::string& topic,){

        }
        

  };
};
#endif