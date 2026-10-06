#include<epoll.hpp>
#include<socket.hpp>
#include<pubsub.hpp>
#include<pubsubprocessor.hpp>
#include <csignal>
using namespace EpollInternals;
using namespace PubSubEngine;
using namespace socketIO;
void onClientRead(Skcl* cl,EpollEvent*epObj,EpollMan* epollMgr,PubSubMan* pubSubMgr){
while(true){

    try{
      if(!cl->skc->read())break;
  

      if(!cl->pubSubObj){
        Logging::infoDef("initilaizing client to pub or sub");
        auto pubsubVar=PSProtocol::assignType(cl,pubSubMgr);
        bool isAssignSuccess=std::visit(Overloaded{
          [cl](std::unique_ptr<PubsubIF>& ps){
            cl->pubSubObj=std::move(ps);
            cl->writeAsync(PSProtocol::responseStatus(200));
            return true;
          },
          [cl]<typename T>(T& inValidMsg)mutable
          requires IsOneof<T, protoState::InvalidProtoMSG,
             protoState::NotInitialized>
           {
              Logging::warnDef("Invalid Command From Client");
              cl->writeAsync(PSProtocol::responseStatus(404));
              cl->skc->resetReadBuff();
              return false;
          },
          [](auto& oth){
              return false;
          }
            
        },pubsubVar); 
        if(!isAssignSuccess)break;
      }
     
      auto commandRes=PSProtocol::processPSIFcmd(cl->skc.get(), cl->pubSubObj.get());
      bool cmdStatus=std::visit(Overloaded{
          [cl](protoState::CommandProcessed&val){
              cl->writeAsync(PSProtocol::responseStatus(val.getStatus()));
              return true;
          },
          [cl](protoState::ListTpc& list){
              cl->writeAsync(PSProtocol::responseList(list.listTpc));
              return true;
          },
          [cl]<typename T>(T& inValidMsg)
          requires IsOneof<T, protoState::InvalidProtoMSG,
            protoState::InvalidPubCmd,
            protoState::InvalidSubCmd>{
              cl->skc->resetReadBuff();
              Logging::warnDef("Invalid Command From Client");
              cl->writeAsync(PSProtocol::responseStatus(404));
              return false;

          },
          [cl](auto& val){
            return false;
          },
      },commandRes);
      if(!cmdStatus){
        break;
      } 
    }catch(std::exception &e){
      Logging::errorDef("exception in client read:",e.what());
      cl->skc->resetReadBuff();
      break;
    }
  

  }

  try{

    auto res=cl->skc->write();
    if(!res.has_value()){
      EpollEventListenerModifier(epObj,epollMgr).enableWriteEvent()->modifyinEpoll();
    }
  }catch(std::exception &e){
  Logging::errorDef("exception in client write:",e.what());

  }


}
void onClientWrite(Skcl*cl,EpollEvent*epObj,EpollMan*epollMgr){
   auto res=cl->skc->write();
   if(res.has_value()){
      EpollEventListenerModifier(epObj,epollMgr).disableWriteEvent()->modifyinEpoll();
      if(Publisher* pubPtr=dynamic_cast<Publisher*>(cl->pubSubObj.get()) ){
          if(epObj->getRdhupStatus().has_value()){
            epObj->setCleanup();
          }
      }
   }
}

void onHalfClose(Skcl* cl,EpollEvent* epObj,EpollMan* epollMgr){
    if(!dynamic_cast<Subscriber*>(cl->pubSubObj.get()) ){
        epObj->setCleanup();
    }
}
void onTermStatus(Skcl* cl,EpollEvent* epObj){
    epObj->setCleanup();
}
void onCleanup(Skcl* cl,EpollEvent* epObj,SkClientCentralSt* st){
    Logging::infoDef("client disconnected");
    if(Subscriber* sub=dynamic_cast<Subscriber*>(cl->pubSubObj.get())){
      sub->cleanUpUnsub();
    }  
    st->removeCl(cl);
}

void onServRead(NBTcpSocket* skt, EpollEvent*,EpollMan* epollMgr,SkClientCentralSt* st,PubSubMan* pubSubMgr){
  try{
      auto cle=skt->getClient();
      
      if(!cle.has_value())return;
      auto clptr=Skcl(std::make_unique<SocketClient>(std::move(cle.value())));
      auto *cl=st->addCl(std::move(clptr));
      Logging::infoDef("new client connected");
      auto epollev=(epollMgr->createEventObjLinIF(cl,([epollMgr,st,pubSubMgr](EpDef::EEG<Skcl>& bl)mutable{

            bl.onReading(EUtil::deleg<Skcl>(onClientRead,epollMgr,pubSubMgr));
            bl.onWrite(EUtil::deleg<Skcl>(onClientWrite,epollMgr),false);
            bl.onHalfClose(EUtil::deleg<Skcl>(onHalfClose,epollMgr));
            bl.onCleanup(EUtil::deleg<Skcl>(onCleanup,st));
            bl.onTermStatus(onTermStatus);
            
      })));
      cl->epollEv=std::unique_ptr<EpollEvent>(epollev);
    //   auto cl=store.
      
    }catch(std::exception& e){
      Logging::errorDef("exception in client connection:",e.what());
      
    }
}
int main(){
  
  Logging::Logger logger("pubsub.txt");
  Logging::setDefaultLogger(logger);
  NBTcpSocket pubsub(3000,20);
  EpollMan epollMgr(10,epollFlags::createcloseonExec);
  PubSubMan pubSubMgr;
  static EpollMan* stEpollMgrRef = &epollMgr;

  std::signal(SIGINT, [](int val){
    if (stEpollMgrRef) {
        stEpollMgrRef->stopLoop();
        std::cout << "waiting until stop loop";
    }
  });
  SkClientCentralSt skclState(&epollMgr);
  epollMgr.createEventObjLinIF(&pubsub, 
    [epollMgrRef=&epollMgr,skclStateRef=&skclState,pubSubMgrRef=&pubSubMgr](EpDef::EEG<NBTcpSocket> &bld ){
            bld.onReading(EUtil::deleg<NBTcpSocket>(onServRead,epollMgrRef,skclStateRef,pubSubMgrRef));
    });
    
    epollMgr.runEventLoop();
}