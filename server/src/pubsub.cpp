#include<epoll.hpp>
#include<socket.hpp>
#include<pubsub.hpp>
#include<pubsubprocessor.hpp>
using namespace EpollInternals;
using namespace PubSubEngine;
using namespace socketIO;
void onClientRead(Skcl* cl,EpollEvent*epObj,EpollMan* epollMgr,PubSubMan* pubSubMgr){
while(true){

    try{
      if(!cl->skc->read())return;
      
      if(!cl->pubSubObj){
        auto pubsubVar=PSProtocol::assignType(cl,pubSubMgr);
        bool isAssignSuccess=std::visit(Overloaded{
          [cl](std::unique_ptr<PubsubIF>& ps){
            cl->pubSubObj=std::move(ps);
            return true;
          },
          [cl](protoState::InvalidProtoMSG& inValidMsg)mutable{
              debug::print(inValidMsg.info());
              cl->skc->WriteDataBuffremain.append(inValidMsg.info());
              cl->skc->resetReadBuff();
              return false;
          },
          [cl](protoState::NotInitialized& ntInit)mutable{
              debug::print(ntInit.info());
              cl->skc->resetReadBuff();
              cl->skc->WriteDataBuffremain.append(ntInit.info());

              return false;
          },
          [](auto& oth){
              debug::print(oth.info());
              return false;
          }
            
        },pubsubVar); 
        if(!isAssignSuccess)break;
      }
      if(Publisher* pub=dynamic_cast<Publisher*>(cl->pubSubObj.get())){
        auto commandRes=PSProtocol::processCmdPub(cl->skc.get(), pub);
        bool cmdStatus=std::visit(Overloaded{
            [](protoState::CommandProcessed&val){
                debug::print(val.info());
                return true;
            },
            [cl](protoState::InvalidProtoMSG& inValidMsg){
              debug::print(inValidMsg.info());
              cl->skc->resetReadBuff();
                return false;

            },
            [cl](protoState::InvalidPubCmd& inValidMsg){
              debug::print(inValidMsg.info());
              cl->skc->resetReadBuff();
              return false;


            },
            [cl](auto& val){
              debug::print(val.info());
              cl->skc->WriteDataBuffremain.append(val.info());
              return false;
            },
        },commandRes);
       if(!cmdStatus){
          break;
       } 
      } 
      if(Subscriber* sub=dynamic_cast<Subscriber*>(cl->pubSubObj.get())){
        auto cmdRes=PSProtocol::processCmdSub(cl->skc.get(), sub);
        auto cmdStatus=std::visit(Overloaded{
           [](protoState::CommandProcessed&val){
                debug::print(val.info());
                return true;
            },
            [cl](protoState::InvalidProtoMSG& inValidMsg){
              debug::print(inValidMsg.info());
              cl->skc->resetReadBuff();
                return false;

            },
            [cl](protoState::InvalidSubCmd& inValidMsg){
              debug::print(inValidMsg.info());
              cl->skc->resetReadBuff();
              return false;


            },
            [](auto&val){
              debug::print(val.info());
              return false;
            },
        },cmdRes);
        if(!cmdStatus){
          break;
        }
      }
    }catch(std::exception &e){
      debug::print(e.what());
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
      debug::print(e.what()); 
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
    if(Publisher* pubPtr=dynamic_cast<Publisher*>(cl->pubSubObj.get()) ){
        epObj->setCleanup();
    }
}
void onCleanup(Skcl* cl,EpollEvent* epObj,SkClientCentralSt* st){
    st->removeCl(cl);
}

void onServRead(NBTcpSocket* skt, EpollEvent*,EpollMan* epollMgr,SkClientCentralSt* st,PubSubMan* pubSubMgr){
  try{
      auto cle=skt->getClient();
      if(!cle.has_value())return;
      auto *cl=st->addCl(std::make_unique<SocketClient>(std::move(cle.value())));
      auto epollev=(epollMgr->createEventObjLinIF(cl,([epollMgr,st,pubSubMgr](EpDef::EEG<Skcl>& bl)mutable{
          
            bl.onReading(EUtil::deleg<Skcl>(onClientRead,epollMgr,pubSubMgr));
            bl.onWrite(EUtil::deleg<Skcl>(onClientWrite,epollMgr),false);
            bl.onHalfClose(EUtil::deleg<Skcl>(onHalfClose,epollMgr));
            bl.onCleanup(EUtil::deleg<Skcl>(onCleanup,st));
            
      })));
      cl->epollEv=std::unique_ptr<EpollEvent>(epollev);

    //   auto cl=store.
      
    }catch(std::exception e){
        debug::print(e.what());
    }
}
int main(){
   

    NBTcpSocket pubsub(3000,20);
    EpollMan epollMgr(10,epollFlags::createcloseonExec);
    PubSubMan pubSubMgr;
    SkClientCentralSt skclState(&epollMgr);
    epollMgr.createEventObjLinIF(&pubsub, 
      [epollMgrRef=&epollMgr,skclStateRef=&skclState,pubSubMgrRef=&pubSubMgr](EpDef::EEG<NBTcpSocket> &bld ){
            bld.onReading(EUtil::deleg<NBTcpSocket>(onServRead,epollMgrRef,skclStateRef,pubSubMgrRef));
    });
    epollMgr.runEventLoop();
}